#ifndef __AGENT_SUBSYSTEM_H__
#define __AGENT_SUBSYSTEM_H__

#include <string>
#include "../common/rpc.h"
#include "../common/timer.h"
#include "../common/pack.h"

class Agent;
class SubSystem
{
    public:
        SubSystem(Agent* agent);
        virtual ~SubSystem() = default;
        SubSystem(const SubSystem&) = delete;
        SubSystem& operator= (const SubSystem&) = delete;

    protected:
        virtual void RegisterCallBack() = 0;

    protected:
        int     CallRPC(const std::string& dest, const Message& msg, const RPCCallBack& func);
        void    BroadcastToService(const Message& msg);
        void    SendToService(const Message& msg, const std::string& dest);
        void    SendToService(const Message& msg, uint32_t handle);
        void    SendToUser(const Message& msg);
        void    WrapSkynetSend(uint32_t dest, int type, void* msg, size_t sz);
        void    WrapSkynetSocketSend(int id, const Message& msg, SocketType socket_type, uint32_t gate);
        void    WrapSkynetSocketSend(int id, void* inner_data, uint32_t size, SocketType socket_type, uint32_t gate);
        int     StartTimer(int time, const TimerCallBack& func, const std::string& timer_name = std::string());
        void    StopTimer(int& id);

        void    MsgCallBack(const std::string& name, const typename Dispatcher<uint64_t>::CallBack& func);
        void    ServiceCallBack(const std::string& name, const typename Dispatcher<uint32_t>::CallBack& func);

    protected:
        Agent*  agent_service_;
};

#endif
