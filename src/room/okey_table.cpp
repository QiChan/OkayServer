#include "okey_table.h"
#include <glog/logging.h>
#include "room.h"

OkeyTable::OkeyTable(Room* room, uint32_t tableid, uint32_t seat_num)
    : Table(room, tableid, seat_num)
{
}

void OkeyTable::VDealCards()
{
    // 每人发14张牌
    for (uint32_t i = 0; i < 14; i++)
    {
        // 防止死循环
        uint32_t count = 0;

        PlayingSeatIterator iter(this);
        for (auto seat = *iter; seat != nullptr;)
        {
            if (count > seats_.size())
            {
                break;
            }
            count++;

            auto card = poker_->Pop();
            if (card == POKER_CARD_NULL)
            {
                ROOM_XLOG(ERROR) << "deal cards error. card is null, i: " << i;
                continue;
            }
            seat->AddCard(card);
            seat = *(++iter);
        }
    }

    // 庄位摸第15张牌
    auto seat = action_component_->GetActionSeat();
    seat->TakeCard(poker_->Pop());
    action_component_->NextAction(nullptr);
}

void OkeyTable::VSystemAction() 
{
    auto seat = action_component_->GetActionSeat();
    if (seat == nullptr)
    {
        ROOM_XLOG(ERROR) << "seat is nullptr";
        return;
    }

    ROOM_XLOG(INFO) << "system action. seatid: " << seat->GetSeatID() << " uid: " << seat->GetSeatUID();

    auto user = seat->GetSeatUser();
    if (user == nullptr)
    {
        ROOM_XLOG(ERROR) << "user is nullptr";
    }

    if (seat->IsIdleState())
    {
        // 摸牌
        UserTakeCard(user);
    }
    else if (seat->IsTakeState())
    {
        // 打牌
        auto req = std::make_shared<pb::ActionREQ>();
        req->set_type(pb::ACTION_THROW);
        req->add_cards1(POKER_CARD_NULL);
        UserThrowCard(user, req);
    }
    else
    {
        ROOM_XLOG(ERROR) << "unknow seat state. uid: " << seat->GetSeatUID() << " seatid: " << seat->GetSeatID() ;
    }
}

void OkeyTable::VHandleUserAction(std::shared_ptr<pb::ActionREQ> msg, RoomUserPtr user)
{
    auto sp_seat = user->GetUserSeat();
    auto action_seat = action_component_->GetActionSeat();
    if (sp_seat == nullptr || action_seat != sp_seat)
    {
        return;
    }

    ROOM_XLOG(INFO) << "user action: " << pb::ActionType_Name(msg->type()) << " uid: " << user->uid() << " seatid: " << sp_seat->GetSeatID();

    auto type = msg->type();
    switch (type)
    {
        case pb::ACTION_EAT:            // 吃
            {
                UserEatCard(user);
                break;
            }
        case pb::ACTION_TAKE:           // 摸
            {
                UserTakeCard(user);
                break;
            }
        case pb::ACTION_THROW:          // 打
            {
                UserThrowCard(user, msg);
                break;
            }
        case pb::ACTION_COMPLETE:       // 成
            {
                UserCompleteCard(user, msg);
                break;
            }
        default:
            {
                ROOM_XLOG(ERROR) << "unknown type: " << pb::ActionType_Name(type) << user;
                break;
            }
    }
}

void OkeyTable::UserEatCard(RoomUserPtr user)
{
    auto prev_seat = action_component_->GetPrevSeat();
    auto seat = action_component_->GetActionSeat();

    if (prev_seat == nullptr)
    {
        ROOM_XLOG(ERROR) << "prev_seat is nullptr. " << seat;
        return;
    }

    if (!seat->IsIdleState())
    {
        return;
    }

    PokerCard card = POKER_CARD_NULL;
    if (!seat->EatCard(prev_seat, card))
    {
        return;
    }

    pb::ActionBRC   brc;
    brc.set_seatid(seat->GetSeatID());
    brc.set_type(pb::ACTION_EAT);
    brc.add_cards1(card);
    BroadcastToUser(brc);

    action_component_->NextAction(seat);

    ROOM_XLOG(INFO) << "eat card: " << card << " uid: " << seat->GetSeatUID() << " seatid: " << seat->GetSeatID();
}

