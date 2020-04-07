#include "agent.h"
#include "../watchdog/watchdog.h"
#include <tools/string_util.h>
#include "../common/scope_guard.h"
#include "../pb/agent.pb.h"
#include "../pb/login.pb.h"
#include "../pb/inner.pb.h"

extern "C"
{
#include "skynet_server.h"
}

Agent::Agent()
    : client_msg_dsp_(&service_context_),
      uid_(0),
      create_timestamp_(0),
      network_subsystem_(this),
      curr_roomid_(0)
{
}

Agent::~Agent()
{
    LOG(INFO) << "agent release. uid: " << uid();
}

bool Agent::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    int fd;
    uint32_t gate;
    uint32_t dog;
    uint32_t socket_type;
    int port;
    char ip_port[64] = {0};
    char realip[64] = {0};
    sscanf((const char*)parm, "%d %u %u %u %ld %63s %63s", &fd, &socket_type, &gate, &dog, &uid_, ip_port, realip);
    std::string strport;
    std::string ip = realip;
    if (ip.empty())
    {
        std::tie(ip, strport) = StringUtil::DivideString(ip_port, ':');
    }
    else
    {
        std::tie(std::ignore, strport) = StringUtil::DivideString(ip_port, ':');
    }
    port = atoi(strport.c_str());


    ScopeGuard  pack_guard(
        [this]
        {
            service_context_.reset_pack_context(uid(), TimeUtil::now_tick_us());
        },
        [this]
        {
            service_context_.clear_pack_context();    
        }        
    );

    network_subsystem_.Init(static_cast<SocketType>(socket_type), fd, gate, dog, ip, port);

    create_timestamp_ = time(NULL);

    RegServiceName("agent", false, false);

    RegisterCallBack();
    return true;
}

void Agent::MsgCallBack(const std::string& name, const typename Dispatcher<uint64_t>::CallBack& func)
{
    client_msg_dsp_.RegisterCallBack(name, func);
}

void Agent::RegisterCallBack()
{
    RegisterPTypeParsePackCTXCallBack(PTYPE_CLIENT, [this](const char*) { service_context_.reset_pack_context(uid(), TimeUtil::now_tick_us()); });

    filters_[pb::EnterRoomREQ::descriptor()->full_name()] = std::bind(&Agent::FilterEnterRoomREQ, this, std::placeholders::_1);

    ServiceCallBack(pb::iAgentRoomStatus::descriptor()->full_name(), std::bind(&Agent::HandleServiceAgentRoomStatus, this, std::placeholders::_1, std::placeholders::_2));

    MsgCallBack(pb::UserRoomREQ::descriptor()->full_name(), [this](MessagePtr, uint64_t){
        pb::UserRoomRSP rsp;
        rsp.set_roomid(curr_roomid_);
        SendToUser(rsp);
    });
}

void Agent::VServiceStartEvent()
{
    network_subsystem_.ServiceStartEvent();
    LOG(INFO) << "[" << service_name() << "] start. uid: " << uid();
}

void Agent::VServiceOverEvent()
{
    network_subsystem_.ServiceOverEvent();
    LOG(INFO) << "[" << service_name() << "] shutdown. uid: " << uid();
}

void Agent::VServerStopEvent()
{
    pb::ServerStopBRC   brc;
    SendToUser(brc);

    // 停服时不在房间的玩家直接踢下线
    if (rooms_.empty())
    {
        network_subsystem_.ShutdownConnect();
    }
}

void Agent::BroadcastToService(const Message& msg)
{
    for (auto& elem : g_watchdog->msg_services_)
    {
        if (elem.second == 0)
        {
            continue;
        }
        SendToService(msg, elem.second);
    }

    for (auto& room : rooms_)
    {
        SendToService(msg, room.second->room_handle_);
    }
}

void Agent::SendToUser(const Message& msg)
{
    network_subsystem_.SendToUser(msg);
}

void Agent::SendToUser(void* inner_data, uint32_t size)
{
    network_subsystem_.SendToUser(inner_data, size);
}

