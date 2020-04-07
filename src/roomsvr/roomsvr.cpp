#include "roomsvr.h"
#include "../room/cash_room.h"
#include <tools/text_filter.h>
#include <tools/real_rand.h>

extern "C"
{
#include "skynet_server.h"
}

RoomSvr::RoomSvr()
{
}

RoomSvr::~RoomSvr()
{
}

bool RoomSvr::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    DependOnService("roomrouter");
    DependOnService("roomlist");

    if (!RealRand::GetInstance()->Init())
    {
        return false;
    }

    std::string name = "room_server";
    name += std::to_string(service_context_.harborid());
    RegServiceName(name, true, false);

    RegisterCallBack();

    TextFilter::GetInstance()->LoadSensitiveWordsDict();

    return true;
}

void RoomSvr::VServiceStartEvent()
{
    pb::iRegisterRoomServer req;
    req.set_handle(skynet_context_handle(ctx()));
    req.set_name(service_name());
    SendToService(req, ServiceHandle("roomrouter"));

    PeriodicallyHeartBeat();


    // register room msg proto
    pb::iRegisterProto msg;
    msg.set_type(static_cast<uint32_t>(MsgServiceType::SERVICE_ROOM));
    msg.set_handle(0);
    {
        CashRoom room;
        room.RegisterCashRoomCallBack();
        for (auto& elem : room.GetClientMsgCallBacks())
        {
            msg.add_proto(elem.first); 
        }
    }
    SendToSystem(msg, main_);

    LOG(INFO) << "[" << service_name() << "] start.";
}

void RoomSvr::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void RoomSvr::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event";
}

void RoomSvr::RegisterCallBack()
{
    ServiceCallBack(pb::iCreateRoomREQ::descriptor()->full_name(), std::bind(&RoomSvr::HandleServiceCreateRoomREQ, this, std::placeholders::_1, std::placeholders::_2));
}

void RoomSvr::PeriodicallyHeartBeat()
{
    pb::iRoomServerHeartBeat hb;
    hb.set_name(service_name());
    SendToService(hb, ServiceHandle("roomrouter"));

    StartTimer(3 * 100, std::bind(&RoomSvr::PeriodicallyHeartBeat, this));
}

void RoomSvr::HandleServiceCreateRoomREQ(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iCreateRoomREQ>(data);

    pb::iCreateRoomRSP  rsp;
    rsp.set_roomid(msg->roomid());
    rsp.set_uid(msg->uid());
    rsp.set_clubid(msg->req().clubid());
    rsp.set_room_name(msg->req().room_name());

    OutPack pack;
    if (!pack.reset(*msg))
    {
        LOG(ERROR) << "[" << service_name() << "] parse error. " << msg->ShortDebugString();

        rsp.set_code(-1);
        SendToService(rsp, ServiceHandle("roomlist"));
        return;
    }

    char* parm = nullptr;
    uint32_t size = 0;
    pack.SerializeToNetData(parm, size);

    auto room_service = msg->room_service_name();
    if (room_service.empty())
    {
        room_service = "room";
    }

    uint32_t room_handle = NewChildService(room_service, std::string(parm, size));
    if (room_handle == 0)
    {
        rsp.set_code(-1);
    }
    else
    {
        rsp.set_code(0);
    }
    SendToService(rsp, ServiceHandle("roomlist"));

    LOG(INFO) << "[" << service_name() << "] create room. " << msg->ShortDebugString() << " handle: " << std::hex << room_handle;
}
