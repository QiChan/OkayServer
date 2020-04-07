#include "mail_user.h"
#include "mail.h"

MailUser::MailUser()
{
}

MailUser::~MailUser()
{
}

void MailUser::VLoadUserInfo()
{
    auto service = static_cast<Mail*>(service_);
    service->HandleUserInitOK(std::dynamic_pointer_cast<MailUser>(shared_from_this()));
}
