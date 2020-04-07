/*
 * 客户端关闭流程：
 * 1. 套接字断开连接
 *  Watchdog::HandleClientClose:
 *      1) 启动定时器，10分钟后调用Watchdog::ShutdownAgentService
 *      2) 给agent发 "client_disconnect" 消息，agent调用NetworkSystem::HandleClientDisconnect函数处理
 *      3) NetworkSystem::HandleClientDisconnect会给所有MsgService发送iAgentDisconnect消息
 *  ShutdownAgentService的逻辑：
 *      直接让AgentService关闭，不要再给gate发kick消息了
 *
 * 2. 其他服务主动ShutdownAgent
 *      1) Agent收到消息后，给gate发送kick消息，断开套接字连接 
 *      2) 套接字断开连接后，gate会想watchdog发送close消息，逻辑流转到上面
 *
 * 3. kick user类比2
 *      watchdog收到消息后，向agent发送iShutdownAgentREQ消息即可
 */

#include "watchdog.h"
#include <tools/string_util.h>
#include <tools/text_parm.h>
#include <json/json.h>
#include "../pb/system.pb.h"
#include "../pb/agent.pb.h"
#include "../pb/login.pb.h"

extern "C"
{
#include "skynet_server.h"
}

Watchdog*   g_watchdog;

Watchdog::Watchdog()
    : gate_(0),
      is_listening_(false),
      dog_dsp_(&service_context_),
      bucket_cap_(kDefaultBucketCap),
      rate_limit_(kDefaultRateLimit),
      rate_limit_switch_(true)
{
}

bool Watchdog::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    sscanf((const char*)parm, "%u", &gate_);

    std::string name = "watchdog";
    name += std::to_string(service_context_.harborid());
    RegServiceName(name, true, true);

    RegisterCallBack();

    StartTimer(kSelfMonitorInterval, std::bind(&Watchdog::MonitorSelf, this));

    g_watchdog = this;
    return true;
}

void Watchdog::VServiceStartEvent()
{
    LOG(INFO) << "[" << service_name() << "] start.";
}

void Watchdog::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void Watchdog::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
    pb::iServerStop brc;
    for (auto& elem : agent_handles_)
    {
        SendToSystem(brc, elem.first);
    }
}

void Watchdog::RegisterCallBack()
{
    // 向main节点注册当前watchdog
    pb::iRegisterWatchdog   req;
    req.set_handle(skynet_context_handle(ctx()));
    SendToSystem(req, main_);


    // 用户还未登录的时候, 收到的客户端数据, 是获取不到uid, 需要在具体的协议里面去设置process_uid
    RegisterPTypeParsePackCTXCallBack(PTYPE_CLIENT, [this](const char*) { service_context_.reset_pack_context(0); });

    SystemCallBack(pb::iRegisterMsgService::descriptor()->full_name(), std::bind(&Watchdog::HandleSystemRegisterMsgService, this, std::placeholders::_1, std::placeholders::_2));

    ServiceCallBack(pb::iKickUser::descriptor()->full_name(), std::bind(&Watchdog::HandleServiceKickUserREQ, this, std::placeholders::_1, std::placeholders::_2));

    dog_dsp_.RegisterCallBack(pb::UserLoginREQ::descriptor()->full_name(), std::bind(&Watchdog::HandleClientUserLoginREQ, this, std::placeholders::_1, std::placeholders::_2));
}

void Watchdog::MonitorSelf()
{
    LOG(INFO) << "[" << service_name() << "] ======WatchDog Self Monitor======" 
        << " agent_fds size: " << agent_fds_.size()
        << " agent_handles size: " << agent_handles_.size()
        << " agent_users size: " << agent_users_.size();
    StartTimer(kSelfMonitorInterval, std::bind(&Watchdog::MonitorSelf, this));
}

void Watchdog::HandleSystemRegisterMsgService(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iRegisterMsgService>(data);
    for (int i = 0; i < msg->item_size(); ++i)
    {
        const pb::iRegisterProto& s = msg->item(i);
        MsgServiceType type = static_cast<MsgServiceType>(s.type());
        msg_services_[type] = s.handle();
        for (int j = 0; j < s.proto_size(); ++j)
        {
            msg_routers_[s.proto(j)] = type;
        }
    }
    LOG(INFO) << "[" << service_name() << "] update route table. proto_route size: " << msg_routers_.size();
}

