#include "mail.h"

Mail::Mail()
    : MsgSlaveService(MsgServiceType::SERVICE_MAIL)
{
}

Mail::~Mail()
{
}

bool Mail::VInitService(skynet_context* ctx, const void* parm, size_t len)
{
    if (!MsgSlaveService::VInitService(ctx, parm, len))
    {
        return false;
    }

    RegServiceName("mail", true);

    RegisterCallBack();

    return true;
}

void Mail::VReloadUserInfo(UserPtr user)
{
    LOG(INFO) << "[" << service_name() << "] uid: " << user->uid() << " handle: " << std::hex << skynet_context_handle(ctx());
}

void Mail::VServiceOverEvent()
{
    LOG(ERROR) << "[" << service_name() << "] shutdown.";
}

void Mail::VServerStopEvent()
{
    LOG(INFO) << "[" << service_name() << "] server stop event.";
}

void Mail::RegisterCallBack()
{
}
