#ifndef __ROOMLIST_ROOM_LIST_H__
#define __ROOMLIST_ROOM_LIST_H__

#include "../common/msg_service.h"
#include "room_list_user.h"
#include "../pb/inner.pb.h"
#include <unordered_set>
#include <random>

class RoomInfo final
{
public:
    RoomInfo()
        : room_handle_(0),
          last_report_time_(0)
    {
    }

    RoomInfo(uint32_t handle, const pb::RoomBrief& brief)
        : room_handle_(handle),
          room_brief_(brief),
          last_report_time_(time(NULL))
    {
    }

    ~RoomInfo() = default;
    RoomInfo(const RoomInfo& obj) = delete;
    RoomInfo& operator=(const RoomInfo& obj) = delete;

public:
    const pb::RoomBrief GetRoomBrief() const { return room_brief_; }
    bool        IsAlive() const { return last_report_time_ > 0; }
    uint32_t    GetRoomHandle() const { return room_handle_; }

private:
    uint32_t        room_handle_;           // room handle
    pb::RoomBrief   room_brief_;
    time_t          last_report_time_;      // last_report_time为0时，表示房间还未创建成功
};

// 可以优化成继承MsgSlaveService
class RoomList final : public MsgService<RoomListUser>
{
    using RoomInfoPtr = std::shared_ptr<RoomInfo>;

public:
    RoomList ();
    virtual ~RoomList ();
    RoomList (RoomList const&) = delete;
    RoomList& operator= (RoomList const&) = delete;

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;
    virtual void VReloadUserInfo(UserPtr user) override;

public:
    void    LoadUserInfo(uint64_t uid);

protected:
    virtual void VServiceOverEvent() override;
    virtual void VServerStopEvent() override;

private:
    void            RegisterCallBack();
    uint32_t        GenRoomID();
    RoomInfoPtr     GetRoomInfo(uint32_t roomid);
    RoomInfoPtr     GetClubRoomInfo(uint32_t clubid, uint32_t roomid);
    uint32_t        GetClubRoomNum(uint32_t clubid) const;
    bool            CheckCreateRoomREQ(const std::shared_ptr<pb::CreateRoomREQ>&);      // 检查参数是否合法

    void            MonitorSelf();


private:
    void HandleServiceRoomBriefReport(MessagePtr data, uint32_t handle);
    void HandleServiceRoomOver(MessagePtr data, uint32_t handle);
    void HandleServiceCreateRoomRSP(MessagePtr data, uint32_t handle);

    void HandleClientMsgUserInfoREQ(MessagePtr data, UserPtr user);
    void HandleClientMsgCreateRoomREQ(MessagePtr data, UserPtr user);
    void HandleClientMsgClubRoomREQ(MessagePtr data, UserPtr user);
    void HandleClientMsgEnterRoomREQ(MessagePtr data, UserPtr user);

    MessagePtr HandleRPCGetClubRoomNumREQ(MessagePtr data);

private:
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>>    club_rooms_;     // clubid -> unordered_set<roomid>
    std::unordered_map<uint32_t, RoomInfoPtr>  all_rooms_;     // roomid -> RoomInfo
    std::default_random_engine      rd_;
};

#endif
