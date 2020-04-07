#ifndef __AGENT_NETWORK_SYSTEM_H__
#define __AGENT_NETWORK_SYSTEM_H__
#include "sub_system.h"

class NetworkSystem final : public SubSystem
{
    public:
        NetworkSystem(Agent* agent);
        ~NetworkSystem();
        NetworkSystem(const NetworkSystem&) = delete;
        NetworkSystem& operator= (const NetworkSystem&) = delete;

    public:
        void Init(SocketType socket_type, int fd, uint32_t gate, uint32_t dog, const std::string& ip, int port);
        void ServiceStartEvent();
        void ServiceOverEvent();

        void ShutdownConnect();

        void SendToUser(const Message& msg);
        void SendToUser(void* inner_data, uint32_t size);
        void SendToDog(const Message& msg);

        void HandleClientDisconnect();
        void HandleRebind(int fd, SocketType socket_type, const std::string& ip, int port, const std::string& realip);

        bool IsAlive() const { return is_alive_; }

    protected:
        virtual void RegisterCallBack() override;

    private:
        void CheckHeartBeat();
        void HandleClientMsgHeartBeatREQ(MessagePtr data, uint64_t uid);

    private:
        SocketType  socket_type_;
        int         agent_fd_;
        uint32_t    gate_handle_;           // 网关handle
        uint32_t    dog_handle_;            // watchdog handle
        std::string client_ip_;             // 客户端ip
        int         client_port_;           // 客户端端口
        bool        is_connected_;
        bool        is_alive_;
        int         check_heartbeat_timer_;
        uint64_t    last_heartbeat_timestamp_;

        static const int kHeartBeatTimeout = 10;
};

#endif
