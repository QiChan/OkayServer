#ifndef __ROOM_CASH_ROOM_H__
#define __ROOM_CASH_ROOM_H__

#include "room.h"

class CashRoom : public Room
{
public:
    CashRoom ();
    virtual ~CashRoom ();
    CashRoom (CashRoom const&) = delete;
    CashRoom& operator= (CashRoom const&) = delete;

public:
    virtual bool VInitService(skynet_context* ctx, const void* parm, size_t len) override;
    virtual void VGameStartEvent(uint32_t tableid, const std::string& game_id) override;
    virtual void VGameOverEvent(uint32_t tableid, const std::string& game_id, std::vector<SeatPtr>& playing_seats, SeatPtr winner, int64_t win_chips) override;

    virtual uint32_t VGetRoomRakeRate() const override { return create_param_.req().rake(); }
    virtual int64_t  VGetRoomBaseScore() const override { return create_param_.req().base_score(); }
    virtual pb::RoomBrief VGetRoomBrief() const override;

protected:
    virtual void VServerStopEvent() override;
    virtual void VUserEnterRoomEvent(UserPtr user) override;
    virtual void VUserSitDownEvent(UserPtr user, SeatPtr seat) override;
    virtual void VUserStandUPEvent(uint64_t uid, pb::SeatBrief brief) override;
    virtual void VUserLeaveTableEvent(uint64_t uid) override;
    virtual bool VCheckStartGame(uint32_t tableid) override;
    virtual void VGameStartFailEvent(uint32_t tableid) override;
    
public:
    void        RegisterCashRoomCallBack();

private:
    TablePtr    GetRoomTable(UserPtr user);
    bool        CheckShutdownCashRoom() const;
    bool        ChangeRoomUserMoneyCallBack(MessagePtr data, uint64_t uid, int64_t chips, const std::string& type);

    bool        ChangeTable(UserPtr);            // 换桌
    void        SitDown(UserPtr, int32_t seatid);

private:
    void    HandleClientMsgSitDownREQ(MessagePtr data, UserPtr user);
    void    HandleClientMsgStandUPREQ(MessagePtr data, UserPtr user);
    void    HandleClientMsgLeaveRoomAfterThisHandREQ(MessagePtr data, UserPtr user);
    void    HandleClientMsgChangeRoomREQ(MessagePtr data, UserPtr user);

    void    HandleRPCSitDownRSP(MessagePtr data, uint64_t uid, uint32_t seatid);

private:
    std::unordered_set<uint64_t>        wait_change_table_uids_;         // 下手换桌的玩家列表
    std::unordered_set<uint64_t>        wait_leave_room_uids_;          // 下手离开的玩家列表 
};

#endif
