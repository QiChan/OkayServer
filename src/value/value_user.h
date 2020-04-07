#ifndef __VALUE_VALUE_USER_H__
#define __VALUE_VALUE_USER_H__

#include "../common/msg_user.h"

class ValueUser final : public MsgUser
{
    public:
        ValueUser();
        virtual ~ValueUser();
        ValueUser(const ValueUser&) = delete;
        ValueUser& operator= (const ValueUser&) = delete;

    public:
        virtual void VLoadUserInfo() override;
        void    CompleteValueUser();
};

#endif 
