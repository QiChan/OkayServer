#include "service_context.h"

ServiceContext::ServiceContext()
    : ctx_(nullptr),
      harborid_(0),
      pack_context_()
{
    harborid_ = atoi(skynet_getenv("harbor"));
}

void ServiceContext::InitServiceContext(skynet_context* ctx)
{
    ctx_ = ctx;
}

void ServiceContext::ServiceOverEvent()
{
    auto now_tick_us = TimeUtil::now_tick_us();
    for (auto& elem : workload_log_)
    {
        if (elem.second.pack_num() > 0)
        {
            WORKLOAD_MONITOR(
                    elem.first,
                    service_name(),
                    harborid(),
                    elem.second.GetAvgSpendUs(),
                    elem.second.GetTotalSpendUs(),
                    elem.second.GetMaxSpendUs(),
                    elem.second.pack_num()
                    );
            elem.second.reset(now_tick_us);
        }
    }

    for (auto& elem : upstream_traffic_log_)
    {
        if (elem.second.pack_num() > 0)
        {
            UPSTREAM_MONITOR(
                    elem.first,
                    elem.second.total_sz(),
                    elem.second.pack_num(),
                    service_name(),
                    harborid()
                    );
            elem.second.reset(now_tick_us);
        }
    }

    for (auto& elem : downstream_traffic_log_)
    {
        if (elem.second.pack_num() > 0)
        {
            DOWNSTREAM_MONITOR(
                    elem.first,
                    elem.second.total_sz(),
                    elem.second.pack_num(),
                    service_name(),
                    harborid()
                    );
            elem.second.reset(now_tick_us);
        }
    }
}

void ServiceContext::MonitorDownstreamTraffic(const std::string& type_name, uint32_t data_len, uint64_t now_tick_us)
{
    if (!proto_profile())
    {
        return;
    }
    auto& elem = downstream_traffic_log_[type_name];
    elem.StatTraffic(data_len);
    if (elem.CheckUpdateLog(now_tick_us))
    {
        DOWNSTREAM_MONITOR(
                type_name,
                elem.total_sz(),
                elem.pack_num(),
                service_name(),
                harborid()
                );
        elem.reset(now_tick_us);
    }
}

void ServiceContext::MonitorUpstreamTraffic(const std::string& type_name, uint32_t data_len, uint64_t now_tick_us)
{
    if (!proto_profile())
    {
        return;
    }
    auto& elem = upstream_traffic_log_[type_name];
    elem.StatTraffic(data_len);
    if (elem.CheckUpdateLog(now_tick_us))
    {
        UPSTREAM_MONITOR(
                type_name,    
                elem.total_sz(),
                elem.pack_num(),
                service_name(),
                harborid()
                );
        elem.reset(now_tick_us);
    }
}

void ServiceContext::MonitorWorkload(const std::string& type_name, int64_t spend_us, uint64_t now_tick_us)
{
    if (!proto_profile())
    {
        return;
    }
    if (spend_us < 0 || spend_us > 1000000000)
    {
        LOG(ERROR) << "type_name: " << type_name << "service_name: " << service_name() << " now_tick_us: " << now_tick_us << " receive_tick_us: " << pack_context_.receive_agent_tick_ << " spend_us: " << spend_us;
        return; 
    }
    auto& elem = workload_log_[type_name];
    elem.StatSpend(spend_us);
    if (elem.CheckUpdateLog(now_tick_us))
    {
        WORKLOAD_MONITOR(
                type_name,
                service_name(),
                harborid(),
                elem.GetAvgSpendUs(),
                elem.GetTotalSpendUs(),
                elem.GetMaxSpendUs(),
                elem.pack_num()
                );
        elem.reset(now_tick_us);
    }
}

int ServiceContext::WrapSkynetSend(struct skynet_context* ctx, uint32_t source, uint32_t destination, int type, int session, void* msg, size_t sz)
{
    if (destination == 0)
    {
        LOG(ERROR) << "[" << service_name() << "] destination is zero.";
        return -1;
    }

    int msg_type = type & PTYPE_MASK;
    if (msg_type == PTYPE_AGENT ||
            msg_type == PTYPE_CLIENT ||
            msg_type == PTYPE_SERVICE ||
            msg_type == PTYPE_RPC_CLIENT ||
            msg_type == PTYPE_RPC_SERVER ||
            msg_type == PTYPE_SYSTEM_SERVICE)
    {
        InPack pack;
        if (!pack.ParseFromInnerData(static_cast<const char*>(msg), sz))
        {
            LOG(ERROR) << "send pack error. type: " << type;
        }
        else
        {
            MonitorUpstreamTraffic(pack.type_name(), pack.data_len(), TimeUtil::now_tick_us());
        }
    }
    return skynet_send(ctx, source, destination, type, session, msg, sz);
}

int ServiceContext::WrapSkynetSocketSend(struct skynet_context* ctx, int id, const Message& msg, SocketType socket_type, uint32_t gate)
{
    char* data;
    uint32_t size;
    SerializeToNetData(msg, data, size);

    MonitorUpstreamTraffic(msg.GetTypeName(), size, TimeUtil::now_tick_us());

    return SkynetSocketSendPlus(ctx, id, data, size, socket_type, gate);
}

int ServiceContext::WrapSkynetSocketSend(struct skynet_context* ctx, int id, const void* inner_data, uint32_t sz, SocketType socket_type, uint32_t gate)
{
    InPack pack;
    if (!pack.ParseFromInnerData(static_cast<const char*>(inner_data), sz))
    {
        LOG(ERROR) << "send pack error";
    }
    else
    {
        MonitorUpstreamTraffic(pack.type_name(), pack.data_len(), TimeUtil::now_tick_us());
    }

    char* data = nullptr;
    uint32_t size = 0;
    InnerDataToNetData(static_cast<const char*>(inner_data), sz, data, size);
    return SkynetSocketSendPlus(ctx, id, data, size, socket_type, gate);
}

int ServiceContext::SkynetSocketSendPlus(struct skynet_context* ctx, int id, void* data, uint32_t size, SocketType socket_type, uint32_t gate)
{
    if (socket_type == SocketType::ROBOT_SOCKET)
    {
        return skynet_send(ctx, 0, gate, PTYPE_CLIENT | PTYPE_TAG_DONTCOPY, id, data, size);
    }
    else if (socket_type == SocketType::TCP_SOCKET)
    {
        return skynet_socket_send(ctx, id, data, size);
    }
    return -1;
}
