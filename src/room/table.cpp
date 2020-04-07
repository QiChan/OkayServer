#include <iomanip>
#include "table.h"
#include "room.h"
#include "okey_poker.h"
#include "../pb/table.pb.h"

Table::Table(Room* room, uint32_t tableid, uint32_t seat_num)
    : room_(room),
      is_playing_(false),
      tableid_(tableid),
      game_count_(0),
      base_score_(0),
      start_game_timer_(0)
{
    if (room_->GetRoomType() == pb::OKEY_ROOM)
    {
        poker_ = std::make_shared<OkeyPoker>();
    }
    else
    {
        poker_ = std::make_shared<OkeyPoker101>();
    }

    seats_.reserve(seat_num);
    for (size_t i = 0; i < seat_num; i++)
    {
        seats_.push_back(std::make_shared<Seat>(i));
    }

    action_component_ = std::make_shared<ActionComponent>(this);
}

Table::~Table()
{
    if (!IsTableEmpty())
    {
        LOG(ERROR) << "table not empty. seat: [" << SitedSeatIterator(this) << "].";
    }
}

void Table::ShutdownTable()
{
    if (!CheckShutdownTable())
    {
        ROOM_XLOG(ERROR) << "cannot shutdown table";
        return;
    }

    // 迭代器失效
    for (auto it = bystanders_.begin(); it != bystanders_.end();)
    {
        auto user = *it;
        it++;
        room_->KickRoomUser(user);
    }
    
    SitedSeatIterator   seat_iter(this);
    for (auto seat = *seat_iter; seat != nullptr; seat = *(++seat_iter))
    {
        auto user = seat->GetSeatUser();
        if (user == nullptr)
        {
            continue;
        }
        if (!room_->KickRoomUser(user))
        {
            ROOM_XLOG(ERROR) << "kick user error. " << seat->GetSeatBrief(user).ShortDebugString();
        }
    }

    room_->StopTimer(start_game_timer_);
    ROOM_XLOG(INFO) << "shutdown table";
}

bool Table::CheckShutdownTable() const
{
    return !IsPlaying();
}

void Table::GameOver(SeatPtr winner, RepeatedPtrField<pb::CardGroup>* cards)
{
    // 此函数的逻辑不能放到定时器去执行，避免出现并发问题，因为is_playing_已经置为false了
    is_playing_ = false;
    action_component_->Reset();

    DelayStartGame();

    std::vector<SeatPtr> playing_seats;
    int64_t win_chips = 0;

    pb::iGameRecord record;
    record.set_room_set_id(room_->GetRoomSetID());
    record.set_game_id(game_id_);
    record.set_game_time(time(NULL));

    // 统计输赢信息
    VStatGameRecord(winner, cards, record, playing_seats, win_chips);

    // 输赢信息持久化
    room_->SendToConcurrentService(record, room_->ServiceHandle("persistence"));

    // 手牌结束事件
    room_->VGameOverEvent(GetTableID(), game_id_, playing_seats, winner, win_chips);
}

void Table::DelayStartGame()
{
    if (IsPlaying())
    {
        return;
    }

    room_->StopTimer(start_game_timer_);
    // 3秒后自动开始下一手
    start_game_timer_ = room_->StartTimer(300, std::bind(&Table::StartGame, this));
}

void Table::StartGame()
{
    if (IsPlaying())
    {
        ROOM_XLOG(ERROR) << "already start.";
        return;
    }

    ReadyToPlayGame();

    if (!CheckStartGame())
    {
        DelayStartGame();
        room_->VGameStartFailEvent(GetTableID());
        return;
    }

    StartPlayGame();

    room_->VGameStartEvent(GetTableID(), game_id_);
}