bool Watchdog::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    if (Service::VHandleTextMessage(from, data, source, session))
    {
        return true;
    }
    if (from == "gate")
    {
        std::string cmd;
        std::string fd_str;
        std::string it;
        std::tie(cmd, it) = StringUtil::DivideString(data, ' ');
        if (cmd == "accept")     // 客户端连接
        {
            std::string ip_port;
            std::tie(fd_str, ip_port) = StringUtil::DivideString(it, ' ');
            int fd = std::stoi(fd_str);
            HandleClientConnect(fd, ip_port);
        }
        else if (cmd == "close")  // 断开连接
        {
            int fd = 0;
            std::tie(fd_str, std::ignore) = StringUtil::DivideString(it, ' ');
            fd = std::stoi(fd_str);
            HandleClientClose(fd);
        }
    }
    else if (from == "php" || from == "main")
    {
        std::string response = "no cmd\n";
        TextParm parm(data.c_str());
        const char* cmd = parm.get("cmd");
        if (strcmp(cmd, "listen") == 0)
        {
            StartListen();
            response = "listen ok\n";
        }
        else if (strcmp(cmd, "online_status") == 0)
        {
            OnlineStatus(response);
        }
        else if (strcmp(cmd, "set_rate") == 0)
        {
            response = "set rate ok\n";
            SetBucketRate(parm.get_uint("rate_switch"), parm.get_uint("rate"), parm.get_uint("bucket_cap"));
        }

        ResponseTextMessage(from, response);
    }
    else
    {
        LOG(ERROR) << "from: " << from << " unknow text command: " << data; 
    }

    return true;
}

void Watchdog::HandleClientConnect(int fd, const std::string& ip_port)
{
    if (IsServerStop())
    {
        KickFD(fd);
        return;
    }

    LOG(INFO) << "new connection: " << fd << " " << ip_port;
    AgentFD& af = agent_fds_[fd];
    af.fd_ = fd;
    af.ip_port_ = ip_port;
    af.handle_ = nullptr;
    af.socket_type_ = SocketType::TCP_SOCKET;
    af.login_timer_ = StartTimer(30 * 100, [fd, this]() // 30秒之内没发送登陆协议自动断开连接
    {
        ShutdownClientConnect(fd);
        LOG(INFO) << "shutdown fd because timeout not login. fd:" << fd;
    });

    char sendline[100];
    sprintf(sendline, "start %d", fd);
    WrapSkynetSend(ctx(), 0, gate_, PTYPE_TEXT, 0, sendline, strlen(sendline));     // 让网关开始接受客户端发过来的数据
}

void Watchdog::ShutdownClientConnect(int fd)
{
    if (!IsAgentFDExist(fd))
    {
        return;
    }

    KickFD(fd);
    LOG(INFO) << "[" << service_name() << "] kick " << fd;
}

void Watchdog::KickFD(int fd)
{
    char sendline[100];
    sprintf(sendline, "kick %d", fd);
    WrapSkynetSend(ctx(), 0, gate_, PTYPE_TEXT, 0, sendline, strlen(sendline));
}

void Watchdog::HandleClientClose(int fd)
{
    if (!IsAgentFDExist(fd))
    {
        // TODO: 上线后可以去掉
        LOG(INFO) << "socket close no find fd: " << fd;
        return;
    }

    AgentHandlePtr handle = GetAgentHandleByFD(fd);
    DelAgentFD(fd);

    if (handle != nullptr)
    {
        handle->disconnect_timer_ = StartTimer(kAgentDisconnectProtectTime, std::bind(&Watchdog::ShutdownAgentService, this, handle->handle_));

        char sendline[100];
        sprintf(sendline, "dog %lu client_disconnect", handle->uid_);
        WrapSkynetSend(ctx(), 0, handle->handle_, PTYPE_TEXT, 0, sendline, strlen(sendline));
        LOG(INFO) << "socket close fd: " << fd << " agent exist, uid:" << handle->uid_;
    }
    else
    {
        LOG(INFO) << "socket close fd:" << fd << " agent not exist";
    }
}

