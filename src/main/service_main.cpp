#include <glog/logging.h>
#include "main.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}


extern "C"
{
    static int main_cb(struct skynet_context* ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)
    {
        Main* main = (Main*)ud;
        main->ServicePoll((const char*)msg, sz, source, session, type);
        return 0;
    }

    Main* main_create()
    {
        Main* main = new Main();
        return main;
    } 

    void main_release(Main* main)
    {
        delete main;
    }

    int main_init(Main* main, struct skynet_context* ctx, char* parm)
    {
        if (!main->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
            return -1;
        }
        skynet_callback(ctx, main, main_cb);
        return 0;
    }
}
