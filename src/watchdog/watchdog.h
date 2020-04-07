#ifndef __WATCHDOG_WATCHDOG_H__
#define __WATCHDOG_WATCHDOG_H__

#include "../common/msg_service.h"
#include "../common/socket_type.h"
#include <atomic>

struct AgentFD;
// 客户端登录成功之后才会创建AgentHandle
struct AgentHandle
{
    uint32_t        handle_ = 0;                // 用户的agent handle
    uint64_t        uid_ = 0;                   // uid
    AgentFD*        agent_fd_ = nullptr;        // 
    int             disconnect_timer_ = 0;      // 掉线后有一段重连的预留时间，这个定时器用来处理预留时间到期后销毁用户的agent
};
using AgentHandlePtr = std::shared_ptr<AgentHandle>;

// 客户端连接之后就会创建AgentFd
struct AgentFD
{
    int             fd_ = -1;
    std::string     ip_port_;                   // 客户端连接的ip和端口
    std::string     real_ip_;                   // 客户端的真实ip
    uint64_t        uid_ = 0;
    AgentHandlePtr  handle_;
    SocketType      socket_type_ = SocketType::NULL_SOCKET;
    bool            login_check_ = false;
    int             login_timer_ = 0;  // 连接上来1分钟之内没走登录流程，断开
};

class Agent;
class Watchdog final : public Service
{
    friend class Agent;
    public:
        Watchdog();
        ~Watchdog() = default;
        Watchdog(const Watchdog&) = delete;
        Watchdog& operator= (const Watchdog&) = delete;

    public:
        MsgServiceType AgentRouteType(const std::string& proto);
        uint32_t    AgentRouteDest(MsgServiceType type);

        int     WrapSkynetSocketSend(struct skynet_context* ctx, int id, const Message& msg, SocketType socket_type, uint32_t gate);

        uint32_t    GetBucketCap() const { return bucket_cap_.load(); }
        uint32_t    GetRateLimit() const { return rate_limit_.load(); }
        bool        GetRateLimitSwitch() const { return rate_limit_switch_.load(); }

    public:
        virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;
        
    protected:
        virtual void VServiceStartEvent() override;   // 服务启动完成
        virtual void VServiceOverEvent() override;    // 服务关闭事件
        virtual void VServerStopEvent() override;    // 关闭服务器事件

        virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;
        virtual void VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session) override;

    private:
        void    RegisterCallBack();
        void    MonitorSelf();

        
        void    HandleSystemRegisterMsgService(MessagePtr data, uint32_t handle);

    private:
        void    HandleClientConnect(int fd, const std::string& ip_port);
        void    HandleClientClose(int fd);
        void    StartListen();
        void    ShutdownClientConnect(int fd);      // 通知网关断开客户端的连接
        void    ShutdownAgentService(uint32_t handle);
        void    OnlineStatus(std::string& ret);
        void    SetBucketRate(bool rate_switch, uint32_t rate, uint32_t bucket_cap);


    private:
        void    HandleServiceKickUserREQ(MessagePtr data, uint32_t handle);
        void    HandleClientUserLoginREQ(MessagePtr data, int fd);
        void    HandleRPCCUserLogin(MessagePtr data, int fd);

    private:
        void    KickFD(int fd);
        void    DelAgentFD(int fd);
        bool    IsAgentFDExist(int fd);
        void    DelAgentHandle(uint32_t handle);
        void    NewAgentHandle(uint32_t handle, uint64_t uid, AgentFD* af);

        AgentHandlePtr    GetAgentHandleByFD(int fd);
        AgentHandlePtr    GetAgentHandleByHandle(uint32_t handle);
        AgentHandlePtr    GetAgentHandleByUid(uint64_t uid);

        uint32_t    NewAgentService(int fd, uint64_t uid);
        uint32_t    RebindAgent(int fd, AgentHandlePtr ah);

    private:
        uint32_t            gate_;      // gate的handle
        bool                is_listening_;
        Dispatcher<int>     dog_dsp_;
        std::unordered_map<uint32_t, AgentHandlePtr>    agent_handles_; // handle -> AgentHandlePtr
        std::unordered_map<int, AgentFD>                agent_fds_;     // socketfd -> AgentFd
        std::unordered_map<uint64_t, AgentHandlePtr>    agent_users_;    // uid -> AgentHandlePtr

        std::unordered_map<MsgServiceType, uint32_t, MsgServiceTypeHash>    msg_services_;  // MsgServiceType -> MsgServiceHandle
        std::unordered_map<std::string, MsgServiceType> msg_routers_;   // proto_name -> MsgServiceType
        static const int kSelfMonitorInterval = 60 * 100;

        static const int kAgentDisconnectProtectTime = 600 * 100; // 断线重连保留时间

    private:
        static const uint32_t kDefaultRateLimit = 2;
        static const uint32_t kDefaultBucketCap = 30;
 
        std::atomic<uint32_t>       bucket_cap_;
        std::atomic<uint32_t>       rate_limit_;
        std::atomic<bool>           rate_limit_switch_;
};

extern Watchdog*    g_watchdog;

#endif
