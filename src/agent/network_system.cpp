#include "network_system.h"
#include "agent.h"
#include "../pb/agent.pb.h"
#include "../pb/login.pb.h"

NetworkSystem::NetworkSystem(Agent* agent)
    : SubSystem(agent),
      socket_type_(SocketType::NULL_SOCKET),
      agent_fd_(0),
      gate_handle_(0),
      dog_handle_(0),
      client_port_(0),
      is_connected_(false),
      is_alive_(false),
      check_heartbeat_timer_(0),
      last_heartbeat_timestamp_(0)
{
}

NetworkSystem::~NetworkSystem()
{
    if (IsAlive())
    {
        LOG(ERROR) << "uid: " << agent_service_->uid() << " should be not alive";
    }
}

void NetworkSystem::Init(SocketType socket_type, int fd, uint32_t gate, uint32_t dog, const std::string& ip, int port)
{
    socket_type_ = socket_type;
    agent_fd_ = fd;
    gate_handle_ = gate;
    dog_handle_ = dog;
    client_ip_ = ip;
    client_port_ = port;

    RegisterCallBack();

    is_connected_ = true;
}

void NetworkSystem::CheckHeartBeat()
{
    if (!IsAlive())
    {
        return;
    }

    StopTimer(check_heartbeat_timer_);
    uint64_t now = time(NULL);
    if (now - last_heartbeat_timestamp_ >= kHeartBeatTimeout)
    {
        LOG(INFO) << "agent heartbeat timeout. uid: " << agent_service_->uid();
        ShutdownConnect();
        return;
    }
    check_heartbeat_timer_ = StartTimer(300, std::bind(&NetworkSystem::CheckHeartBeat, this));
}

void NetworkSystem::ShutdownConnect()
{
    if (!is_connected_)
    {
        return;
    }

    char sendline[100];
    sprintf(sendline, "kick %d", agent_fd_);
    WrapSkynetSend(gate_handle_, PTYPE_TEXT, sendline, strlen(sendline));
    LOG(INFO) << "agent shutdown self. fd: " << agent_fd_ << " uid: " << agent_service_->uid();
    is_connected_ = false;
}

void NetworkSystem::ServiceStartEvent()
{
    is_alive_ = true;

    pb::iAgentInit  req;
    req.set_uid(agent_service_->uid());
    req.set_ip(client_ip_);
    BroadcastToService(req);

    last_heartbeat_timestamp_ = time(NULL);
    CheckHeartBeat();
}

void NetworkSystem::ServiceOverEvent()
{
    if (IsAlive())
    {
        pb::iAgentRelease   req;
        req.set_uid(agent_service_->uid());
        BroadcastToService(req);

        is_alive_ = false;
    }

    StopTimer(check_heartbeat_timer_);
}
    
void NetworkSystem::SendToUser(const Message& msg)
{
    if (!is_connected_)
    {
        return;
    }
    WrapSkynetSocketSend(agent_fd_, msg, socket_type_, gate_handle_);
}

void NetworkSystem::SendToUser(void* inner_data, uint32_t size)
{
    if (!is_connected_)
    {
        return;
    }
    WrapSkynetSocketSend(agent_fd_, inner_data, size, socket_type_, gate_handle_);
}


void NetworkSystem::HandleClientDisconnect()
{
    StopTimer(check_heartbeat_timer_);
    is_connected_ = false;
    agent_fd_ = 0;

    if (IsAlive())
    {
        pb::iAgentDisconnect    req;
        req.set_uid(agent_service_->uid());
        BroadcastToService(req);
    }
    LOG(INFO) << "agent disconnect. uid: " << agent_service_->uid();
}

void NetworkSystem::HandleRebind(int fd, SocketType socket_type, const std::string& ip, int port, const std::string& realip)
{
    is_connected_ = true;
    last_heartbeat_timestamp_ = time(NULL);

    socket_type_ = socket_type;
    agent_fd_ = fd;
    if (realip.empty())
    {
        client_ip_ = ip;
    }
    else
    {
        client_ip_ = realip;
    }
    client_port_ = port;

    if (IsAlive())
    {
        pb::iAgentRebind    req;
        req.set_uid(agent_service_->uid());
        req.set_ip(client_ip_);
        BroadcastToService(req);
    }

    CheckHeartBeat();

    LOG(INFO) << "agent rebind. id: " << agent_service_->uid() << " ip: " << client_ip_ << " port: " << client_port_;
}

void NetworkSystem::RegisterCallBack()
{
    MsgCallBack(pb::HeartBeatREQ::descriptor()->full_name(), std::bind(&NetworkSystem::HandleClientMsgHeartBeatREQ, this, std::placeholders::_1, std::placeholders::_2));

    ServiceCallBack(pb::iInitAgentREQ::descriptor()->full_name(), [this](MessagePtr data, uint32_t handle)
    {
        if (!IsAlive())
        {
            return;
        }
        auto msg = std::dynamic_pointer_cast<pb::iInitAgentREQ>(data);
        LOG(INFO) << "---------iInitAgentREQ. handle: " << msg->handle(); 
        pb::iAgentInit  rsp;
        rsp.set_uid(agent_service_->uid());
        rsp.set_ip(client_ip_);
        SendToService(rsp, msg->handle());
    });

    ServiceCallBack(pb::iShutdownAgentREQ::descriptor()->full_name(), [this](MessagePtr data, uint32_t handle)
    {
        ShutdownConnect();
    });
}

void NetworkSystem::HandleClientMsgHeartBeatREQ(MessagePtr data, uint64_t uid)
{
    last_heartbeat_timestamp_ = time(NULL);
    pb::HeartBeatRSP    rsp;
    SendToUser(rsp);
}
