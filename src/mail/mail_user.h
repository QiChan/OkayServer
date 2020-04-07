#ifndef __MAIL_MAIL_USER_H__
#define __MAIL_MAIL_USER_H__

#include "../common/msg_user.h"

class MailUser final : public MsgUser
{
public:
    MailUser ();
    virtual ~MailUser ();
    MailUser (MailUser const&) = delete;
    MailUser& operator= (MailUser const&) = delete; 

public:
    virtual void VLoadUserInfo() override;
};



#endif 
