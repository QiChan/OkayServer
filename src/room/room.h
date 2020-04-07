#ifndef __ROOM_ROOM_H__
#define __ROOM_ROOM_H__

#include "table.h"
#include "room_user.h"
#include "counter_component.h"
#include "../common/msg_service.h"
#include "../pb/inner.pb.h"


class Room : public MsgService<RoomUser>
{
public:
    Room ();
    virtual ~Room ();
    Room (Room const&) = delete;
    Room& operator= (Room const&) = delete;

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;

protected:
    using TablePtr = std::shared_ptr<Table>;
    using SeatPtr = std::shared_ptr<Seat>;

    // 继承自Service的事件
    virtual void VServiceStartEvent() override;
    virtual void VServiceOverEvent() override;
    virtual void VServerStopEvent() override;


    virtual void VUserDisconnectEvent(UserPtr user) override;
    virtual void VUserReleaseEvent(UserPtr user) override;
    virtual void VUserRebindEvent(UserPtr user) override;
    virtual void VReloadUserInfo(UserPtr user) override {}

public:
    /*
     * 各类事件接口
     * 1. 子类重写时需要先调用父类的函数
     * 2. 必须是动作已经执行成功之后才会触发对应的时间
     */
    virtual void VUserEnterRoomEvent(UserPtr user);      // 用户进入房间事件
    virtual void VUserLeaveRoomEvent(UserPtr user);      // 用户离开房间事件
    virtual void VUserSitDownEvent(UserPtr user, SeatPtr seat);        // 用户坐下事件
    virtual void VUserEnterTableEvent(UserPtr user, TablePtr table);     // 用户进入牌桌事件
    virtual void VUserStandUPEvent(uint64_t uid, pb::SeatBrief brief);  // 用户站起事件
    virtual void VUserLeaveTableEvent(uint64_t uid);     // 用户离开牌桌事件

    virtual bool VCheckStartGame(uint32_t tableid);      // 检查是否可以开始游戏
    virtual void VGameStartFailEvent(uint32_t tableid);  // 游戏开始失败事件
    virtual void VGameStartEvent(uint32_t tableid, const std::string& game_id);      // 游戏开始事件
    virtual void VGameOverEvent(uint32_t tableid, const std::string& game_id, std::vector<SeatPtr>& playing_seats, SeatPtr winner, int64_t win_chips);       // 游戏结束事件

public:
    virtual uint32_t VGetRoomRakeRate() const = 0;
    virtual int64_t  VGetRoomBaseScore() const = 0;
    virtual pb::RoomBrief VGetRoomBrief() const;

public:
    bool        IsAnonymousRoom() const { return create_param_.req().anony(); }
    void        SetCreateParam(const pb::iCreateRoomREQ& param) { create_param_ = param; }
    uint32_t    GetRoomID() const { return create_param_.roomid(); }
    uint32_t    GetRoomClubID() const { return create_param_.req().clubid(); }
    uint32_t    GetActionTime() const { return create_param_.req().action_time(); }
    uint64_t    GetRoomOwnerID() const { return create_param_.uid(); }
    uint32_t    GetRoomHandle() const { return skynet_context_handle(ctx()); }
    pb::RoomType        GetRoomType() const { return create_param_.req().room_type(); }
    pb::RoomMode        GetRoomMode() const { return create_param_.req().room_mode(); }
    uint32_t            GetSeatNum() const { return create_param_.req().seat_num(); }
    uint32_t            GetSitedNum() const;
    const std::string&  GetRoomName() const { return create_param_.req().room_name(); }
    const std::string&  GetRoomSetID() const { return room_set_id_; }
    TablePtr            GetUserTable(uint64_t uid) { return counter_.GetUserTable(uid); }
    SeatPtr             GetUserSeat(uint64_t uid) { return counter_.GetUserSeat(uid); }

    bool        KickRoomUser(UserPtr user);     // 将用户踢出房间, 调用之后, user不可再用
    void        SendToConcurrentService(const Message& msg, uint32_t handle);


protected:
    void        RegisterCallBack();

    UserPtr     NewRoomUser(const std::shared_ptr<pb::iEnterRoom>& msg);
    void        DelRoomUser(uint64_t uid);      // 调用之后, user不可再用
    TablePtr    NewRoomTable();
    TablePtr    GetTable(uint32_t tableid);

protected:
    void    ChangeRoomUserMoney(int64_t money, uint64_t uid, const std::string& type, const std::string& attach, const RPCCallBack& func = nullptr);
    void    ResetRoomSetID();
    void    DelayShutdownRoom(uint32_t seconds);

private:
    void    HandleServiceEnterRoom(MessagePtr data, uint32_t handle);

    void    HandleClientMsgLeaveRoomREQ(MessagePtr data, UserPtr user);
    void    HandleClientMsgActionREQ(MessagePtr data, UserPtr user);

private:
    void                    PeriodicallyReportSelf();
    pb::iAgentRoomStatus    FillAgentRoomStatus(bool enter, uint32_t clubid) const;
    pb::iRoomRecord         GetRoomRecord() const;

private:
    bool                    is_shutdown_;
    time_t                  create_time_;

protected:
    pb::iCreateRoomREQ      create_param_;
    std::string             room_set_id_;       // 房间唯一id
    CounterComponent        counter_;
    std::unordered_map<uint32_t, TablePtr>      tables_;    
};

#define ROOM_LOG(obj, level) room_log(obj, LOG(level), __func__)
#define ROOM_XLOG(level) ROOM_LOG(this, level)

inline std::ostream& room_log(Room* room, std::ostream& os, const std::string& func)
{
    if (room == nullptr)
    {
        LOG(ERROR) << func << " room is null. ";
        return os;
    }
    os << func << "() ";
    os << "roomid: " << room->GetRoomID() << "|";
    return os;
}

inline std::ostream& room_log(Table* table, std::ostream& os, const std::string & func)
{
    if (table == nullptr)
    {
        LOG(ERROR) << func << " table is NULL. ";
        return os;
    }

    os << func << "() ";
    os << "roomid: " << table->GetRoomID() << " tableid: " << table->GetTableID();
    os << "|";
    return os;
} 

class RoomShell final
{
    public:
        RoomShell()
            : room_(nullptr)
        {
        }
        ~RoomShell() = default;
        RoomShell(const RoomShell&) = delete;
        RoomShell& operator= (const RoomShell&) = delete;

        std::shared_ptr<Room>   room_;
};

#endif