void Watchdog::ShutdownAgentService(uint32_t handle)
{
    auto agent_handle = GetAgentHandleByHandle(handle);
    if (agent_handle == nullptr)
    {
        LOG(WARNING) << "shutdown agent service. but agent handle not exist.";
        return;
    }

    char sendline[100];
    sprintf(sendline, "dog %lu client_release", agent_handle->uid_);
    WrapSkynetSend(ctx(), 0, handle, PTYPE_TEXT, 0, sendline, strlen(sendline));
    LOG(INFO) << "shutdown agent service. uid: " << agent_handle->uid_;
    DelAgentHandle(handle);
}

void Watchdog::HandleServiceKickUserREQ(MessagePtr data, uint32_t handle)
{
    auto msg = std::dynamic_pointer_cast<pb::iKickUser>(data);
    uint64_t uid = msg->uid();

    auto ah = GetAgentHandleByUid(uid);
    if (ah == nullptr)
    {
        return;
    }

    pb::iShutdownAgentREQ   req;
    SendToService(req, ah->handle_);
    LOG(INFO) << "[" << service_name() << "] kick user. uid: " << uid;
}

void Watchdog::StartListen()
{
    if (is_listening_)
    {
        return;
    }
    is_listening_ = true;
    char sendline[100];
    sprintf(sendline, "listen");
    WrapSkynetSend(ctx(), 0, gate_, PTYPE_TEXT, 0, sendline, strlen(sendline));
}

void Watchdog::SetBucketRate(bool rate_switch, uint32_t rate, uint32_t bucket_cap)
{
    LOG(INFO) << "watchdog set rate: " << rate << " switch: " << rate_switch;
    bucket_cap_.store(bucket_cap);
    rate_limit_.store(rate);
    rate_limit_switch_.store(rate_switch);
}

void Watchdog::DelAgentFD(int fd)
{
    auto it = agent_fds_.find(fd);
    if (it == agent_fds_.end())
    {
        return;
    }

    AgentHandlePtr handle = it->second.handle_;
    if (handle != nullptr)
    {
        handle->agent_fd_ = nullptr;
    }
    StopTimer(it->second.login_timer_);
    agent_fds_.erase(it);
}

bool Watchdog::IsAgentFDExist(int fd)
{
    auto it = agent_fds_.find(fd);
    if (it == agent_fds_.end())
    {
        return false;
    }
    return true;
}

AgentHandlePtr Watchdog::GetAgentHandleByFD(int fd)
{
    auto it = agent_fds_.find(fd);
    if (it == agent_fds_.end())
    {
        return nullptr;
    }
    return it->second.handle_;
}

AgentHandlePtr Watchdog::GetAgentHandleByHandle(uint32_t handle)
{
    auto it = agent_handles_.find(handle);
    if (it == agent_handles_.end())
    {
        return nullptr;
    }
    return it->second;
}

AgentHandlePtr Watchdog::GetAgentHandleByUid(uint64_t uid)
{
    auto it = agent_users_.find(uid);
    if (it == agent_users_.end())
    {
        return nullptr;
    }
    return it->second;
}

void Watchdog::DelAgentHandle(uint32_t handle)
{
    AgentHandlePtr handle_ptr = GetAgentHandleByHandle(handle);
    if (handle_ptr == nullptr)
    {
        return;
    }

    AgentFD* af = handle_ptr->agent_fd_;
    if (af != nullptr)
    {
        af->handle_ = nullptr;
    }
    agent_users_.erase(handle_ptr->uid_);
    agent_handles_.erase(handle);
}

void Watchdog::NewAgentHandle(uint32_t handle, uint64_t uid, AgentFD* af)
{
    agent_handles_[handle] = std::make_shared<AgentHandle>();
    AgentHandlePtr ah = agent_handles_[handle];
    ah->handle_ = handle;
    ah->uid_ = uid;
    ah->agent_fd_ = af; 
    
    af->handle_ = ah;
    agent_users_[uid]= ah;
}

