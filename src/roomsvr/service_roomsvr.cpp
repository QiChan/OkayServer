#include "roomsvr.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
    static int roomsvr_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
    {
        RoomSvr* roomsvr = (RoomSvr*)ud;
        roomsvr->ServicePoll((const char*)msg, sz, source, session, type);

        return 0; //0表示成功
    }

    RoomSvr* roomsvr_create()
    {
        RoomSvr* roomsvr = new RoomSvr();
        return roomsvr;
    }

    void roomsvr_release(RoomSvr* roomsvr)
    {
        delete roomsvr;
    }

    int roomsvr_init(RoomSvr* roomsvr, struct skynet_context* ctx, char* parm)
    {
        if (!roomsvr->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
            return -1;
        }
        skynet_callback(ctx, roomsvr, roomsvr_cb);
        return 0;
    }
}
