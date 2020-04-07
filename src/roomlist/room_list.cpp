#include "room_list.h"
#include "../pb/login.pb.h"

RoomList::RoomList()
    : MsgService(MsgServiceType::SERVICE_ROOMLIST),
      rd_(time(NULL))
{
}

RoomList::~RoomList()
{
}

bool RoomList::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!MsgService::VInitService(ctx, parm, len))
    {
        return false;
    }

    DependOnService("roomrouter");
    DependOnService("dbhelper");

    RegServiceName("roomlist", true, true);

    RegisterCallBack();

    MonitorSelf();

    return true;
}

void RoomList::VReloadUserInfo(UserPtr user)
{
    LoadUserInfo(user->uid());
}

void RoomList::LoadUserInfo(uint64_t uid)
{
    pb::iLoadRoomListUserREQ    req;
    req.set_uid(uid);
    CallRPC(ServiceHandle("dbhelper"), req, [this](MessagePtr data) {
        auto msg = std::dynamic_pointer_cast<pb::iLoadRoomListUserRSP>(data);
        uint64_t uid = msg->info().uid();
        auto icon = msg->info().icon();
        auto name = msg->info().name();
        auto user = GetUser(uid);
        if (user == nullptr)
        {
            return;
        }
        user->CompleteRoomListUser(name, icon);
    });
}

void RoomList::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void RoomList::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}