void OkeyTable::UserTakeCard(RoomUserPtr user)
{
    auto seat = action_component_->GetActionSeat();
    if (!seat->IsIdleState())
    {
        return;
    }

    PokerCard card = poker_->Pop();
    if (card == POKER_CARD_NULL || !seat->TakeCard(card))
    {
        ROOM_XLOG(ERROR) << "take card failed. " << seat << " card: " << card;
        return;
    }

    pb::ActionBRC   brc;
    brc.set_seatid(seat->GetSeatID());
    brc.set_type(pb::ACTION_TAKE);
    BroadcastToUser(brc, user);

    brc.add_cards1(card);
    user->SendToUser(brc);

    action_component_->NextAction(seat);

    ROOM_XLOG(INFO) << "take card: " << card << " uid: " << seat->GetSeatUID() << " seatid: " << seat->GetSeatID();
}

void OkeyTable::UserThrowCard(RoomUserPtr user, std::shared_ptr<pb::ActionREQ> msg)
{
    if (msg->cards1_size() <= 0)
    {
        return;
    }
    PokerCard card = msg->cards1(0);

    auto seat = action_component_->GetActionSeat();
    if (!seat->IsTakeState())
    {
        return;
    }

    if (!seat->ThrowCard(card))
    {
        return;
    }

    pb::ActionBRC   brc;
    brc.set_seatid(seat->GetSeatID());
    brc.set_type(pb::ACTION_THROW);
    brc.add_cards1(card);
    BroadcastToUser(brc);

    ROOM_XLOG(INFO) << "throw_card: " << card << " uid: " << seat->GetSeatUID() << " seatid: " << seat->GetSeatID();

    // 牌摸光了平局
    if (poker_->GetLastCardNum() == 0)
    {
        GameOver(nullptr, nullptr);
    }
    else
    {
        action_component_->NextAction(seat);
    }
}

void OkeyTable::UserCompleteCard(RoomUserPtr user, std::shared_ptr<pb::ActionREQ> msg)
{
    auto seat = action_component_->GetActionSeat();
    if (!seat->IsTakeState())
    {
        return;
    }

    if (msg->cards1_size() <= 0)
    {
        return;
    }

    bool has_flush_pair = false;        // 有同色对
    bool has_flush_set = false;         // 用同色顺

    auto hand_cards = seat->GetHandCards();

    {
        PokerCard discard_card = msg->cards1(0);            // 成牌时需要丢弃的那张牌
        auto it = hand_cards.find(discard_card);
        if (it == hand_cards.end())
        {
            return;
        }
        hand_cards.erase(it);
    }


    for (int i = 0; i < msg->cards2_size(); i++)
    {
        auto group = msg->cards2(i);
        std::vector<PokerCard>  cards;
        for (int j = 0; j < group.cards_size(); j++)
        {
            auto card = group.cards(j);

            auto it = hand_cards.find(card);
            if (it == hand_cards.end())
            {
                return;
            }
            hand_cards.erase(it);

            cards.push_back(card);
        }

        // 没有同色顺的时候才能用同色对
        if (!has_flush_set && poker_->VIsFlushPairs(cards))
        {
            has_flush_pair = true;
            continue;
        }

        // 没有同色对的时候才能采用同色顺和三条、四条
        if (has_flush_pair)
        {
            // 成牌失败
            return;
        }

        if (!poker_->VIsTripsOrQuads(cards) && !poker_->VIsFlushStraight(cards))
        {
            // 成牌失败
            return;
        }
        has_flush_set = true;
    }

    // 成牌成功
    if (hand_cards.empty())
    {
        ROOM_XLOG(INFO) << "complete_card: [" << msg->ShortDebugString() << "] uid: " << seat->GetSeatUID() << " seatid: " << seat->GetSeatID();
        GameOver(seat, msg->mutable_cards2());
    }
}