void Table::StartPlayGame()
{
    is_playing_ = true;

    // 生成手牌id
    ResetGameID();

    {
        ReadySeatIterator   seat_iter(this);
        for (auto seat = *seat_iter; seat != nullptr; seat = *(++seat_iter))
        {
            seat->StartPlayGame(base_score_);
        }
    }

    {
        // 1. 洗牌
        poker_->Shuffle();
        // 2. 抽indicator
        poker_->VChooseIndicator();
        // 3. 抽庄位
        action_component_->ChooseDealer(seats_.size()); 
        // 4. 发牌
        VDealCards();
    }

    {
        SitedSeatIterator iter(this);
        for (auto seat = *iter; seat != nullptr; seat = *(++iter))
        {
            auto user = seat->GetSeatUser();
            if (user == nullptr)
            {
                continue;
            }

            pb::GameStartBRC    brc;
            brc.set_roomid(GetRoomID());
            brc.set_tableid(GetTableID());
            brc.set_dealer_seatid(action_component_->GetActionSeatID());
            brc.mutable_table_brief()->CopyFrom(VGetTableBrief(user));
            user->SendToUser(brc);
        }

        for (auto& user : bystanders_)
        {
            pb::GameStartBRC    brc;
            brc.set_roomid(GetRoomID());
            brc.set_tableid(GetTableID());
            brc.set_dealer_seatid(action_component_->GetActionSeatID());
            brc.mutable_table_brief()->CopyFrom(VGetTableBrief(user));
            user->SendToUser(brc);
        }
    }

    ROOM_XLOG(INFO) << "start play game. gameid: " << game_id_ << " base_score: " << base_score_ << " playing_user: [" << PlayingSeatIterator(this) << "]";
}

void Table::ReadyToPlayGame()
{
    base_score_ = room_->VGetRoomBaseScore();

    SitedSeatIterator iter(this);
    for (auto seat = *iter; seat != nullptr; seat = *(++iter))
    {
        seat->ReadyToPlayGame(base_score_);
    }
}

bool Table::CheckStartGame()
{
    if (!room_->VCheckStartGame(GetTableID()))
    {
        return false;
    }

    // 获取可玩牌人数
    auto sited_num = GetReadyPlayNum();
    if (sited_num < GetNeedMinSeatNum())
    {
        return false;
    }

    return true;
}

bool Table::UserEnterTable(RoomUserPtr user)
{
    auto sp_seat = user->GetUserSeat();
    if (sp_seat == nullptr)
    {
        bystanders_.insert(user);
    }
    else
    {
        sp_seat->EnterTable(user);
        
        pb::EnterTableBRC   brc;
        brc.set_roomid(GetRoomID());
        brc.set_tableid(tableid_);
        brc.mutable_seat_brief()->CopyFrom(sp_seat->GetSeatBrief());
        BroadcastToUser(brc, user);
    }
    return true;
}

bool Table::UserLeaveTable(RoomUserPtr user)
{
    // 旁观者离开房间不需要广播消息
    bystanders_.erase(user);

    // 有座位就不让离开房间，需要先执行站起操作
    auto seat = user->GetUserSeat();
    if (seat == nullptr)
    {
        return true;
    }

    ROOM_XLOG(ERROR) << "leave table error, should standup first. " << user;
    return false;
}

bool Table::UserStandUP(RoomUserPtr user)
{
    auto seat = user->GetUserSeat();
    if (seat == nullptr)
    {
        return false;
    }

    pb::SeatBrief brief = seat->GetSeatBrief();
    if (!seat->StandUP())
    {
        return false;
    }
    
    pb::StandUPBRC  brc;
    brc.set_roomid(GetRoomID());
    brc.set_tableid(tableid_);
    brc.mutable_seat_brief()->CopyFrom(brief);
    BroadcastToUser(brc, user);

    // 加入旁观者列表
    bystanders_.insert(user);

    return true;
}

bool Table::UserHostSeat(RoomUserPtr user)
{
    auto seat = user->GetUserSeat();
    if (seat == nullptr)
    {
        return false;
    }
    seat->HostSeat();

    pb::HostSeatBRC brc;
    brc.set_roomid(GetRoomID());
    brc.set_tableid(tableid_);
    brc.mutable_seat_brief()->CopyFrom(seat->GetSeatBrief());
    BroadcastToUser(brc, user);

    action_component_->HandleHostSeatEvent(seat);

    return true;
}

std::tuple<bool, Table::SeatPtr> Table::UserSitDown(RoomUserPtr user, int32_t seatid, int64_t chips, int64_t last_money)
{
    // 自动坐下
    if (seatid < 0)
    {
        auto it = seats_.begin();
        for (; it != seats_.end(); ++it)
        {
            auto seat = *it;
            if (!seat->IsSited())
            {
                seatid = seat->GetSeatID();
                break;
            }
        }

        if (it == seats_.end())
        {
            return std::make_tuple(false, nullptr);
        }
    }

    auto sp_seat = GetSeat(seatid);
    if (sp_seat == nullptr)
    {
        return std::make_tuple(false, nullptr);
    }

    if (!sp_seat->SitDown(user, chips, last_money))
    {
        return std::make_tuple(false, nullptr);
    }

    // 从旁观者列表删除
    bystanders_.erase(user);

    // 广播玩家坐下的消息
    pb::SitDownBRC  brc;
    brc.set_roomid(GetRoomID());
    brc.set_tableid(tableid_);
    brc.mutable_seat_brief()->CopyFrom(sp_seat->GetSeatBrief());
    BroadcastToUser(brc, user);

    return std::make_tuple(true, sp_seat);
}

