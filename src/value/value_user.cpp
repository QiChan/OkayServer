#include "value_user.h"
#include "value_service.h"

ValueUser::ValueUser()
{
}

ValueUser::~ValueUser()
{
}

void ValueUser::VLoadUserInfo()
{
    CompleteValueUser();
}

void ValueUser::CompleteValueUser()
{
    auto service = static_cast<ValueService*>(service_);
    service->HandleUserInitOK(std::dynamic_pointer_cast<ValueUser>(shared_from_this()));
}
