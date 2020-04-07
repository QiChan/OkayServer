#include "room_list.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

using namespace std;

extern "C"
{
	static int roomlist_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
	{
		RoomList* roomlist = (RoomList*)ud;
		roomlist->ServicePoll((const char*)msg, sz, source, session, type);
		
		return 0; //0表示成功
	}

	RoomList* roomlist_create()
	{
		RoomList* roomlist = new RoomList();
		return roomlist;
	}

	void roomlist_release(RoomList* roomlist)
	{
		delete roomlist;
	}

	int roomlist_init(RoomList* roomlist, struct skynet_context* ctx, char* parm)
	{
		if (!roomlist->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
			return -1;
        }
		skynet_callback(ctx, roomlist, roomlist_cb);
		return 0;
	}
}



