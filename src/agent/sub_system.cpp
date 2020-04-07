#include "sub_system.h"
#include "agent.h"

SubSystem::SubSystem(Agent* agent)
    : agent_service_(agent) 
{
    assert(agent != nullptr);
}

int SubSystem::CallRPC(const std::string& dest, const Message& msg, const RPCCallBack& func)
{
    return agent_service_->CallRPC(agent_service_->ServiceHandle(dest), msg, func);
}

void SubSystem::BroadcastToService(const Message& msg)
{
    agent_service_->BroadcastToService(msg);
}

void SubSystem::SendToService(const Message& msg, uint32_t handle)
{
    agent_service_->SendToService(msg, handle);
}

void SubSystem::SendToService(const Message& msg, const std::string& dest)
{
    agent_service_->SendToService(msg, agent_service_->ServiceHandle(dest));
}

void SubSystem::SendToUser(const Message& msg)
{
    agent_service_->SendToUser(msg);
}

void SubSystem::WrapSkynetSend(uint32_t dest, int type, void* msg, size_t sz)
{
    agent_service_->WrapSkynetSend(agent_service_->ctx(), 0, dest, type, 0, msg, sz);
}

void SubSystem::WrapSkynetSocketSend(int id, const Message& msg, SocketType socket_type, uint32_t gate)
{
    agent_service_->WrapSkynetSocketSend(agent_service_->ctx(), id, msg, socket_type, gate);
}

void SubSystem::WrapSkynetSocketSend(int id, void* inner_data, uint32_t size, SocketType socket_type, uint32_t gate)
{
    agent_service_->WrapSkynetSocketSend(agent_service_->ctx(), id, inner_data, size, socket_type, gate);
}

int SubSystem::StartTimer(int time, const TimerCallBack& func, const std::string& timer_name)
{
    return agent_service_->StartTimer(time, func, timer_name);
}

void SubSystem::StopTimer(int& id)
{
    agent_service_->StopTimer(id);
}


void SubSystem::MsgCallBack(const std::string& name, const typename Dispatcher<uint64_t>::CallBack& func)
{
    agent_service_->MsgCallBack(name, func);
}

void SubSystem::ServiceCallBack(const std::string& name, const typename Dispatcher<uint32_t>::CallBack& func)
{
    agent_service_->ServiceCallBack(name, func);
}
