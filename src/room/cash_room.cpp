#include "cash_room.h"

CashRoom::CashRoom()
{
}

CashRoom::~CashRoom()
{
}

bool CashRoom::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Room::VInitService(ctx, parm, len))
    {
        return false;
    }

    // 创建第一张桌子
    NewRoomTable();

    RegisterCashRoomCallBack();
    return true;
}

CashRoom::TablePtr CashRoom::GetRoomTable(UserPtr user)
{
    // TODO: for test, always retur first table
    return tables_[0];

    auto table = user->GetUserTable();
    if (table != nullptr)
    {
        return table;
    }

    std::vector<TablePtr>       first_priority;
    std::vector<TablePtr>       second_priority;
    std::vector<TablePtr>       third_priority; 
    std::vector<TablePtr>       fourth_priority;
    std::vector<TablePtr>       empty_tables;
    first_priority.reserve(tables_.size());
    second_priority.reserve(tables_.size());
    third_priority.reserve(tables_.size());
    fourth_priority.reserve(tables_.size());
    empty_tables.reserve(tables_.size());

    for (auto& elem : tables_)
    {
        auto table = elem.second;
        if (table == nullptr)
        {
            ROOM_XLOG(ERROR) << "table is nullptr, tableid: " << elem.first;
            continue;
        }

        // 判断是否满人
        if (table->IsTableFull())
        {
            continue;
        }

        if (table->IsTableEmpty())
        {
            empty_tables.emplace_back(table);
            continue;
        }

        // 2人桌的情况选出第一张不为空的桌子，如果没有再选出第一张空的桌子
        if (GetSeatNum() == 2)
        {
            if (table->IsPlaying())
            {
                continue;
            }
            first_priority.emplace_back(table);
        }
        else
        {
            // 优先3人桌
            if (table->GetSitedNum() == 3)
            {
                if (!table->IsPlaying())
                {
                    // 最高优先级直接return
                    first_priority.emplace_back(table);
                }
                else
                {
                    // 3人桌在玩第二优先级
                    second_priority.emplace_back(table);
                }
            }
            else if (table->GetSitedNum() == 2)
            {
                // 2 人桌第三优先级
                third_priority.emplace_back(table);
            }
            else
            {
                // 1 人桌第四优先级
                fourth_priority.emplace_back(table);
            }
        }
    }

    if (!first_priority.empty())
    {
        return *first_priority.begin();
    }

    if (!second_priority.empty())
    {
        return *second_priority.begin();
    }

    if (!third_priority.empty())
    {
        return *third_priority.begin();
    }

    if (!fourth_priority.empty())
    {
        return *fourth_priority.begin();
    }

    if (empty_tables.empty())
    {
        return NewRoomTable();
    }
    else
    {
        return *empty_tables.begin();
    }
}

