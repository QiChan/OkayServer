#include "game_robot.h"
#include "client.h"

GameRobot::GameRobot()
    : clubid_(0),
      roomid_(0),
      tableid_(0),
      seatid_(-1),
      card_(0),
      can_eat_(false)
{
}

GameRobot::~GameRobot()
{
}

void GameRobot::VInitRobot(Client* client, TextParm& parm)
{
    Robot::VInitRobot(client, parm);

    clubid_ = parm.get_uint("clubid");
    roomid_ = parm.get_uint("roomid");

    RegisterCallBack();
}

void GameRobot::LoginSuccessEvent()
{
    EnterRoom();
}

void GameRobot::EnterRoom()
{
    pb::EnterRoomREQ    req;
    req.set_clubid(clubid_);
    req.set_roomid(roomid_);
    client_->ClientSend(req);
}

void GameRobot::SitDown()
{
    pb::SitDownREQ  req;
    req.set_seatid(-1);
    client_->ClientSend(req);
}

void GameRobot::TakeOrEatCard()
{
    client_->StartTimer(100, [this]{
        pb::ActionREQ   req;
        if (can_eat_)
        {
            // 吃牌
            req.set_type(pb::ACTION_EAT);
        }
        else
        {
            // 摸牌
            req.set_type(pb::ACTION_TAKE);
        }
        client_->ClientSend(req);
    });
}
void GameRobot::ThrowCard()
{
    client_->StartTimer(100, [this]{
        // 打牌
        pb::ActionREQ   req;
        req.set_type(pb::ACTION_THROW);
        req.add_cards1(card_);
        client_->ClientSend(req);
        card_ = 0;

        if (seatid_ == 0)
        {
            can_eat_ = true;
        }
    });
}

void GameRobot::RegisterCallBack()
{
    client_->RegisterCallBack(pb::EnterRoomRSP::descriptor()->full_name(), std::bind(&GameRobot::HandleMsgEnterRoomRSP, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::EnterTableRSP::descriptor()->full_name(), std::bind(&GameRobot::HandleMsgEnterTableRSP, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::SitDownRSP::descriptor()->full_name(), std::bind(&GameRobot::HandleMsgSitDownRSP, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::GameStartBRC::descriptor()->full_name(), std::bind(&GameRobot::HandleMsgGameStartBRC, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::ActionBRC::descriptor()->full_name(), std::bind(&GameRobot::HandleMsgActionBRC, this, std::placeholders::_1, std::placeholders::_2));
}

void GameRobot::HandleMsgEnterRoomRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::EnterRoomRSP>(data);

    if (msg->code() != 0)
    {
        LOG(ERROR) << "enter room failed. clubid: " << clubid_ << " roomid: " << roomid_ << " uid: " << uid_ << " code: " << msg->code();
        KillSelf();
        return;
    }

    LOG(INFO) << "enter room success. clubid: " << clubid_ << " roomid: " << roomid_ << " uid: " << uid_;
}

void GameRobot::HandleMsgEnterTableRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::EnterTableRSP>(data);
    LOG(INFO) << "enter table success. roomid: " << msg->roomid() << " tableid: " << msg->tableid() << " uid: " << uid_ << msg->ShortDebugString();
    tableid_ = msg->tableid();

    // 牌桌已经开始的情况
    playing_seats_.clear();
    auto table_brief = msg->mutable_table_brief();
    for (int i = 0; i < table_brief->seat_brief_size(); i++)
    {
        auto seat_brief = table_brief->seat_brief(i);
        if (seat_brief.user_brief().uid() == uid_)
        {
            seatid_ = seat_brief.seatid();
            LOG(INFO) << "have a seat. seatid: " << seatid_ << " uid: " << uid_;
        }

        if (seat_brief.playing())
        {
            playing_seats_.insert(seat_brief.seatid());
        }
    }

    if (seatid_ == -1)
    {
        SitDown();
    }
}

void GameRobot::HandleMsgSitDownRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::SitDownRSP>(data);
    if (msg->code() != 0)
    {
        LOG(INFO) << "sit down failed. clubid: " << clubid_ << " roomid: " << roomid_ << " uid: " << uid_ << " code: " << msg->code();
        return;
    }

    auto brief = msg->table_brief();
    for (int i = 0; i < brief.seat_brief_size(); i++)
    {
        auto seat_brief = brief.seat_brief(i);
        if (seat_brief.user_brief().uid() == uid_)
        {
            seatid_ = seat_brief.seatid();
        }
    }

    LOG(INFO) << "sit down success. clubid: " << clubid_ << " roomid: " << roomid_ << " uid: " << uid_ << " seatid: " << seatid_;
}

void GameRobot::HandleMsgGameStartBRC(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::GameStartBRC>(data);
    LOG(INFO) << "uid: " << uid_ << " " << msg->ShortDebugString();

    playing_seats_.clear();
    card_ = 0;
    auto brief = msg->table_brief();
    for (int i = 0; i < brief.seat_brief_size(); i++)
    {
        auto seat_brief = brief.seat_brief(i);
        if (seat_brief.playing())
        {
            playing_seats_.insert(seat_brief.seatid());
        }
    }

    int32_t dealer = msg->dealer_seatid();

    if (dealer == seatid_)
    {
        ThrowCard();
    }
}

void GameRobot::HandleMsgActionBRC(MessagePtr data, uint64_t)
{
    if (seatid_ == -1)
    {
        return;
    }

    auto msg = std::dynamic_pointer_cast<pb::ActionBRC>(data);
    int32_t action_seatid = msg->seatid();
    // 自己摸牌
    if (action_seatid == seatid_ && (msg->type() == pb::ACTION_TAKE || msg->type() == pb::ACTION_EAT))
    {
        card_ = msg->cards1(0);
        ThrowCard();
        return;
    }

    if (msg->type() == pb::ACTION_THROW)
    {
        // 判断是否到自己行动
        if (GetNextActionSeatID(action_seatid) == seatid_)
        {
            TakeOrEatCard();
        }
    }
}

int32_t GameRobot::GetNextActionSeatID(uint32_t curr_seatid)
{
    auto it = playing_seats_.begin();
    for (; it != playing_seats_.end(); ++it)
    {
        if (*it == curr_seatid)
        {
            break;
        }
    }

    if (it == playing_seats_.end())
    {
        LOG(ERROR) << "not found action seat. seatid: " << curr_seatid << " roomid: " << roomid_ << " uid: " << uid_;
    }

    auto next = ++it;
    if (next == playing_seats_.end())
    {
        return *playing_seats_.begin();
    }
    else
    {
        return *next;
    }
}
