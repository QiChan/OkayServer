#include "persistence.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
    static int persistence_cb(struct skynet_context* ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)
    {
        Persistence* persistence = (Persistence*)ud;
        persistence->ServicePoll((const char*)msg, sz, source, session, type);
        return 0;
    }

    Persistence* persistence_create()
    {
        Persistence* persistence = new Persistence();
        return persistence;
    }

    void persistence_release(Persistence* persistence)
    {
        delete persistence;
    }

    int persistence_init(Persistence* persistence, struct skynet_context* ctx, char* parm)
    {
        if (!persistence->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
            return -1;
        }
        skynet_callback(ctx, persistence, persistence_cb);
        return 0;
    }
}
