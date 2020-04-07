#include "room_user.h"
#include "room.h"

RoomUser::RoomUser()
    : clubid_(0)
{
}

RoomUser::~RoomUser()
{
}

void RoomUser::VLoadUserInfo()
{
}


void RoomUser::CompleteRoomUser(const std::string& ip, uint32_t clubid)
{
    SetIP(ip);
    clubid_ = clubid;

    auto service = static_cast<Room*>(service_);
    service->HandleUserInitOK(GetRoomUserPtr());
}


pb::SeatUserBrief RoomUser::GetSeatUserBrief(std::shared_ptr<RoomUser> user)
{
    pb::SeatUserBrief   brief;

    auto self_user = GetRoomUserPtr();
    auto room = static_cast<Room*>(service_);
    if (!room->IsAnonymousRoom() || user == self_user)
    {
        brief.set_uid(uid());
        brief.set_clubid(clubid_);
        brief.set_name(name_);
        brief.set_icon(icon_);
    }
    return brief;
}

bool RoomUser::SitDown(int32_t seatid, int64_t chips, int64_t last_money)
{
    auto table = GetUserTable();
    if (table == nullptr)
    {
        // 必须在房间里面才能坐下
        return false;
    }

    auto seat = GetUserSeat();
    if (seat != nullptr)
    {
        return false;
    }

    auto ret = table->UserSitDown(GetRoomUserPtr(), seatid, chips, last_money);

    if (!std::get<0>(ret))
    {
        return false;
    }

    seat = std::get<1>(ret);

    auto room = static_cast<Room*>(service_);
    room->VUserSitDownEvent(GetRoomUserPtr(), seat);
    return true;
}

bool RoomUser::EnterTable(TablePtr table)
{
    if (table == nullptr)
    {
        return false;
    }

    auto user_table = GetUserTable();
    if (user_table != nullptr && table != user_table)
    {
        // 已经在房间里面，只允许进入所在的房间
        return false;
    }

    if (!table->UserEnterTable(GetRoomUserPtr()))
    {
        return false;
    }

    auto room = static_cast<Room*>(service_);
    room->VUserEnterTableEvent(GetRoomUserPtr(), table);
    return true;
}

bool RoomUser::StandUP()
{
    auto table = GetUserTable();
    auto seat = GetUserSeat();
    if (table == nullptr || seat == nullptr)
    {
        return false;
    }

    pb::SeatBrief brief = seat->GetSeatBrief();
    if (!table->UserStandUP(GetRoomUserPtr()))
    {
        return false;
    }

    auto room = static_cast<Room*>(service_);
    room->VUserStandUPEvent(uid(), brief);
    return true;
}

bool RoomUser::LeaveTable()
{
    auto user_table = GetUserTable();

    // 玩家不在房间直接返回true
    if (user_table == nullptr)
    {
        return true;
    }

    // 如果玩家有座位需要先执行站起操作, 站起失败离开房间也会返回false
    auto user_seat = GetUserSeat();
    if (user_seat != nullptr)
    {
        if (!StandUP())
        {
            return false;
        }
    }

    // 站起成功之后，这里一定会离开成功
    if (!user_table->UserLeaveTable(GetRoomUserPtr()))
    {
        LOG(ERROR) << GetRoomUserPtr() << " leave table failed. tableid: " << user_table->GetTableID();
        return false; 
    }

    // 返回true时才可以将table置为nullptr
    auto room = static_cast<Room*>(service_);

    room->VUserLeaveTableEvent(uid());
    return true;
}

bool RoomUser::HostSeat()
{
    auto table = GetUserTable();
    if (table == nullptr)
    {
        return false;
    }
    return table->UserHostSeat(GetRoomUserPtr());
}

RoomUser::TablePtr RoomUser::GetUserTable()
{
    auto room = static_cast<Room*>(service_);
    return room->GetUserTable(uid());
}

RoomUser::SeatPtr RoomUser::GetUserSeat()
{
    auto room = static_cast<Room*>(service_);
    auto seat = room->GetUserSeat(uid());

    if (seat != nullptr && seat->GetSeatUID() != uid())
    {
        LOG(ERROR) << GetRoomUserPtr() << " seat not belong user. seat_uid: " << seat->GetSeatUID();
        return nullptr;
    }
    return seat;
}

inline std::ostream& operator<< (std::ostream& os, std::shared_ptr<RoomUser> user)
{
    if (user == nullptr)
    {
        os << " user is nullptr";
        return os;
    }

    os << " uid: " << user->uid() << " clubid: " << user->GetClubID();
    auto table = user->GetUserTable();
    if (table != nullptr)
    {
        os << " tableid: " << table->GetTableID();
    }
    auto seat = user->GetUserSeat();
    if (seat != nullptr)
    {
        os << " seatid: " << seat->GetSeatID();
    }
    return os;
}
