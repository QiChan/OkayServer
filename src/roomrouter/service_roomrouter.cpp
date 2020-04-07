#include "room_router.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

using namespace std;

extern "C"
{
	static int roomrouter_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
	{
		RoomRouter* roomrouter = (RoomRouter*)ud;
		roomrouter->ServicePoll((const char*)msg, sz, source, session, type);
		
		return 0; //0表示成功
	}

	RoomRouter* roomrouter_create()
	{
		RoomRouter* roomrouter = new RoomRouter();
		return roomrouter;
	}

	void roomrouter_release(RoomRouter* roomrouter)
	{
		delete roomrouter;
	}

	int roomrouter_init(RoomRouter* roomrouter, struct skynet_context* ctx, char* parm)
	{
		if (!roomrouter->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
			return -1;
        }
		skynet_callback(ctx, roomrouter, roomrouter_cb);
		return 0;
	}
}



