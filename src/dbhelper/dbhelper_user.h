#ifndef __DBHELPER_DBHELPER_USER_H__
#define __DBHELPER_DBHELPER_USER_H__

#include "../common/msg_user.h"

class DBHelperUser final : public MsgUser
{
public:
    DBHelperUser ();
    virtual ~DBHelperUser ();
    DBHelperUser (DBHelperUser const&) = delete;
    DBHelperUser& operator= (DBHelperUser const&) = delete;

public:
    virtual void VLoadUserInfo() override;
};

#endif
