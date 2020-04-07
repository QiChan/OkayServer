#include "robot_watchdog.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet_server.h"
#include "skynet_handle.h"
}

RobotWatchdog::RobotWatchdog()
{
}

bool RobotWatchdog::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!Service::VInitService(ctx, parm, len))
    {
        return false;
    }

    sscanf((const char*)parm, "%u", &gate_);
    RegServiceName("robotwatchdog", true, false);

    return true;
}

void RobotWatchdog::WebCreateRobot(TextParm& parm)
{
	int num = parm.get_int("num");
	if (num == 0)
		num = 1;
	int64_t from_uid = parm.get_int64("from_uid");
	if (from_uid <= 0)
	{
        LOG(ERROR) << "from_uid must bigger than zero. from_uid: " << from_uid;
        return; 
	}

	int time = 0;
    for (int i = 0; i < num; ++i, ++from_uid)
    {
        DelayCreateRobot(time, parm, from_uid);
        time += 10;
    }
}

void RobotWatchdog::DelayCreateRobot(int time, TextParm& parm, uint64_t uid)
{
	StartTimer(time, std::bind(&RobotWatchdog::CreateRobot, this, parm, uid));
}

void RobotWatchdog::CreateRobot(TextParm& parm, uint64_t uid)
{
	parm.insert_int("uid", uid);

	const char* strParam = parm.serilize();
	skynet_context* ctx = skynet_context_new("robot", strParam);
    uint32_t handle = 0;
    if (ctx)
    {
        handle = skynet_context_handle(ctx);
    }
	if (handle == 0)
    {
        LOG(ERROR) << "create robot error. parm: " << strParam; 
		return;
    }
    auto data = std::make_shared<RobotData>(uid);
	ClientHandle& h = client_handles_[handle];
	h.handle_ = handle;
	h.data_ = data;

    LOG(INFO) << "create robot success. " << strParam;
}

void RobotWatchdog::VServiceStartEvent()
{
    skynet_handle_namehandle(skynet_context_handle(ctx()), "robotwatchdog");
    LOG(INFO) << "[" << service_name() << "] start.";
    MonitorSelf();
}

void RobotWatchdog::VServiceOverEvent()
{
}

void RobotWatchdog::VServerStopEvent()
{
}

bool RobotWatchdog::VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session)
{
    if (from == "gate")
    {
        std::string cmd;
        std::string it;
        std::tie(cmd, it) = StringUtil::DivideString(data, ' ');
        if (cmd == "close")
        {

            std::string client_str;
            std::tie(std::ignore, client_str) = StringUtil::DivideString(it, ' ');
            uint32_t client = std::stoul(client_str);
            HandleDisconnect(client);
        }
    }
    else if (from == "php")
    {
        TextParm parm(data.c_str());
        const char* cmd = parm.get("cmd");
        if (strcmp(cmd, "createrobot") == 0)
        {
            WebCreateRobot(parm);
        }
        else if (strcmp(cmd, "kill") == 0)
        {
            WebKickAll();
        }
        string response = "ok\n";
        ResponseTextMessage(from, response);
    }
    else if (from == "client")
    {
        std::string cmd;
        std::string it;
        std::tie(cmd, it) = StringUtil::DivideString(data, ' ');
        if (cmd == "connected")
        {
            HandleConnect(source, std::stoi(it));
        }
        else if (cmd == "killself")
        {
            ShutdownClient(source);
        }
    }
    else
    {
        LOG(ERROR) << "unknow text command, from: " << from << " data: " << data;
    }
    return true;
}

void RobotWatchdog::HandleConnect(uint32_t client, int fd)
{
	ClientHandle& h = client_handles_[client];
	h.fd_ = fd;
    LOG(INFO) << "[" << service_name() << "] robot: " << h.data_->uid_ << " connected. fd: " << fd;
}

void RobotWatchdog::HandleDisconnect(uint32_t client)
{
	auto it = client_handles_.find(client);
	if (it == client_handles_.end())
		return;

    LOG(INFO) << "[" << service_name() << "] disconnect. fd: " << it->second.fd_;
	client_handles_.erase(client);
	if (skynet_handle_retire(client) == 1)
    {
		LOG(INFO) << "retire ok.";
    }
	else
    {
		LOG(ERROR) << "retire FALSE.";
    }
}

void RobotWatchdog::ShutdownClient(uint32_t client)
{
	auto it = client_handles_.find(client);
    if (it == client_handles_.end())
    {
        return;
    }

	char sendline[100];
	sprintf(sendline, "kick %d", it->second.fd_);
	skynet_send(ctx(), 0, gate_, PTYPE_TEXT, 0, sendline, strlen(sendline));
    LOG(INFO) << "[" << service_name() << "] shutdown uid: " << it->second.data_->uid_;
}

void RobotWatchdog::WebKickAll()
{
	char sendline[100];
	sprintf(sendline, "robotdog kill");
    for (auto& elem : client_handles_)
    {
		skynet_send(ctx(), 0, elem.first, PTYPE_TEXT, 0, sendline, strlen(sendline));
	}
    LOG(INFO) << "[" << service_name() << "] kick all.";
}

void RobotWatchdog::MonitorSelf()
{
	LOG(INFO) << "=====RobotWatchDog Self Monitor=====";
    LOG(INFO) << "client_handles size: " << client_handles_.size();
	StartTimer(10 * 60 * 100, std::bind(&RobotWatchdog::MonitorSelf, this));
}


