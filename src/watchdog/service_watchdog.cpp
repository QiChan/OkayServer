#include <glog/logging.h>
#include "watchdog.h"

extern "C"
{
#include "skynet.h"
#include "skynet_handle.h"
#include "skynet_server.h"
}

extern "C"
{
    static int watchdog_cb(struct skynet_context* ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)
    {
        Watchdog* dog = (Watchdog*)ud;
        dog->ServicePoll((const char*)msg, sz, source, session, type);
        return 0;
    }

    Watchdog* watchdog_create()
    {
        Watchdog* dog = new Watchdog();
        return dog;
    }

    void Watchdog_release(Watchdog* dog)
    {
        delete dog;
    }

    int watchdog_init(Watchdog* dog, struct skynet_context* ctx, char* parm)
    {
        uint32_t handle = skynet_context_handle(ctx);
        char sendline[100];
        snprintf(sendline, sizeof(sendline), "%s %u %d", parm, handle, atoi(skynet_getenv("max_online")));
        struct skynet_context* gate_ctx = skynet_context_new("gate", sendline);
        if (gate_ctx == nullptr)
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
        skynet_callback(ctx, dog, watchdog_cb);
        return 0;
    }
}
