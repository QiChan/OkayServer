#include "master.h"
#include <tools/text_parm.h>
#include <tools/string_util.h>
#include "../common/scope_guard.h"
#include "../pb/inner.pb.h"

Master::Master()
    : slave_num_(0)
{
}

bool Master::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }
    
    std::string temp;
    std::string it(static_cast<const char*>(parm), len);
    std::tie(temp, it) = StringUtil::DivideString(it, ' ');
    slave_num_ = atoi(temp.c_str());       // slave_num

    std::tie(slave_name_, it) = StringUtil::DivideString(it, ' ');    // slave_name
    slave_parm_ = it;

    ServiceBroadcastFilterInit();
    RegServiceName(slave_name_, true, false);

    RegisterCallBack();

    LOG(INFO) << "service_name: [" << slave_name_ << "] slave_num: " << slave_num_;

    return true;
}

void Master::VServiceStartEvent()
{
    // start slave service，不能再InitService函数创建，否则子服务的parent会设置错误
    
    std::string head;
    head = "leader ";
    auto leader_handle = NewChildService(slave_name_, head + slave_parm_);
    if (leader_handle == 0)
    {
        LOG(ERROR) << "new child service failed. service_name: [" << slave_name_ << "] parm: " << slave_parm_;
        return;
    }
    slaves_.push_back(leader_handle);

    head = "member ";
    for (int i = 0; i < slave_num_ - 1; ++i)
    {
        auto member_handle = NewChildService(slave_name_, head + slave_parm_);
        if (member_handle == 0)
        {
            LOG(ERROR) << "new child service failed. service_name: [" << slave_name_ << "] parm: " << slave_parm_;
            return;
        }
        slaves_.push_back(member_handle);
    }

    for (auto i : slaves_)
    {
        LOG(INFO) << "slave name: [" << slave_name_ << "] slave handle: " << std::hex << i << " start.";
    }
}

void Master::VServiceOverEvent()
{
    LOG(INFO) << "master [" << service_name() << "] shutdown.";
}

void Master::VServerStopEvent()
{
}

void Master::RegisterCallBack()
{
}

void Master::ServiceBroadcastFilterInit()
{
}

bool Master::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    bool broadcast = false;

    TextParm parm(data.c_str());
    broadcast = atoi(parm.get("broadcast"));

    RouteToSlave(data.c_str(), data.size(), source, session, PTYPE_TEXT, broadcast);
    return true;
}

void Master::VHandleClientMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    RouteToSlave(data, size, source, session, PTYPE_CLIENT, false);
}

void Master::VHandleServiceMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    DispatcherStatus status = service_msg_dsp_.DispatchMessage(data, size, source);

    if (status == DispatcherStatus::DISPATCHER_SUCCESS)
    {
        return;
    }

    if (status == DispatcherStatus::DISPATCHER_CALLBACK_ERROR)
    {
        InPack pack;
        pack.ParseFromInnerData(data, size);
        bool broadcast = false;
        if (dsp_broadcast_filter_.find(pack.type_name()) != dsp_broadcast_filter_.end())
        {
            broadcast = true;
        }
        RouteToSlave(data, size, source, session, PTYPE_SERVICE, broadcast);
        return;
    }

    LOG(ERROR) << "[" << service_name() << "] inner proto to service error: " << static_cast<uint16_t>(status);
    return;
}

void Master::VHandleSystemMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    DispatcherStatus status = system_msg_dsp_.DispatchMessage(data, size, source);

    if (status == DispatcherStatus::DISPATCHER_SUCCESS)
    {
        return;
    }

    if (status == DispatcherStatus::DISPATCHER_CALLBACK_ERROR)
    {
        InPack pack;
        pack.ParseFromInnerData(data, size);
        bool broadcast = false;
        if (dsp_broadcast_filter_.find(pack.type_name()) != dsp_broadcast_filter_.end())
        {
            broadcast = true;
        }

        RouteToSlave(data, size, source, session, PTYPE_SYSTEM_SERVICE, broadcast);
        return;
    }

    LOG(ERROR) << "[" << service_name() << "] system proto to service error: " << static_cast<uint16_t>(status);
    return;
}

void Master::VHandleRPCServerMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    RouteToSlave(data, size, source, session, PTYPE_RPC_SERVER, false);
}

void Master::VHandleRPCClientMsg(const char* data, uint32_t size, uint32_t source, int session)
{
    RouteToSlave(data, size, source, session, PTYPE_RPC_CLIENT, false);
}

void Master::RouteToSlave(const char* data, uint32_t size, uint32_t source, int session, int type, bool broadcast)
{
    if (slaves_.empty())
    {
        LOG(ERROR) << "[" << service_name() << "] slave empty error.";
        return;
    }

    if (broadcast)
    {
        for (auto h : slaves_)
        {
            skynet_send(ctx(), source, h, type, session, (void*)data, size);
        }
    }
    else
    {
        uint64_t uid = service_context_.current_process_uid();
        //有uid按uid hash，没uid随机
        int idx = 0;
        if (uid != 0)
        {
            idx = uid % slaves_.size();
        }
		else
        {
			idx = rand() % slaves_.size();
        }
		uint32_t handle = slaves_[idx];
        skynet_send(ctx(), source, handle, type, session, (void*)data, size);
    }
}
