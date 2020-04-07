#include "rpc.h"
#include "scope_guard.h"

RPCClient::RPCClient()
    : service_context_(nullptr),
      idx_(1)
{
}

void RPCClient::InitRPCClient(ServiceContext* sctx)
{
    service_context_ = sctx;
}

int RPCClient::CallRPC(uint32_t dest, const Message& msg, const RPCCallBack& func)
{
    if (service_context_ == nullptr)
    {
        LOG(ERROR) << "m_service_context is nullptr";
        return -1;
    }

    auto context = service_context_->pack_context();
    VLOG(50) << context << ", type = " << msg.GetTypeName() << ", content = " << msg.ShortDebugString();

    char* data = nullptr;
    uint32_t size = 0;
    int session = idx_++;
    SerializeToInnerData(msg, data, size, context);
    service_context_->WrapSkynetSend(service_context_->get_skynet_context(), 0, dest, PTYPE_RPC_SERVER | PTYPE_TAG_DONTCOPY, session, data, size);
    rpc_[session] = func;
    return session;
}

void RPCClient::CancelRPC(int& id)
{
    rpc_.erase(id);
    id = 0;
}

void RPCClient::RPCClientEvent(const char* data, uint32_t size, uint32_t source, int session)
{
    std::unordered_map<int, RPCCallBack>::iterator it = rpc_.find(session);
    if (it == rpc_.end())
    {
        return;
    }

    InPack pack;
    if (!pack.ParseFromInnerData(data, size))
    {
        LOG(ERROR) << "dispatch message pack error";
        return;
    }

    MessagePtr msg = pack.CreateMessage();
    if (msg == nullptr)
    {
        LOG(ERROR) << "rpc client message pb error";
        return;
    }

    VLOG(50) << "RECV RPC CLIENT PACK. session" << session << "type: " << msg->GetTypeName() << " content: [" << msg->ShortDebugString() << "]";

    if (service_context_->proto_profile())
    {
        uint64_t start = TimeUtil::now_tick_us();
        service_context_->MonitorDownstreamTraffic(pack.type_name(), pack.data_len(), start);

        it->second(msg);

        uint64_t end = TimeUtil::now_tick_us();
        service_context_->MonitorWorkload(pack.type_name(), end - start, end);
    }
    else
    {
        it->second(msg);
    }

    rpc_.erase(session);
}

void RPCClient::RPCTimeoutEvent(int session)
{
    std::unordered_map<int, RPCCallBack>::iterator it = rpc_.find(session);
    if (it == rpc_.end())
    {
        return;
    }

    it->second(nullptr);
    rpc_.erase(session);
}

RPCServer::RPCServer()
    : service_context_(nullptr),
      source_(0),
      session_(0)
{
}

void RPCServer::InitRPCServer(ServiceContext* sctx)
{
    service_context_ = sctx;
}

void RPCServer::RegisterRPC(const std::string& name, const CallBack& func)
{
    callbacks_[name] = func;
}

DispatcherStatus RPCServer::RPCServerEvent(const char* data, uint32_t size, uint32_t source, int session)
{
    if (service_context_ == nullptr)
    {
        LOG(ERROR) << "m_service_context is nullptr";
        return DispatcherStatus::DISPATCHER_FORBIDDEN;
    }

    InPack pack;
    if (!pack.ParseFromInnerData(data, size))
    {
        LOG(ERROR) << "dispatch message pack error";
        return DispatcherStatus::DISPATCHER_PACK_ERROR;
    }

    CallBackMap::iterator it = callbacks_.find(pack.type_name());
    if (it == callbacks_.end())
    {
        LOG(ERROR) << "rpc dispatch message callback error, type: " << pack.type_name();
        return DispatcherStatus::DISPATCHER_CALLBACK_ERROR;
    }

    MessagePtr msg = pack.CreateMessage();
    if (msg == nullptr)
    {
        LOG(ERROR) << "dispatch message pb error";
        return DispatcherStatus::DISPATCHER_PB_ERROR;
    }

    source_ = source;
    session_ = session;

    ScopeGuard pack_guard(
            [this, &pack] 
            { 
                rpc_pack_context_ = pack.pack_context(); 
            }, 
            [this] 
            { 
                rpc_pack_context_.reset(); 
            }
    );

    VLOG(50) << "RECV RPC PACK. " << rpc_pack_context_ << " proto type: " << pack.type_name() << " content: [" << msg->ShortDebugString() << "]";

    MessagePtr rsp;
    if (service_context_->proto_profile())
    {
        uint64_t start = TimeUtil::now_tick_us();
        service_context_->MonitorDownstreamTraffic(pack.type_name(), pack.data_len(), start);

        rsp = it->second(msg);

        uint64_t end = TimeUtil::now_tick_us();
        service_context_->MonitorWorkload(pack.type_name(), end - start, end);
    }
    else
    {
        rsp = it->second(msg);
    }

    if (rsp != nullptr)
    {
        response(rsp, source, session);
    }
    return DispatcherStatus::DISPATCHER_SUCCESS;
}

void RPCServer::response(MessagePtr msg, uint32_t dest, int session)
{
    char* data = nullptr;
    uint32_t size = 0;
    SerializeToInnerData(*msg, data, size, rpc_pack_context_);
    service_context_->WrapSkynetSend(service_context_->get_skynet_context(), 0, dest, PTYPE_RPC_CLIENT | PTYPE_TAG_DONTCOPY, session, data, size);

    VLOG(50) << "SEND RPC RESPONSE PACK. " << rpc_pack_context_ << " type:" << msg->GetTypeName() << " content: [" << msg->ShortDebugString() << "]";
}
