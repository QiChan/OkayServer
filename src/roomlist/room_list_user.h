#ifndef __ROOMLIST_ROOM_LIST_USER_H__
#define __ROOMLIST_ROOM_LIST_USER_H__

#include "../common/msg_user.h"

class RoomListUser final : public MsgUser
{
public:
    RoomListUser ();
    virtual ~RoomListUser ();
    RoomListUser (RoomListUser const&) = delete;
    RoomListUser& operator= (RoomListUser const&) = delete;

public:
    virtual void VLoadUserInfo() override;

    void    CompleteRoomListUser(const std::string& name, const std::string& icon);

public:
    const std::string& name() const { return name_; }
    const std::string& icon() const { return icon_; }

private:
    std::string     name_;
    std::string     icon_;
};

#endif
