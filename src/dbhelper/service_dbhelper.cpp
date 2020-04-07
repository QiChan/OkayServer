#include "dbhelper.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
    static int dbhelper_cb(struct skynet_context* ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)
    {
        DBHelper* dbhelper = (DBHelper*)ud;
        dbhelper->ServicePoll((const char*)msg, sz, source, session, type);
        return 0;
    }

    DBHelper* dbhelper_create()
    {
        DBHelper* dbhelper = new DBHelper();
        return dbhelper;
    }

    void dbhelper_release(DBHelper* dbhelper)
    {
        delete dbhelper;
    }

    int dbhelper_init(DBHelper* dbhelper, struct skynet_context* ctx, char* parm)
    {
        if (!dbhelper->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
            return -1;
        }
        skynet_callback(ctx, dbhelper, dbhelper_cb);
        return 0;
    }
}