Table::SeatPtr Table::GetSeat(uint32_t seatid)
{
    if (seatid >= seats_.size())
    {
        return nullptr;
    }
    return seats_[seatid];
}

void Table::BroadcastToUser(const Message& msg, RoomUserPtr except_user)
{
    for (auto& seat : seats_)
    {
        auto user = seat->GetSeatUser();
        if (user == nullptr)
        {
            continue;
        }
        if (user == except_user)
        {
            continue;
        }
        user->SendToUser(msg);
    }

    for (auto& user : bystanders_)
    {
        if (user == except_user)
        {
            continue;
        } 
        user->SendToUser(msg);
    }
}

pb::TableBrief Table::VGetTableBrief(RoomUserPtr user) 
{
    pb::TableBrief  brief;
    brief.set_roomid(room_->GetRoomID());
    brief.set_tableid(tableid_);
    brief.set_playing(is_playing_);
    brief.set_indicator_card(poker_->VGetIndicatorCard());
    brief.set_last_card_num(poker_->GetLastCardNum());
    brief.set_action_time(room_->GetActionTime());

    for (auto& seat : seats_)
    {
        auto seat_brief = brief.add_seat_brief();
        seat_brief->CopyFrom(seat->GetSeatBrief(user));
    }

    brief.mutable_action_brief()->CopyFrom(action_component_->GetActionBrief());

    return brief;
}

uint32_t Table::GetRoomID() const 
{
    return room_->GetRoomID();
}

uint32_t Table::GetTableID() const
{
    return tableid_;
}

uint32_t Table::GetSitedNum()
{
    uint32_t num = 0;
    SitedSeatIterator   iter(this);
    for (auto seat = *iter; seat != nullptr; seat = *(++iter))
    {
        num++;
    }
    return num;
}

uint32_t Table::GetReadyPlayNum()
{
    uint32_t num = 0;
    ReadySeatIterator   iter(this);
    for (auto seat = *iter; seat != nullptr; seat = *(++iter))
    {
        num++;
    }
    return num;
}

void Table::ResetGameID()
{
    game_count_++;
    std::stringstream ss;
    ss << room_->GetRoomSetID() << '-' << tableid_;
    ss << '-' << std::setw(8) << std::setfill('0') << game_count_;
    game_id_ = ss.str();
}

bool Table::IsTableFull()
{
    uint32_t sited_num = GetSitedNum();
    return sited_num >= seats_.size();
}

bool Table::IsTableEmpty()
{
    uint32_t sited_num = GetSitedNum();
    return sited_num == 0;
}

int Table::StartTimer(int time, const TimerCallBack& func, const std::string& timer_name)
{
    return room_->StartTimer(time, func, timer_name);
}

void Table::StopTimer(int& id)
{
    room_->StopTimer(id);
}

uint32_t Table::GetActionTime() const
{
    return room_->GetActionTime();
}

uint32_t Table::GetNeedMinSeatNum() const
{
    if (seats_.size() > 2)
    {
        return 3;
    }
    else
    {
        return 2;
    }
}

int64_t Table::CalculateRake(int64_t chips) const
{
    int64_t rake = 0;
    uint32_t rake_rate = room_->VGetRoomRakeRate();
    rake = chips * rake_rate / 100;

    // 四舍五入
    return floor( (double)rake / 100 ) * 100;
}

pb::iGameUserRecord Table::GetGameUserRecord(RoomUserPtr user, int64_t rake, int64_t chips)
{
    pb::iGameUserRecord record;
    if (user == nullptr)
    {
        ROOM_XLOG(ERROR) << "user is nullptr. rake: " << rake << " chips: " << chips;
        return record;
    }

    record.set_clubid(user->GetClubID());
    record.set_uid(user->uid());
    record.set_rake(rake);
    record.set_chips(chips);

    return record;
}
