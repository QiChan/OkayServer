#ifndef __MAIL_MAIL_H__
#define __MAIL_MAIL_H__

#include "../common/msg_slave_service.h"
#include "../common/mysql_client.h"
#include "mail_user.h"

class Mail final : public MsgSlaveService<MailUser>
{
public:
    Mail ();
    virtual ~Mail ();
    Mail (Mail const&) = delete;;
    Mail& operator= (Mail const&) = delete; 

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;
    virtual void VReloadUserInfo(UserPtr user) override;

protected:
    virtual void VServiceOverEvent() override;
    virtual void VServerStopEvent() override;

private:
    void    RegisterCallBack();

private:
    /* data */
};

#endif
