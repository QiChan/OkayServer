#include "room_robot.h"
#include "client.h"
    
RoomRobot::RoomRobot()
    : clubid_(0),
      roomid_(0)
{
}

RoomRobot::~RoomRobot()
{
}

void RoomRobot::VInitRobot(Client* client, TextParm& parm)
{
    Robot::VInitRobot(client, parm);

    clubid_ = parm.get_uint("clubid");

    RegisterCallBack();
}

void RoomRobot::LoginSuccessEvent()
{
    PeriodicallyGetClubRoom();
}

void RoomRobot::RegisterCallBack()
{
    client_->RegisterCallBack(pb::CreateRoomRSP::descriptor()->full_name(), std::bind(&RoomRobot::HandleMsgCreateRoomRSP, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::ClubRoomRSP::descriptor()->full_name(), std::bind(&RoomRobot::HandleMsgClubRoomRSP, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::EnterRoomRSP::descriptor()->full_name(), std::bind(&RoomRobot::HandleMsgEnterRoomRSP, this, std::placeholders::_1, std::placeholders::_2));
    client_->RegisterCallBack(pb::LeaveRoomRSP::descriptor()->full_name(), std::bind(&RoomRobot::HandleMsgLeaveRoomRSP, this, std::placeholders::_1, std::placeholders::_2));
}

void RoomRobot::CreateClubRoom()
{
    pb::CreateRoomREQ  req;
    req.set_clubid(clubid_);
    req.set_room_type(pb::OKEY_ROOM);

    std::string room_name = "room" + std::to_string(uid_);
    req.set_room_name(room_name);
    client_->ClientSend(req);
}

void RoomRobot::HandleMsgCreateRoomRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::CreateRoomRSP>(data);
    if (msg->code() < 0)
    {
        LOG(ERROR) << "create club room failed. uid: " << uid_ << " code: " << msg->code();
        return;
    }
    LOG(INFO) << "create room success. uid: " << uid_ << " roomid: " << msg->roomid();
}

void RoomRobot::HandleMsgClubRoomRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::ClubRoomRSP>(data);
    rooms_ = msg;

    if (rooms_->rooms().size() == 0)
    {
        CreateClubRoom();
    }
    else
    {
        if (roomid_ == 0)
        {
            EnterRoom(rooms_->rooms(0).roomid());
        }
        else
        {
            LeaveRoom();
        }
    }

    LOG(INFO) << "get club rooms. uid: " << uid_ << " clubid: " << clubid_ << " room_num: " << rooms_->rooms().size();
}

void RoomRobot::PeriodicallyGetClubRoom()
{
    pb::ClubRoomREQ req;
    req.set_clubid(clubid_);
    client_->ClientSend(req);

    client_->StartTimer(20 * 100, std::bind(&RoomRobot::PeriodicallyGetClubRoom, this));
}

void RoomRobot::EnterRoom(uint32_t roomid)
{
    if (roomid_ != 0 && roomid_ != roomid)
    {
        LOG(ERROR) << "already in room. uid: " << uid_ << " roomid_: " << roomid_ << " roomid: " << roomid;
        return;
    }

    pb::EnterRoomREQ    req;
    req.set_clubid(clubid_);
    req.set_roomid(roomid);
    client_->ClientSend(req);
}

void RoomRobot::LeaveRoom()
{
    if (roomid_ == 0)
    {
        LOG(ERROR) << "not in room. uid: " << uid_;
        return;
    }

    pb::LeaveRoomREQ    req;
    client_->ClientSend(req);
}

void RoomRobot::HandleMsgEnterRoomRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::EnterRoomRSP>(data);
    int32_t code = msg->code();
    uint32_t clubid = msg->clubid();
    uint32_t roomid = msg->roomid();
    if (code < 0)
    {
        LOG(WARNING) << "enter room failed. uid: " << uid_ << " clubid: " << clubid << " roomid: " << roomid << " code: " << code;
        return;
    }

    roomid_ = roomid;
    LOG(INFO) << "enter room success. uid: " << uid_ << " clubid: " << clubid << " roomid: " << roomid;
}

void RoomRobot::HandleMsgLeaveRoomRSP(MessagePtr data, uint64_t)
{
    auto msg = std::dynamic_pointer_cast<pb::LeaveRoomRSP>(data);
    if (msg->roomid() != roomid_ || msg->clubid() != clubid_)
    {
        LOG(ERROR) << "leave room error. uid: " << uid_ << " roomid: " << msg->roomid() << " clubid: " << msg->clubid() << " expected roomid: " << roomid_ << " expected clubid: " << clubid_;
        return; 
    }
    roomid_ = 0;
    LOG(INFO) << "leave room success. uid: " << uid_ << " roomid: " << msg->roomid() << " clubid: " << msg->clubid();
}
