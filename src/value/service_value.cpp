#include "value_service.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
    static int value_cb(struct skynet_context * ctx, void * ud, int type, int session, uint32_t source, const void * msg, size_t sz)
    {
        ValueService* value = (ValueService*)ud;
        value->ServicePoll((const char*)msg, sz, source, session, type);

        return 0; //0表示成功
    }

    ValueService* value_create()
    {
        ValueService* value = new ValueService();
        return value;
    }

    void value_release(ValueService* value)
    {
        delete value;
    }

    int value_init(ValueService* value, struct skynet_context* ctx, char* parm)
    {
        if (!value->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
            return -1;
        }
        skynet_callback(ctx, value, value_cb);
        return 0;
    }
}
