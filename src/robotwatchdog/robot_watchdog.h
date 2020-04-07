#ifndef __ROBOTWATCHDOG_ROBOT_WATCHDOG_H__
#define __ROBOTWATCHDOG_ROBOT_WATCHDOG_H__

#include "../common/service.h"
#include <tools/text_parm.h>

struct RobotData
{
	RobotData(uint64_t uid)
	{
		uid_ = uid;
	}
	uint64_t uid_;
};

struct ClientHandle
{
	ClientHandle()
	{
		data_ = nullptr;
	}

	uint32_t handle_;
	int fd_;
    std::shared_ptr<RobotData> data_;
};

class RobotWatchdog final : public Service
{
public:
	RobotWatchdog();
    ~RobotWatchdog() = default;
    RobotWatchdog(const RobotWatchdog&) = delete;
    RobotWatchdog& operator= (const RobotWatchdog&) = delete;


public:
	virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

public:
    void    WebCreateRobot(TextParm& parm);
    void    WebKickAll();
	void    ShutdownClient(uint32_t client);

public:
    void    HandleConnect(uint32_t client, int fd);
    void    HandleDisconnect(uint32_t client);

private:
    void    DelayCreateRobot(int time, TextParm& parm, uint64_t uid);
    void    CreateRobot(TextParm& parm, uint64_t uid);
    void    MonitorSelf();

protected:
    virtual void VServiceStartEvent() override;
    virtual void VServiceOverEvent() override;
    virtual void VServerStopEvent() override;
    virtual bool VHandleTextMessage(const std::string& from, const std::string& data, uint32_t source, int session) override;

private:
	uint32_t gate_;
	std::unordered_map<uint32_t, ClientHandle> client_handles_;
};

#endif
