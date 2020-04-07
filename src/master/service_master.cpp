#include "master.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
	static int master_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
	{
		Master* master = (Master*)ud;
		master->ServicePoll((const char*)msg, sz, source, session, type);
		return 0; //0表示成功
	}

	Master* master_create()
	{
		Master* master = new Master();
		return master;
	}

	void master_release(Master* master)
	{
		delete master;
	}

	int master_init(Master* master, struct skynet_context* ctx, char* parm)
	{
		if (!master->VInitService(ctx, parm, StringUtil::StringLen(parm)))
			return -1;
		skynet_callback(ctx, master, master_cb);
		return 0;
	}
}






