#include "dbhelper_user.h"
#include "dbhelper.h"

DBHelperUser::DBHelperUser()
{
}

DBHelperUser::~DBHelperUser()
{
}

void DBHelperUser::VLoadUserInfo()
{
    auto service = static_cast<DBHelper*>(service_);
    service->HandleUserInitOK(std::dynamic_pointer_cast<DBHelperUser>(shared_from_this()));
}


