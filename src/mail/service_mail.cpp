#include "mail.h"
#include <tools/string_util.h>

extern "C"
{
#include "skynet.h"
}

extern "C"
{
    static int mail_cb(struct skynet_context* ctx, void* ud, int type, int session, uint32_t source, const void* msg, size_t sz)
    {
        Mail* mail = (Mail*)ud;
        mail->ServicePoll((const char*)msg, sz, source, session, type);
        return 0;
    }

    Mail* mail_create()
    {
        Mail* mail = new Mail();
        return mail;
    }

    void mail_release(Mail* mail)
    {
        delete mail;
    }

    int mail_init(Mail* mail, struct skynet_context* ctx, char* parm)
    {
        if (!mail->VInitService(ctx, parm, StringUtil::StringLen(parm)))
        {
            return -1;
        }
        skynet_callback(ctx, mail, mail_cb);
        return 0;
    }
}
