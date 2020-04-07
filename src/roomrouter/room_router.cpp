#include "room_router.h"
#include "room_router_config.h"
#include <tools/text_parm.h>

extern "C"
{
#include "skynet.h"
}

RoomServer::RoomServer(uint32_t handle, const std::string& name)
    : handle_(handle),
      alive_time_(time(NULL)),
      name_(name)
{
}

RoomServer::~RoomServer()
{
}

RoomServer::RoomServer(RoomServer const& obj)
{
    handle_ = obj.handle_;
    alive_time_ = obj.alive_time_;
    name_ = obj.name_;
}

bool RoomServer::IsAlive() const
{
    time_t now = time(NULL);
    if (now - alive_time_ > 5)
    {
        LOG(INFO) << "room server has dead. name: " << GetName();
        return false;
    }
    return true;
}

void RoomServer::Live()
{
    alive_time_ = time(NULL);
}

bool RoomServer::Filter(const std::shared_ptr<pb::iCreateRoomREQ>& msg)
{
    RoomServerConfig* config = RoomRouterConfig::GetServerConfig(GetName());
    if (config == nullptr)
    {
        return false;
    }
 
    if (!IsAlive())
    {
        return false;
    }

    return config->Filter(msg);
}

const std::string& RoomServer::GetName() const
{
    return name_;
}


RoomRouter::RoomRouter()
{
    room_servers_it_ = room_servers_.begin();
}

RoomRouter::~RoomRouter()
{
}

bool RoomRouter::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    DependOnService("roomlist");

    RegServiceName("roomrouter", true, true);

    RegisterCallBack();

    if (!RoomRouterConfig::LoadConfig())
    {
        LOG(ERROR) << "load roomsvr_router config error.";
        return false;
    }

    return true;
}

bool RoomRouter::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    if (Service::VHandleTextMessage(from, data, source, session))
    {
        return true;
    }

    TextParm parm(data.c_str());
    if (from == "php")
    {
        const char* cmd = parm.get("cmd");
        std::string response = "no cmd\n";
        if (strcmp(cmd, "reload_router") == 0)
        {
            if (RoomRouterConfig::LoadConfig())
            {
                response = "reload router ok\n";
            }
            else
            {
                response = "reload router failed\n";
            }
        }

        ResponseTextMessage(from, response);
    }
    else
    {
        LOG(ERROR) << "from: " << from << " unknow text command: " << data;
    }

    return true;
}

void RoomRouter::VServiceStartEvent()
{
    LOG(INFO) << "[" << service_name() << "] start.";
}

void RoomRouter::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void RoomRouter::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}

void RoomRouter::RegisterCallBack()
{
    ServiceCallBack(pb::iCreateRoomREQ::descriptor()->full_name(), std::bind(&RoomRouter::HandleServiceCreateRoomREQ, this, std::placeholders::_1, std::placeholders::_2));
    ServiceCallBack(pb::iRegisterRoomServer::descriptor()->full_name(), std::bind(&RoomRouter::HandleServiceRegisterRoomServer, this, std::placeholders::_1, std::placeholders::_2));
    ServiceCallBack(pb::iRoomServerHeartBeat::descriptor()->full_name(), std::bind(&RoomRouter::HandleServiceRoomServerHeartBeat, this, std::placeholders::_1, std::placeholders::_2));
}

RoomServer* RoomRouter::RouteServer(const std::shared_ptr<pb::iCreateRoomREQ>& msg)
{
    size_t n = room_servers_.size();
    size_t i = 0;
    for (; i < n; ++i)
    {
        if (room_servers_it_ == room_servers_.end())
        {
            room_servers_it_ = room_servers_.begin();
            continue;
        }
        RoomServer* server = &room_servers_it_->second;
        room_servers_it_++;
        if (server != nullptr && server->Filter(msg))
        {
            return server;
        }
    }

    return nullptr;
}

void RoomRouter::HandleServiceCreateRoomREQ(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iCreateRoomREQ>(data);

    pb::iCreateRoomRSP  rsp;
    rsp.set_roomid(msg->roomid());
    rsp.set_uid(msg->uid());
    rsp.set_clubid(msg->req().clubid());
    rsp.set_room_name(msg->req().room_name());

    RoomServer* server = RouteServer(msg);
    if (server == nullptr)
    {
        LOG(WARNING) << "[" << service_name() << "] route failed. " << msg->ShortDebugString();
        rsp.set_code(-1);
        SendToService(rsp, ServiceHandle("roomlist"));
        return;
    }

    auto config = RoomRouterConfig::GetServerConfig(server->GetName());
    if (config == nullptr)
    {
        LOG(ERROR) << "[" << service_name() << "] room router config not exist. server_name: " << server->GetName();

        rsp.set_code(-1);
        SendToService(rsp, ServiceHandle("roomlist"));
        return;
    }
    msg->set_room_service_name(config->GetRoomName());

    SendToService(*msg, server->handle());
    LOG(INFO) << "[" << service_name() << "] create room. use " << server->GetName();
}

void RoomRouter::HandleServiceRegisterRoomServer(MessagePtr data, uint32_t source)
{
    auto msg = std::dynamic_pointer_cast<pb::iRegisterRoomServer>(data);
    if (!NewServer(msg->handle(), msg->name()))
    {
        LOG(ERROR) << "[" << service_name() << "] room server register failed. name: " << msg->name() << " handle: " << std::hex << msg->handle();
        return;
    }

    if (RoomRouterConfig::GetServerConfig(msg->name()) == nullptr)
    {
        LOG(WARNING) << "[" << service_name() << "] room server dont have router config. name: " << msg->name() << " handle: " << std::hex << msg->handle();
    }

    LOG(INFO) << "[" << service_name() << "] room server register. name: " << msg->name() << " handle: " << std::hex << msg->handle();
}

void RoomRouter::HandleServiceRoomServerHeartBeat(MessagePtr data, uint32_t source)
{
    auto msg = std::dynamic_pointer_cast<pb::iRoomServerHeartBeat>(data);
    RoomServer* server = GetServer(msg->name());
    if (server == nullptr)
    {
        return;
    }
    server->Live();
}

RoomServer* RoomRouter::GetServer(const std::string& name)
{
    auto it = room_servers_.find(name);
    if (it == room_servers_.end())
    {
        return nullptr;
    }
    return &it->second;
}

bool RoomRouter::NewServer(uint32_t handle, const std::string& name)
{
    RoomServer server(handle, name);
    auto result = room_servers_.emplace(name, RoomServer(handle, name));
    room_servers_it_ = room_servers_.begin();
    return result.second;
}