uint32_t Watchdog::NewAgentService(int fd, uint64_t uid)
{
    if (IsServerStop())
    {
        return 0;
    }

    if (!IsAgentFDExist(fd))
    {
        LOG(ERROR) << "new agent. but fd not exists. fd: " << fd << " uid:" << uid;
        return 0;
    }

    AgentFD* af = &agent_fds_[fd];

    char sendline[256];
    sprintf(sendline, "%d %u %u %u %lu %s %s", fd, static_cast<uint32_t>(af->socket_type_), gate_, skynet_context_handle(ctx()), uid, af->ip_port_.c_str(), af->real_ip_.c_str());
    struct skynet_context* agent_ctx = skynet_context_new("agent", sendline);
    if (agent_ctx == nullptr)
    {
        LOG(ERROR) << "new agent failed. uid: " << uid;
        return 0;
    }

    uint32_t handle = skynet_context_handle(agent_ctx);

    NewAgentHandle(handle, uid, af);

    LOG(INFO) << "new agent service. fd: " << fd << " uid: " << uid << " ip_port: " << af->ip_port_.c_str() << " real_ip: " << af->real_ip_.c_str();

    return handle;
}

uint32_t Watchdog::RebindAgent(int fd, AgentHandlePtr ah)
{
    if (!IsAgentFDExist(fd))
    {
        LOG(ERROR) << "rebind agent. but fd not exist. fd: " << fd << " uid: " << ah->uid_;
        return 0;
    }

    AgentFD* af = &agent_fds_[fd];

    uint32_t handle = ah->handle_;
    char sendline[256];
    sprintf(sendline, "dog %lu client_rebind %d %u %s %s", ah->uid_, fd, static_cast<uint32_t>(af->socket_type_), af->ip_port_.c_str(), af->real_ip_.c_str());
    WrapSkynetSend(ctx(), 0, handle, PTYPE_TEXT, 0, sendline, strlen(sendline));

    ah->agent_fd_ = af;
    af->handle_ = ah;
    StopTimer(ah->disconnect_timer_);

    LOG(INFO) << "agent rebind fd: " << fd << " uid: " << ah->uid_ << " ip_port: " << af->ip_port_.c_str() << " real_ip: " << af->real_ip_.c_str();

    return handle;
}

MsgServiceType Watchdog::AgentRouteType(const std::string& proto)
{
    auto it = msg_routers_.find(proto);
    if (it == msg_routers_.end())
    {
        return MsgServiceType::SERVICE_NULL;
    }
    return it->second;
}

uint32_t Watchdog::AgentRouteDest(MsgServiceType type)
{
    auto it = msg_services_.find(type);
    if (it == msg_services_.end())
    {
        return 0;
    }
    return it->second;
}


void Watchdog::VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session)
{
    char* c = (char*)data;
    int fd = atoi(strsep(&c, " "));

    DispatcherStatus status = dog_dsp_.DispatchClientMessage(c, size - (c - (char*)data), fd);
    if (status != DispatcherStatus::DISPATCHER_SUCCESS)
    {
		//这里还没有AgentHandle
        ShutdownClientConnect(fd);

        LOG(INFO) << "watchdog destroy agent cause error msg. fd: " << fd << " status: " << static_cast<uint16_t>(status);
    }
}

void Watchdog::OnlineStatus(std::string& ret)
{
    Json::Value root;
    for (auto& elem : agent_handles_)
    {
        Json::Value agent;
        agent["uid"] = std::to_string(elem.second->uid_);
        auto fd = elem.second->agent_fd_;
        if (fd != nullptr)
        {
            if (fd->real_ip_.empty())
            {
                agent["ip"] = fd->ip_port_;
            }
            else
            {
                agent["ip"] = fd->real_ip_;
            }
            agent["socket_type"] = static_cast<uint32_t>(fd->socket_type_);
            root.append(agent);
        }
    }

    Json::StyledWriter  writer;
    ret = writer.write(root);
}

int Watchdog::WrapSkynetSocketSend(struct skynet_context* ctx, int id, const Message& msg, SocketType socket_type, uint32_t gate)
{
    return service_context_.WrapSkynetSocketSend(ctx, id, msg, socket_type, gate);
}

