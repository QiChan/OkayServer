#include "agent.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
    static int agent_cb(struct skynet_context* ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)
    {
        Agent* agent = (Agent*)ud;
        agent->ServicePoll((const char*)msg, sz, source, session, type);
        return 0;
    }

    Agent* agent_create()
    {
        Agent* agent = new Agent();
        return agent;
    }

    void agent_release(Agent* agent)
    {
        delete agent;
    }

    int agent_init(Agent* agent, struct skynet_context* ctx, char* parm)
    {
        if (!agent->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
            return -1;
        }
        skynet_callback(ctx, agent, agent_cb);
        return 0;
    }
}
