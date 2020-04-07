#include "monitor.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

using namespace std;

extern "C"
{
	static int monitor_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
	{
		Monitor* monitor = (Monitor*)ud;
		monitor->ServicePoll((const char*)msg, sz, source, session, type);
		return 0; //0表示成功
	}

	Monitor* monitor_create()
	{
		Monitor* monitor = new Monitor();
		return monitor;
	}

	void monitor_release(Monitor* monitor)
	{
		delete monitor;
	}

	int monitor_init(Monitor* monitor, struct skynet_context* ctx, char* parm)
	{
		if (!monitor->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
			return -1;
        }
		skynet_callback(ctx, monitor, monitor_cb);
		return 0;
	}
}