bool Agent::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    ScopeGuard  pack_guard(
        [this]
        {
            service_context_.reset_pack_context(uid(), TimeUtil::now_tick_us());
        },
        [this]
        {
            service_context_.clear_pack_context();    
        }        
    );

    if (from == "dog")
    {
        std::string cmd;
        std::string it;
        std::tie(cmd, it) = StringUtil::DivideString(data, ' ');
        if (cmd == "client_disconnect")                 // 连接断开
        {
            network_subsystem_.HandleClientDisconnect();
        }
        else if (cmd == "client_rebind")                // 重连
        {
            int fd;
            char ip_port[64] = { 0 };
            char realip[64] = { 0 };
            uint32_t socket_type;
            sscanf(it.c_str(), "%d %u %63s %63s", &fd, &socket_type, ip_port, realip);
            std::string ip, port;
            std::tie(ip, port) = StringUtil::DivideString(ip_port, ':');
            network_subsystem_.HandleRebind(fd, static_cast<SocketType>(socket_type), ip, atoi(port.c_str()), realip);
        }
        else if (cmd == "client_release")               // 销毁服务
        {
            ShutdownService();
        }
    }

    return true;
}

void Agent::VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session)
{
    if (!IsServiceStart())
    {
        LOG(WARNING) << "agent get msg, but service not start. uid: " << uid();
        return;
    }

    InPack  pack;
    if (!pack.ParseFromNetData(service_context_.pack_context(), data, size))
    {
        network_subsystem_.ShutdownConnect();
        LOG(INFO) << "parse pack error. uid: " << uid();
        return;
    }

    auto rate_limiter_result = rate_limiter_.take(pack.type_name(), current_pack_context().receive_agent_tick_, g_watchdog->GetBucketCap(), g_watchdog->GetRateLimit());
    if (!std::get<0>(rate_limiter_result))
    {
        LOG(INFO) << "agent frequency exceed limit. uid: " << uid() << " type: " << pack.type_name() << " rate: " << g_watchdog->GetRateLimit() << " bucket_cap: " << g_watchdog->GetBucketCap() << " water: " << std::get<1>(rate_limiter_result) << " timestamp: " << current_pack_context().receive_time_ << "us";

        if (g_watchdog->GetRateLimitSwitch())
        {
            return;
        }
    }

    if (!FilterClientMsg(pack))
    {
        return;
    }

    DispatcherStatus status = client_msg_dsp_.DispatchClientMessage(data, size, uid());
    if (status == DispatcherStatus::DISPATCHER_SUCCESS)
    {
        return;
    }

    if (status != DispatcherStatus::DISPATCHER_CALLBACK_ERROR)
    {
        LOG(INFO) << "dispatch error. status:" << static_cast<uint16_t>(status) << " uid:" << uid();
        network_subsystem_.ShutdownConnect();
        return;
    }


    service_context_.MonitorDownstreamTraffic(pack.type_name(), pack.data_len(), current_pack_context().receive_agent_tick_);

    uint32_t dest = 0;
    auto type = g_watchdog->AgentRouteType(pack.type_name());
    if (type == MsgServiceType::SERVICE_NULL)
    {
        network_subsystem_.ShutdownConnect();
        LOG(INFO) << "route error. prototype:" << pack.type_name() << " uid: " << uid();
        return;
    }
    else if (type == MsgServiceType::SERVICE_ROOM)
    {
        if (rooms_.empty())
        {
            // 这里还是不踢下线比较好
//          network_subsystem_.ShutdownConnect();
            LOG(INFO) << "not in room. type:" << pack.type_name() << " uid: " << uid();
            return;
        }

        // TODO: 暂时不支持多桌，只发送给其中一个房间
        auto room = GetRoom(curr_roomid_);
        if (room == nullptr)
        {
            LOG(ERROR) << "room is nullptr. roomid: " << curr_roomid_ << " uid: " << uid() << " type: " << pack.type_name();
            return;
        }

        dest = room->room_handle_;
    }
    else
    {
        dest = g_watchdog->AgentRouteDest(type);
    }
    ClientForward(data, size, dest);
}