void Watchdog::HandleClientUserLoginREQ(MessagePtr data, int fd)
{
    auto msg = std::dynamic_pointer_cast<pb::UserLoginREQ>(data);
    uint64_t uid = msg->uid();
    service_context_.set_current_process_uid(uid);

    if (!IsAgentFDExist(fd))
    {
        LOG(ERROR) << "user login. fd not found!";
        return;
    }

    auto& agent_fd = agent_fds_[fd];
    if (agent_fd.login_check_ == true)
    {
        LOG(WARNING) << "relogin. fd:" << fd << " uid: " << msg->uid();
        return;
    }

    StopTimer(agent_fd.login_timer_);

    agent_fd.login_check_ = true;
    agent_fd.uid_ = uid;

    if (IsServerStop())
    {
        pb::UserLoginRSP    rsp;
        rsp.set_code(-3);
        WrapSkynetSocketSend(ctx(), fd, rsp, agent_fd.socket_type_, gate_);
        ShutdownClientConnect(fd);
        return;
    }

    if (uid == 0)
    {
        pb::UserLoginRSP    rsp;
        rsp.set_code(-1);
        WrapSkynetSocketSend(ctx(), fd, rsp, agent_fd.socket_type_, gate_);
        ShutdownClientConnect(fd);
        return;
    }

    pb::iCheckLoginREQ  req;
    req.set_uid(uid);
    req.set_rdkey(msg->rdkey());
    req.set_version(msg->version());
    CallRPC(main_, req, std::bind(&Watchdog::HandleRPCCUserLogin, this, std::placeholders::_1, fd));

    return;
}

void Watchdog::HandleRPCCUserLogin(MessagePtr data, int fd)
{
    auto msg = std::dynamic_pointer_cast<pb::iCheckLoginRSP>(data);
    if (!IsAgentFDExist(fd))
    {
        LOG(INFO) << "fd not exsts. login failed. fd: " << fd;
        return;
    }
    
    auto& agent_fd = agent_fds_[fd];
    if (msg->code() != 0)
    {
        pb::UserLoginRSP    rsp;
        rsp.set_code(msg->code());
        WrapSkynetSocketSend(ctx(), fd, rsp, agent_fd.socket_type_, gate_);
        ShutdownClientConnect(fd);
        return;
    }
    agent_fd.real_ip_ = msg->client_ip();

    auto it = agent_users_.find(agent_fd.uid_);
    uint32_t handle;
    if (it == agent_users_.end())
    {
        handle = NewAgentService(fd, agent_fd.uid_);
    }
    else    // user already in server
    {
        auto ah = it->second;
        auto af = ah->agent_fd_;
        if (af != nullptr)  // 断开旧的socket
        {
            pb::UserLogoutRSP   rsp;
            rsp.set_code(-1);
            WrapSkynetSocketSend(ctx(), af->fd_, rsp, af->socket_type_, gate_);

            af->handle_ = nullptr;
            ShutdownClientConnect(af->fd_);
            LOG(INFO) << "kick old socket. uid: " << agent_fd.uid_ << " fd: " << af->fd_;
        }
        handle = RebindAgent(fd, ah);
    }

    if (handle == 0)
    {
        pb::UserLoginRSP    rsp;
        rsp.set_code(-4);
        WrapSkynetSocketSend(ctx(), fd, rsp, agent_fd.socket_type_, gate_);
        ShutdownClientConnect(fd);
        LOG(ERROR) << "create | rebind agent error. uid: " << agent_fd.uid_;
        return;
    }

    // 通知网关将客户端的消息转发到agent
    char forwardline[100];
    sprintf(forwardline, "forward %d :%x :0", fd, handle);
    WrapSkynetSend(ctx(), 0, gate_, PTYPE_TEXT, 0, forwardline, strlen(forwardline));

    pb::UserLoginRSP    rsp;
    rsp.set_code(0);
    rsp.set_uid(agent_fd.uid_);
    WrapSkynetSocketSend(ctx(), fd, rsp, agent_fd.socket_type_, gate_);
}