void CashRoom::RegisterCashRoomCallBack()
{
    RegisterCallBack();

    MsgCallBack(pb::SitDownREQ::descriptor()->full_name(), std::bind(&CashRoom::HandleClientMsgSitDownREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::StandUPREQ::descriptor()->full_name(), std::bind(&CashRoom::HandleClientMsgStandUPREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::LeaveRoomAfterThisHandREQ::descriptor()->full_name(), std::bind(&CashRoom::HandleClientMsgLeaveRoomAfterThisHandREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::ChangeRoomREQ::descriptor()->full_name(), std::bind(&CashRoom::HandleClientMsgChangeRoomREQ, this, std::placeholders::_1, std::placeholders::_2));
}

void CashRoom::HandleClientMsgLeaveRoomAfterThisHandREQ(MessagePtr data, UserPtr user)
{
    wait_leave_room_uids_.insert(user->uid());
    wait_change_table_uids_.erase(user->uid());

    pb::LeaveRoomAfterThisHandRSP   rsp;
    rsp.set_code(0);
    rsp.set_roomid(GetRoomID());
    user->SendToUser(rsp);
}

void CashRoom::HandleClientMsgChangeRoomREQ(MessagePtr data, UserPtr user)
{
    wait_leave_room_uids_.erase(user->uid());

    auto seat = GetUserSeat(user->uid());
    if (seat == nullptr)
    {
        return;
    }

    pb::ChangeRoomRSP   rsp;
    rsp.set_roomid(GetRoomID());
    if (seat->IsPlaying())
    {
        wait_change_table_uids_.insert(user->uid());
        rsp.set_code(0);
    }
    else
    {
        if (ChangeTable(user))
        {
            rsp.set_code(0);
        }
        else
        {
            rsp.set_code(-1);
        }
    }
    user->SendToUser(rsp);
}

void CashRoom::HandleClientMsgSitDownREQ(MessagePtr data, UserPtr user)
{
    if (IsServerStop())
    {
        return;
    }

    auto msg = std::dynamic_pointer_cast<pb::SitDownREQ>(data);
    int32_t seatid = msg->seatid();

    SitDown(user, seatid);
}

bool CashRoom::ChangeRoomUserMoneyCallBack(MessagePtr data, uint64_t uid, int64_t chips, const std::string& type)
{
    auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);

    if (msg->code() != 0)
    {
        return false;
    }

    // 服务关闭了，禁止坐下
    if (IsServerStop())
    {
        ChangeRoomUserMoney(chips, uid, type, room_set_id_);
        ROOM_XLOG(INFO) << type << ", because server stop. room_set_id: " << room_set_id_ << " uid: " << uid << " chips: " << chips;
        return false;
    }

    // TODO: 小于三倍的时候弹出提示

    return true;
}


void CashRoom::HandleClientMsgStandUPREQ(MessagePtr data, UserPtr user)
{
    pb::StandUPRSP  rsp;
    rsp.set_roomid(GetRoomID());
    auto table = user->GetUserTable();
    if (table == nullptr)
    {
        return;
    }
    pb::TableBrief brief = table->VGetTableBrief(user);
    if (!user->StandUP())
    {
        rsp.set_code(-1);
    }
    else
    {
        rsp.set_code(0);
        rsp.set_tableid(table->GetTableID());
        rsp.mutable_table_brief()->CopyFrom(brief);
    }
    user->SendToUser(rsp);
}

void CashRoom::VUserSitDownEvent(UserPtr user, SeatPtr seat)
{
    Room::VUserSitDownEvent(user, seat);

    auto table = GetUserTable(user->uid());
    if (table == nullptr)
    {
        ROOM_XLOG(ERROR) << "sit down done. but table is null." << user;
        return;
    }

    // 坐下人数够触发一次DelayStartGame逻辑, 防止房间一直在等待
    if (table->GetSitedNum() >= table->GetNeedMinSeatNum())
    {
        table->DelayStartGame();
    }
    ROOM_XLOG(INFO) << user;
}

void CashRoom::VUserStandUPEvent(uint64_t uid, pb::SeatBrief brief)
{
    Room::VUserStandUPEvent(uid, brief);

    ChangeRoomUserMoney(brief.chips(), uid, "cash_standup", room_set_id_);

    ROOM_XLOG(INFO) << "user stand up. " << brief.ShortDebugString();
}

void CashRoom::VGameStartEvent(uint32_t tableid, const std::string& game_id)
{
    Room::VGameStartEvent(tableid, game_id);
}

void CashRoom::VGameOverEvent(uint32_t tableid, const std::string& game_id, std::vector<SeatPtr>& playing_seats, SeatPtr winner, int64_t win_chips)
{
    Room::VGameOverEvent(tableid, game_id, playing_seats, winner, win_chips);

    if (winner != nullptr && win_chips != 0)
    {
        // 给赢的玩家结算
        ChangeRoomUserMoney(win_chips, winner->GetSeatUID(), "cash_win", game_id);
        ROOM_XLOG(INFO) << "game_id: " << game_id << " winner: [" << winner << "] win_chips: " << win_chips;
    }

    // 判断要不要关闭房间
    if (IsServerStop())
    {
        if (CheckShutdownCashRoom())
        {
            DelayShutdownRoom(30);
        }

        // 关闭当前桌子
        tables_[tableid]->ShutdownTable();

        return;
    }

    int64_t chips = VGetRoomBaseScore();


    auto func = [this, chips](MessagePtr data, uint64_t uid){
        std::string type = "rollback_cash_buyin";
        if (!ChangeRoomUserMoneyCallBack(data, uid, chips, type))
        {
            // 让玩家站起
            auto seat = GetUserSeat(uid);
            if (seat == nullptr)
            {
                return;
            }
            auto user = seat->GetSeatUser();
            user->StandUP();
            ROOM_XLOG(INFO) << "user stand up, because no more money to play " << user;
            return;
        }

        auto user = GetUser(uid);
        if (user == nullptr)
        {
            // rollback
            ChangeRoomUserMoney(chips, uid, type, room_set_id_);
            ROOM_XLOG(INFO) << type << ", because user not in room room_set_id: " << room_set_id_ << " uid: " << uid << " chips: " << chips;
            return;
        }

        auto seat = user->GetUserSeat();
        if (seat == nullptr)
        {
            // rollback
            ChangeRoomUserMoney(chips, uid, type, room_set_id_);
            ROOM_XLOG(INFO) << type << ", because user not in seat room_set_id: " << room_set_id_ << " uid: " << uid << " chips: " << chips;
            return;
        }

        auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);
        int64_t last_money = msg->value();

        if (!seat->AddHandChips(chips, last_money))
        {
            // rollback
            ChangeRoomUserMoney(chips, uid, type, room_set_id_);
            ROOM_XLOG(INFO) << type << ", because seat AddHandChips error, room_set_id: " << room_set_id_ << " uid: " << uid << " chips: " << chips;
            return;
        }
        return;
    };


    for (auto& seat : playing_seats)
    {
        // 下手离开的玩家踢出去
        auto user = seat->GetSeatUser();
        if (user == nullptr)
        {
            ROOM_XLOG(ERROR) << "seat_user is nullptr. tableid: " << tableid << " seatid: " << seat->GetSeatID();
            continue;
        }

        // 下手离开的玩家处理
        {
            auto it = wait_leave_room_uids_.find(user->uid());
            if (it != wait_leave_room_uids_.end())
            {
                // 注意it失效了
                wait_leave_room_uids_.erase(it);

                if (!KickRoomUser(user))
                {
                    ROOM_XLOG(ERROR) << "kick user error. " << seat;
                }
                else
                {
                    ROOM_XLOG(INFO) << "kick seat out when game over. " << seat;
                }

                continue;
            }
        }

        // 下手换桌的玩家处理
        {
            auto it = wait_change_table_uids_.find(user->uid());
            if (it != wait_change_table_uids_.end())
            {
                // 注意it失效了
                wait_change_table_uids_.erase(it);

                if (ChangeTable(user))
                {
                    continue;
                }
            }
        }

        // 托管的玩家踢出去
        if (seat->IsHost())
        {
            if (user == nullptr)
            {
                ROOM_XLOG(ERROR) << "seat_user is nullptr. tableid: " << tableid << " seatid: " << seat->GetSeatID();
                continue;
            }
            KickRoomUser(user);
            ROOM_XLOG(INFO) << "kick host seat out. " << seat;
            continue;
        }

        // 买入
        if (seat->GetHandChips() < chips)
        {
            ChangeRoomUserMoney(-chips, seat->GetSeatUID(), "cash_buyin", room_set_id_, std::bind(func, std::placeholders::_1, seat->GetSeatUID()));
        }
    }
}

void CashRoom::VServerStopEvent()
{
    Room::VServerStopEvent();

    // 如果房间可以关闭，那么30秒后关闭房间，需要预留30秒的缓冲时间
    if (CheckShutdownCashRoom())
    {
        DelayShutdownRoom(30);
    }

    // 没开始的房间都可以关闭
    for (auto& elem : tables_)
    {
        auto table = elem.second;
        if (table->CheckShutdownTable())
        {
            table->ShutdownTable();
        }
    }
}

void CashRoom::VUserEnterRoomEvent(UserPtr user)
{
    Room::VUserEnterRoomEvent(user);

    // CashRoom进入房间就直接进入桌子
    auto table = GetRoomTable(user);
    if (!user->EnterTable(table))
    {
        ROOM_XLOG(WARNING) << "enter table failed. " << user << " target_table: " << table->GetTableID();
    }
}

void CashRoom::VUserLeaveTableEvent(uint64_t uid)
{
    Room::VUserLeaveTableEvent(uid);
}

bool CashRoom::VCheckStartGame(uint32_t tableid)
{
    if (!Room::VCheckStartGame(tableid))
    {
        return false;
    }

    if (IsServerStop())
    {
        return false;
    }
    return true;
}

void CashRoom::VGameStartFailEvent(uint32_t tableid)
{
    Room::VGameStartFailEvent(tableid);

    if (IsServerStop())
    {
        auto table = GetTable(tableid);
        if (table == nullptr)
        {
            ROOM_XLOG(ERROR) << "table is null. tableid: " << tableid;
            return;
        }

        LOG(WARNING) << "should not be here. tableid: " << tableid;
        table->ShutdownTable();
    }
}

bool CashRoom::CheckShutdownCashRoom() const
{
    for (auto& elem : tables_)
    {
        if (!elem.second->CheckShutdownTable())
        {
            return false;
        }
    }
    return true;
}

pb::RoomBrief CashRoom::VGetRoomBrief() const
{
    pb::RoomBrief brief = Room::VGetRoomBrief();
    brief.set_base_score(VGetRoomBaseScore());
    return brief;
}

bool CashRoom::ChangeTable(UserPtr user)
{
    if (user == nullptr)
    {
        return false;
    }

    if (!user->LeaveTable())
    {
        ROOM_XLOG(ERROR) << "leave table error. " << user; 
        return false;
    }

    // 进入新桌子
    auto table = GetRoomTable(user);
    if (!user->EnterTable(table))
    {
        ROOM_XLOG(ERROR) << "enter table failed. " << user << " target_table: " << table->GetTableID();
        return false;
    }

    // 坐下
    SitDown(user, -1);
    return true;
}

void CashRoom::SitDown(UserPtr user, int32_t seatid)
{
    uint64_t uid = user->uid();
    int64_t chips = VGetRoomBaseScore();

    auto func = [this, uid, seatid, chips](MessagePtr data) {
        std::string type = "rollback_cash_buyin";
        if (!ChangeRoomUserMoneyCallBack(data, uid, chips, type))
        {
            return;
        }

        auto msg = std::dynamic_pointer_cast<pb::iChangeValueRSP>(data);
        int64_t last_money = msg->value();

        auto user = GetUser(uid);
        if (user == nullptr)
        {
            ChangeRoomUserMoney(chips, uid, type, room_set_id_);
            ROOM_XLOG(INFO) << type << ", because user not in room room_set_id: " << room_set_id_ << " uid: " << uid << " chips: " << chips;
            return;
        }

        auto table = user->GetUserTable();
        if (table == nullptr)
        {
            ChangeRoomUserMoney(chips, uid, type, room_set_id_);
            ROOM_XLOG(INFO) << type << ", because user not in table room_set_id: " << room_set_id_ << " uid: " << uid << " chips: " << chips;
            return;
        }

        pb::SitDownRSP  rsp;
        rsp.set_roomid(GetRoomID());
        rsp.set_tableid(table->GetTableID());

        if (!user->SitDown(seatid, chips, last_money))
        {
            ChangeRoomUserMoney(chips, uid, "rollback_sitdown", room_set_id_);
            ROOM_XLOG(INFO) << "rollback_sitdown, beacause sitdown failed. room_set_id: " << room_set_id_ << " uid: " << uid << " chips: " << chips;
            rsp.set_code(-2);
            user->SendToUser(rsp);
            return;
        }

        rsp.set_code(0);
        rsp.mutable_table_brief()->CopyFrom(table->VGetTableBrief(user));
        user->SendToUser(rsp);
        ROOM_XLOG(INFO) << "user sit down. " << user << " chips: " << chips << " last_money: " << last_money;
    };

    ChangeRoomUserMoney(-chips, user->uid(), "cash_buyin", room_set_id_, func);
}