void Agent::VHandleForwardMsg(const char* data, uint32_t size, uint32_t handle, int session)
{
    InPack pack;
    if (!pack.ParseFromInnerData(data, size))
    {
        LOG(ERROR) << "forward pack error.";
    }
    else
    {
        uint64_t now_tick_us = TimeUtil::now_tick_us();
        // agent不统计其它服务发给客户端的入口流量
        
        // 统计负载, 转发给客户端的负载只统计自己发出的
        if (pack.pack_context().process_uid_ == uid() && pack.pack_context().harborid() == service_context_.harborid())
        {
            if (pack.pack_context().receive_agent_tick_ == 0)
            {
                LOG(ERROR) << "receive_agent_tick is zero. uid: " << uid() << " type_name: " << pack.type_name() << " receive_time: " << pack.pack_context().receive_time_;
            }
            else
            {
                service_context_.MonitorWorkload(pack.type_name(), now_tick_us - current_pack_context().receive_agent_tick_, now_tick_us);
            }
        }
    }

    SendToUser((void* )data, size);
}

void Agent::ClientForward(const char* data, uint32_t size, uint32_t dest)
{
    if (dest == 0)
    {
        return;
    }
    uint32_t sz = 0;
    char* pack_data = nullptr;
    NetDataToInnerData(data, size, current_pack_context(), pack_data, sz);

    // agent不统计转发消息给其它服务的出口流量
	skynet_send(ctx(), 0, dest, PTYPE_CLIENT | PTYPE_TAG_DONTCOPY, 0, pack_data, sz);
}

void Agent::HandleServiceAgentRoomStatus(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iAgentRoomStatus>(data);
    uint32_t roomid = msg->roomid();
    uint32_t room_handle = msg->handle();
    uint32_t clubid = msg->clubid();

    if (msg->enter())
    {
        auto room = GetRoom(roomid);
        // 进房间
        if (!rooms_.empty() && room == nullptr)   
        {
            // 出现了多桌
            LOG(WARNING) << "user enter multiple rooms. uid: " << uid() << " roomid: " << roomid << " currrent rooms_size: " << rooms_.size();
        }

        rooms_[roomid] = std::make_shared<RoomInfo>(roomid, room_handle, clubid);
        // TODO: 不支持多桌
        curr_roomid_ = roomid;
    }
    else
    {
        // 出房间
        auto room = GetRoom(roomid);
        if (room == nullptr)
        {
            LOG(ERROR) << "user out room error, not in room. uid: " << uid() << " roomid: " << roomid;        
        } 
        else
        {
            rooms_.erase(roomid);
        }
        curr_roomid_ = 0;

        // 停服后，离开所有房间直接踢下线
        if (rooms_.empty() && IsServerStop())
        {
            network_subsystem_.ShutdownConnect();
        }
    }
}

std::shared_ptr<Agent::RoomInfo>   Agent::GetRoom(uint32_t roomid)
{
    auto it = rooms_.find(roomid);
    if (it == rooms_.end())
    {
        return nullptr;
    }
    return it->second;
}

bool Agent::FilterEnterRoomREQ(MessagePtr data)
{
    if (rooms_.empty())
    {
        return true;
    }

    auto msg = std::dynamic_pointer_cast<pb::EnterRoomREQ>(data);
    uint32_t roomid = msg->roomid();
    uint32_t clubid = msg->clubid();

    pb::EnterRoomRSP    rsp;
    rsp.set_clubid(clubid);
    rsp.set_roomid(roomid);
    
    auto room = GetRoom(roomid);
    if (room == nullptr)
    {
        // 出现了多桌
        LOG(WARNING) << "user enter multiple rooms. uid: " << uid() << " roomid: " << roomid << " currrent rooms_size: " << rooms_.size();
        return true;
//        rsp.set_code(-3);
//        SendToUser(rsp);
//        return false;
    }

    if (room->clubid_ != clubid)
    {
        rsp.set_code(-2);
        SendToUser(rsp);
        return false;
    }

    return true;
}

bool Agent::FilterClientMsg(const InPack& pack)
{
    auto it = filters_.find(pack.type_name());
    if (it == filters_.end())
    {
        return true;
    }

    auto msg = pack.CreateMessage();
    if (msg == nullptr)
    {
        LOG(ERROR) << "dispatch message pb error. type: " << pack.type_name();
        return false;
    }

    return it->second(msg);
}
