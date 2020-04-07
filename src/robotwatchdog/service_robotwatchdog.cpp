#include "robot_watchdog.h"

extern "C"
{
#include "skynet.h"
#include "skynet_handle.h"
#include "skynet_server.h"
}


extern "C"
{
	static int robotwatchdog_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
	{
		RobotWatchdog * dog = (RobotWatchdog *)ud;
        dog->ServicePoll((const char*)msg, sz, source, session, type);
		return 0; //0 indicate success
	}

	RobotWatchdog * robotwatchdog_create()
	{
		RobotWatchdog * dog = new RobotWatchdog();
		return dog;
	}

	void robotwatchdog_release(RobotWatchdog* dog)
	{
		delete dog;
	}

	int robotwatchdog_init(RobotWatchdog* dog, struct skynet_context* ctx, char* parm)
	{
		unsigned handle = skynet_context_handle(ctx);
		char sendline[100];
		snprintf(sendline, sizeof(sendline), "0.0.0.0:0 %u %d", handle, atoi(skynet_getenv("max_online")));
		struct skynet_context* gate_ctx = skynet_context_new("gate", sendline);
		if (gate_ctx == NULL)
		{
			LOG(ERROR) << "create gate error";
			return -1;
		}
		uint32_t gate = skynet_context_handle(gate_ctx);

        char temp[100];
        sprintf(temp, "%u", gate);
        if (!dog->VInitService(ctx, temp, strlen(temp)))
        {
            return -1;
        }

		skynet_callback(ctx, dog, robotwatchdog_cb);
		skynet_handle_namehandle(gate, "gate");
		return 0;
	}
}