void OkeyTable::VStatGameRecord(SeatPtr winner, RepeatedPtrField<pb::CardGroup>* cards_ptr, pb::iGameRecord& record, std::vector<SeatPtr>& playing_seats, int64_t& win_chips)
{
    std::stringstream win_log;
    win_log << "game over. gameid: " << game_id_ << " playing_user: [" << PlayingSeatIterator(this) << "] ";

    int32_t     win_seatid = -1;
    int64_t     lose_chips = -base_score_;
    int64_t     total_rake = 0;

    if (winner == nullptr)
    {
        lose_chips = 0;
    }

    int64_t     winner_rake = 0;            // 记录赢家自己的抽水
    PlayingSeatIterator iter(this);
    for (auto seat = *iter; seat != nullptr; seat = *(++iter))
    {
        int64_t seat_lost_chips = seat->GameOver(lose_chips);
        playing_seats.emplace_back(seat);
        int64_t rake = CalculateRake(seat_lost_chips);
        total_rake += rake;
        win_chips += (seat_lost_chips - rake);

        if (seat == winner)
        {
            // 记录赢家的抽水, 赢家需要特殊处理
            winner_rake = rake;
            continue;
        }

        auto seat_user = seat->GetSeatUser();
        if (seat_user == nullptr)
        {
            ROOM_XLOG(ERROR) << "seat_user is nullptr. lost_chips: " << seat_lost_chips << " rake: " << rake << seat; 
            continue;
        }

        // 负数表示输的筹码
        record.add_user_records()->CopyFrom(GetGameUserRecord(seat_user, rake, -seat_lost_chips));
    }

    if (winner != nullptr)
    {
        winner->GameOver(win_chips);
        win_seatid = winner->GetSeatID();

        win_log << "card_group: [";
        for (auto begin = cards_ptr->begin(); begin != cards_ptr->end(); ++begin)
        {
            win_log << begin->ShortDebugString() << " ";
        }
        win_log << "] ";

        ROOM_XLOG(INFO) << win_log.str() << " winner: [" << winner << "] win_chips: " << win_chips << " total_rake: " << total_rake;

        auto winner_user = winner->GetSeatUser();
        if (winner_user == nullptr)
        {
            ROOM_XLOG(ERROR) << "winner_user is nullptr. win_chips: " << win_chips << " rake: " << winner_rake;
        }
        else
        {
            record.add_user_records()->CopyFrom(GetGameUserRecord(winner_user, winner_rake, win_chips));
        }
    }
    else
    {
        ROOM_XLOG(INFO) << win_log.str() << " game is draw";
    }

    // 注意winner可能为nullptr
    pb::WinnerInfoBRC   brc;
    brc.set_roomid(GetRoomID());
    brc.set_tableid(GetTableID());
    brc.set_seatid(win_seatid);
    brc.set_chips(win_chips);

    if (cards_ptr != nullptr)
    {
        brc.mutable_cards()->CopyFrom(*cards_ptr);
    }

    BroadcastToUser(brc);
}

OkeyTable101::OkeyTable101(Room* room, uint32_t tableid, uint32_t seat_num)
    : Table(room, tableid, seat_num)
{
}

void OkeyTable101::VDealCards()
{
    // TODO:
    ROOM_XLOG(INFO) << "OkeyTable101: deal cards";
}

void OkeyTable101::VSystemAction()
{
    // TODO:
    ROOM_XLOG(INFO) << "OkeyTable101: system action";
}

void OkeyTable101::VStatGameRecord(SeatPtr winner, RepeatedPtrField<pb::CardGroup>* cards_ptr, pb::iGameRecord& record, std::vector<SeatPtr>& playing_seats, int64_t& win_chips)
{
    // TODO:
    ROOM_XLOG(INFO) << "OkeyTable101: game overevent";
}

void OkeyTable101::VHandleUserAction(std::shared_ptr<pb::ActionREQ> msg, RoomUserPtr user)
{
    // TODO:
    ROOM_XLOG(INFO) << "OkeyTable101: handle user action";
}
