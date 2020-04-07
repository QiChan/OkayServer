#include "room_list_user.h"
#include "room_list.h"

RoomListUser::RoomListUser()
{
}

RoomListUser::~RoomListUser()
{
}

void RoomListUser::VLoadUserInfo()
{
    auto service = static_cast<RoomList*>(service_);
    service->LoadUserInfo(uid());
}

void RoomListUser::CompleteRoomListUser(const std::string& name, const std::string& icon)
{
    name_ = name;
    icon_ = icon;

    auto service = static_cast<RoomList*>(service_);
    service->HandleUserInitOK(std::dynamic_pointer_cast<RoomListUser>(shared_from_this()));
}