void RoomList::RegisterCallBack()
{
    MsgCallBack(pb::UserInfoREQ::descriptor()->full_name(), std::bind(&RoomList::HandleClientMsgUserInfoREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::CreateRoomREQ::descriptor()->full_name(), std::bind(&RoomList::HandleClientMsgCreateRoomREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::ClubRoomREQ::descriptor()->full_name(), std::bind(&RoomList::HandleClientMsgClubRoomREQ, this, std::placeholders::_1, std::placeholders::_2));
    MsgCallBack(pb::EnterRoomREQ::descriptor()->full_name(), std::bind(&RoomList::HandleClientMsgEnterRoomREQ, this, std::placeholders::_1, std::placeholders::_2));

    ServiceCallBack(pb::iRoomBriefReport::descriptor()->full_name(), std::bind(&RoomList::HandleServiceRoomBriefReport, this, std::placeholders::_1, std::placeholders::_2));
    ServiceCallBack(pb::iRoomOver::descriptor()->full_name(), std::bind(&RoomList::HandleServiceRoomOver, this, std::placeholders::_1, std::placeholders::_2));
    ServiceCallBack(pb::iCreateRoomRSP::descriptor()->full_name(), std::bind(&RoomList::HandleServiceCreateRoomRSP, this, std::placeholders::_1, std::placeholders::_2));

    RegisterRPC(pb::iGetClubRoomNumREQ::descriptor()->full_name(), std::bind(&RoomList::HandleRPCGetClubRoomNumREQ, this, std::placeholders::_1));
}

void RoomList::HandleServiceRoomBriefReport(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iRoomBriefReport>(data);
    auto room_brief = msg->room();
    uint32_t room_handle = msg->handle();
    uint32_t clubid = room_brief.clubid();
    uint32_t roomid = room_brief.roomid();

    auto& club_room = club_rooms_[clubid];

    club_room.emplace(roomid);
    all_rooms_[roomid] = std::make_shared<RoomInfo>(room_handle, room_brief);
}

void RoomList::HandleServiceRoomOver(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iRoomOver>(data);
    auto clubid = msg->clubid();
    auto roomid = msg->roomid();

    auto club_room_it = club_rooms_.find(clubid);
    if (club_room_it == club_rooms_.end())
    {
        LOG(WARNING) << "room over. but cannot find room. clubid: " << clubid << " roomid: " << roomid;
        return;
    }
    auto& club_room = club_room_it->second;

    club_room.erase(roomid);
    all_rooms_.erase(roomid);
}

void RoomList::HandleServiceCreateRoomRSP(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iCreateRoomRSP>(data);
    auto roomid = msg->roomid();
    auto uid = msg->uid();
    auto clubid = msg->clubid();
    auto code = msg->code();
    auto room_name = msg->room_name();

    if (code < 0)
    {
        all_rooms_.erase(roomid);
    }

    UserPtr user = GetUser(uid);
    if (user == nullptr)
    {
        return;
    }

    pb::CreateRoomRSP   rsp;
    rsp.set_roomid(roomid);
    rsp.set_room_name(room_name);
    rsp.set_clubid(clubid);
    rsp.set_code(code);
    user->SendToUser(rsp);
}

void RoomList::HandleClientMsgCreateRoomREQ(MessagePtr data, UserPtr user)
{
    if (IsServerStop())
    {
        return;
    }
    auto msg = std::dynamic_pointer_cast<pb::CreateRoomREQ>(data);
    if (!CheckCreateRoomREQ(msg))
    {
        LOG(INFO) << "check create room failed. " << msg->ShortDebugString();
        return;
    }

    // TODO: check clubid
    pb::CreateRoomRSP   rsp;
    rsp.set_room_name(msg->room_name());
    rsp.set_clubid(msg->clubid());

    uint32_t roomid = GenRoomID();
    if (roomid == 0)
    {
        LOG(ERROR) << "[" << service_name() << "] gen roomid error";
        rsp.set_code(-1);
        user->SendToUser(rsp);
        return;
    }

    all_rooms_[roomid] = std::make_shared<RoomInfo>();

    pb::iCreateRoomREQ  req;
    req.set_roomid(roomid);
    req.set_uid(user->uid());
    req.mutable_req()->CopyFrom(*msg);
    SendToService(req, ServiceHandle("roomrouter"));
}

void RoomList::HandleClientMsgUserInfoREQ(MessagePtr data, UserPtr user)
{
    pb::UserInfoRSP rsp;
    rsp.set_name(user->name());
    rsp.set_icon(user->icon());
    user->SendToUser(rsp);
}

void RoomList::HandleClientMsgClubRoomREQ(MessagePtr data, UserPtr user)
{
    auto msg = std::dynamic_pointer_cast<pb::ClubRoomREQ>(data);
    uint32_t clubid = msg->clubid();

    pb::ClubRoomRSP rsp;
    rsp.set_clubid(clubid);

    auto club_room_it = club_rooms_.find(clubid);
    if (club_room_it != club_rooms_.end())
    {
        for (auto roomid : club_room_it->second)
        {
            auto room = GetRoomInfo(roomid);
            if (room == nullptr)
            {
                continue;
            }
            auto room_brief = rsp.add_rooms();
            room_brief->CopyFrom(room->GetRoomBrief());
        }
    }

    user->SendToUser(rsp);
}

void RoomList::HandleClientMsgEnterRoomREQ(MessagePtr data, UserPtr user)
{
    if (IsServerStop())
    {
        return;
    }

    /*
     * TODO:
     * 1. gps
     * 2. check club
     */
    auto msg = std::dynamic_pointer_cast<pb::EnterRoomREQ>(data);
    uint32_t clubid = msg->clubid();
    uint32_t roomid = msg->roomid();

    pb::EnterRoomRSP    rsp;
    rsp.set_clubid(clubid);
    rsp.set_roomid(roomid);

    auto room = GetClubRoomInfo(clubid, roomid);
    if (room == nullptr)
    {
        rsp.set_code(-1);
        user->SendToUser(rsp);
        return;
    }

    if (clubid != room->GetRoomBrief().clubid())
    {
        rsp.set_code(-2);
        user->SendToUser(rsp);
        return;
    }

    pb::iEnterRoom  req;
    req.set_clubid(clubid);
    req.set_uid(user->uid());
    req.set_handle(user->GetHandle());
    req.set_ip(user->GetIP());
    SendToService(req, room->GetRoomHandle());
}

MessagePtr RoomList::HandleRPCGetClubRoomNumREQ(MessagePtr data)
{
    auto msg = std::dynamic_pointer_cast<pb::iGetClubRoomNumREQ>(data);
    auto rsp = std::make_shared<pb::iGetClubRoomNumRSP>();

    for (int i = 0; i < msg->clubids_size(); i++)
    {
        auto clubid = msg->clubids(i);
        uint32_t room_num = GetClubRoomNum(clubid);

        auto item = rsp->add_items();
        item->set_clubid(clubid);
        item->set_num(room_num);
    }

    return rsp;
}

uint32_t RoomList::GenRoomID()
{
    uint32_t roomid = 0;
    for (int count = 0; count < 1000; ++count)
    {
        std::uniform_int_distribution<uint32_t> u(100000, 999999);
        uint32_t temp_id = u(rd_);

        if (all_rooms_.find(temp_id) == all_rooms_.end())
        {
            roomid = temp_id;
            break;
        }
    }

    return roomid;
}

RoomList::RoomInfoPtr RoomList::GetRoomInfo(uint32_t roomid)
{
    auto it = all_rooms_.find(roomid);
    if (it == all_rooms_.end())
    {
        return nullptr;
    }

    if (!it->second->IsAlive())
    {
        return nullptr;
    }

    return it->second;
}

uint32_t RoomList::GetClubRoomNum(uint32_t clubid) const
{
    auto club_room_it = club_rooms_.find(clubid);
    if (club_room_it == club_rooms_.end())
    {
        return 0;
    }
    return club_room_it->second.size();
}

RoomList::RoomInfoPtr RoomList::GetClubRoomInfo(uint32_t clubid, uint32_t roomid)
{
    auto club_room_it = club_rooms_.find(clubid);
    if (club_room_it == club_rooms_.end())
    {
        return nullptr;
    }
    
    if (club_room_it->second.find(roomid) == club_room_it->second.end())
    {
        return nullptr;
    }

    return GetRoomInfo(roomid);
}

bool RoomList::CheckCreateRoomREQ(const std::shared_ptr<pb::CreateRoomREQ>& msg)
{
    auto action_time = msg->action_time();
    if (action_time < 20 || action_time > 50)
    {
        return false;
    }

    uint32_t rake = msg->rake();
    if (rake != 1 && rake != 2 && rake != 3 && rake != 5 && rake != 10)
    {
        return false;
    }

    auto room_mode = msg->room_mode();

    // club cash
    if (room_mode == pb::CLUB_CASH_ROOM)
    {
        int64_t base_score = msg->base_score();
        if (base_score < 100)
        {
            return false;
        }
        if (base_score > 1000 && rake >= 10)
        {
            return false;
        }

        if (base_score >= 100000000)
        {
            return false;
        }

        return true;
    }

    return false;
}

void RoomList::MonitorSelf()
{
    LOG(INFO) << "[" << service_name() << "] ======RoomList Self Monitor======"
        << " all_rooms size: " << all_rooms_.size();
    StartTimer(60 * 100, std::bind(&RoomList::MonitorSelf, this));
}
