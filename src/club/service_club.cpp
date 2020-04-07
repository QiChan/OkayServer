#include "club.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
    static int club_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
    {
        Club* club = (Club*)ud;
        club->ServicePoll((const char*)msg, sz, source, session, type);

        return 0; //0表示成功
    }

    Club* club_create()
    {
        Club* club = new Club();
        return club;
    }

    void club_release(Club* club)
    {
        delete club;
    }

    int club_init(Club* club, struct skynet_context* ctx, char* parm)
    {
        if (!club->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
            return -1;
        }
        skynet_callback(ctx, club, club_cb);
        return 0;
    }
}
