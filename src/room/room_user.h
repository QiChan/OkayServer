#ifndef __ROOM_ROOM_USER_H__
#define __ROOM_ROOM_USER_H__

#include "../common/msg_user.h"
#include "../pb/table.pb.h"

class Table;
class Seat;

class RoomUser : public MsgUser
{
public:
    using TablePtr = std::shared_ptr<Table>;
    using SeatPtr = std::shared_ptr<Seat>;

public:
    RoomUser ();
    virtual ~RoomUser ();
    RoomUser (RoomUser const&) = delete;
    RoomUser& operator= (RoomUser const&) = delete; 

public:
    bool        SitDown(int32_t seatid, int64_t chips, int64_t last_money);
    bool        StandUP();
    bool        EnterTable(TablePtr table);
    bool        LeaveTable();
    bool        HostSeat();     // 托管座位

public:
    uint32_t    GetClubID() const { return clubid_; }
    TablePtr    GetUserTable();
    SeatPtr     GetUserSeat();
    const std::string& name() const { return name_; }
    const std::string& icon() const { return icon_; }


    void        CompleteRoomUser(const std::string& ip, uint32_t clubid);
    
    pb::SeatUserBrief GetSeatUserBrief(std::shared_ptr<RoomUser> user);

    std::shared_ptr<RoomUser> GetRoomUserPtr() { return std::dynamic_pointer_cast<RoomUser>(shared_from_this()); }

public:
    virtual void VLoadUserInfo() override;

private:
    uint32_t        clubid_;
    std::string     name_;
    std::string     icon_;
};

std::ostream& operator<< (std::ostream& os, std::shared_ptr<RoomUser> user);

#endif
