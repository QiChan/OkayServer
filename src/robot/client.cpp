#include "client.h"
#include "../json/json.h"
#include <tools/text_parm.h>
#include "../pb/login.pb.h"
#include <tools/string_util.h>
#include <fstream>
#include "../common/scope_guard.h"

Client::Client()
    : client_msg_dsp_(&service_context_)
{
}

bool Client::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    if (!LoadClientVersion())
    {
        return false;
    }

	gate_ = skynet_queryname(ctx, ".gate");
	watchdog_ = skynet_queryname(ctx, ".robotwatchdog");

    DependOnService("value");
    RegServiceName("robot", false, false);

	TextParm text_parm(static_cast<const char*>(parm), len);


	RegisterCallBack();

    std::string robot_type = text_parm.get_string("robot_type");
    robot_ = RobotFactory::GetInstance()->CreateRobot(robot_type);
    if (robot_ == nullptr)
    {
        LOG(ERROR) << "create robot error. robot_type: " << robot_type;
        return false; 
    } 
	uid_ = text_parm.get_int64("uid");

    robot_->VInitRobot(this, text_parm); 
	return true;
}

void Client::VServiceStartEvent()
{
    LOG(INFO) << "robot start. uid: " << uid_;
    Connect(skynet_getenv("server_ip"), atoi(skynet_getenv("gate_port")));
    return;
}

void Client::RegisterCallBack()
{
    RegisterPTypeParsePackCTXCallBack(PTYPE_CLIENT, [](const char* data){});

    client_msg_dsp_.RegisterCallBack(pb::UserLoginRSP::descriptor()->full_name(), std::bind(&Client::HandleMsgLogin, this, std::placeholders::_1, std::placeholders::_2));
    client_msg_dsp_.RegisterCallBack(pb::HeartBeatRSP::descriptor()->full_name(), std::bind(&Client::HandleMsgHeartBeat, this, std::placeholders::_1, std::placeholders::_2));
    client_msg_dsp_.RegisterCallBack(pb::UserLogoutRSP::descriptor()->full_name(), std::bind(&Client::HandleMsgUserLogout, this, std::placeholders::_1, std::placeholders::_2));
    client_msg_dsp_.RegisterCallBack(pb::ServerStopBRC::descriptor()->full_name(), [this](MessagePtr data, uint64_t){
        ClientKillSelf();
    });
}

void Client::ClientHeartBeat()
{
	pb::HeartBeatREQ hb;
	ClientSend(hb);
	StartTimer(700, std::bind(&Client::ClientHeartBeat, this));
}

void Client::ClientSend(const Message& msg)
{
	char* data = nullptr;
	uint32_t size = 0;
	SerializeToNetData(msg, data, size);
	skynet_socket_send(ctx(), client_fd_, data, size);
	//log_debug("client send size:%d type:%s", size, msg.GetTypeName().c_str());
}

void Client::ClientKillSelf()
{
	LOG(INFO) << "robot kill self. " << uid_;
	char sendline[100];
	sprintf(sendline, "client killself");
	skynet_send(ctx(), 0, watchdog_, PTYPE_TEXT, 0, sendline, strlen(sendline));
}


void Client::Connect(const char* ip, int port)
{
    char sendline[100];
    snprintf(sendline, sizeof(sendline), "connect %s %d", ip, port);
    skynet_send(ctx(), 0, gate_, PTYPE_TEXT, 0, sendline, strlen(sendline));
}

void Client::HandleConnect(int fd)
{
    LOG(INFO) << "connect success. uid: " << uid_;
    client_fd_ = fd;
    char sendline[100];
    sprintf(sendline, "client connected %d", fd);
    skynet_send(ctx(), 0, watchdog_, PTYPE_TEXT, 0, sendline, strlen(sendline));

    pb::UserLoginREQ ul;
    ul.set_uid(uid_);
    ul.set_version(version_);
    ul.set_rdkey("robotmykey");
    ClientSend(ul);
}

void Client::RegisterCallBack(const std::string& name, const std::function<void(MessagePtr, uint64_t)>& func)
{
    client_msg_dsp_.RegisterCallBack(name, func);
}


bool Client::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    if (from == "gate")
    {
        std::string cmd;
        std::string it;
        std::tie(cmd, it) = StringUtil::DivideString(data, ' ');
        if (cmd == "connected")
        {
            HandleConnect(std::stoi(it));
        }
    }
    else if (from == "robotdog")
    {
        std::string cmd;
        std::tie(cmd, std::ignore) = StringUtil::DivideString(data, ' ');
        if (cmd == "kill")
        {
            ClientKillSelf(); 
        }
    }

    return true;
}


void Client::VHandleClientMsg(const char* data, uint32_t size, uint32_t handle, int session)
{
    DispatcherStatus status = client_msg_dsp_.DispatchClientMessage(data, size, uid_);

    if (status != DispatcherStatus::DISPATCHER_SUCCESS)
	{
/*        InPack pack;
        if (!pack.ParseFromNetData(service_context_.pack_context(), data, size))
        {
            LOG(ERROR) << "dispatch error. uid: " << uid_;
        }
        else
        {
            LOG(ERROR) << "dispatch error. uid: " << uid_ << " type: " << pack.type_name();
        }
        */
		return;
	}
}

void Client::HandleMsgLogin(MessagePtr msg, uint64_t)
{
	auto pack = std::dynamic_pointer_cast<pb::UserLoginRSP>(msg);
    if (pack->code() != 0)
    {
        LOG(INFO) << "login error. code: " << pack->code() << ", uid: " << pack->uid();
        return;
    }

	StartTimer(100, std::bind(&Client::LoginSuccessEvent, this));
}

void Client::LoginSuccessEvent()
{
    ClientHeartBeat();
	LOG(INFO) << "login success. uid: " << uid_;
    robot_->LoginSuccessEvent();
}

void Client::HandleMsgHeartBeat(MessagePtr msg, uint64_t)
{
	auto pack = std::dynamic_pointer_cast<pb::HeartBeatRSP>(msg);
}

void Client::HandleMsgUserLogout(MessagePtr msg, uint64_t)
{
    LOG(INFO) << "user logout. uid: " << uid_;
    ClientKillSelf(); 
}

bool Client::LoadClientVersion()
{
    std::ifstream ifs;
    ifs.open("./client_publish/version.json");
    if (!ifs)
    {
        LOG(ERROR) << "open version.conf failed.";
        return false;
    }

    ScopeGuard guard([]{}, [&ifs]{ ifs.close(); });

    Json::Reader    reader;
    Json::Value     root;
    if (!reader.parse(ifs, root))
    {
        LOG(ERROR) << "parse version.conf error";
        return false;
    }
    version_ = root["client_ver"].asString();
    LOG(INFO) << "load client version success. version: " << version_;
    return true;
}
