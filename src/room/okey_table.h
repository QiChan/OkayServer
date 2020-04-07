#ifndef __ROOM_OKEY_TABLE_H__
#define __ROOM_OKEY_TABLE_H__

#include "table.h"

class OkeyTable : public Table
{
public:
    OkeyTable (Room*, uint32_t tableid, uint32_t seat_num);
    virtual ~OkeyTable () = default;
    OkeyTable (OkeyTable const&) = delete;
    OkeyTable& operator= (OkeyTable const&) = delete;

protected:
    virtual void VDealCards() override;
    virtual void VSystemAction() override;
    virtual void VStatGameRecord(SeatPtr winner, RepeatedPtrField<pb::CardGroup>* cards_ptr, pb::iGameRecord& record, std::vector<SeatPtr>& playing_seats, int64_t& win_chips) override;
    virtual void VHandleUserAction(std::shared_ptr<pb::ActionREQ> msg, RoomUserPtr user) override;

private:
    void    UserEatCard(RoomUserPtr);
    void    UserTakeCard(RoomUserPtr);
    void    UserThrowCard(RoomUserPtr, std::shared_ptr<pb::ActionREQ> msg);
    void    UserCompleteCard(RoomUserPtr, std::shared_ptr<pb::ActionREQ> msg);
};

class OkeyTable101 : public Table
{
public:
    OkeyTable101 (Room*, uint32_t tableid, uint32_t seat_num);
    virtual ~OkeyTable101 () = default;
    OkeyTable101 ( OkeyTable101 const&) = delete;
    OkeyTable101& operator= ( OkeyTable101 const&) = delete;

protected:
    virtual void VDealCards() override;
    virtual void VSystemAction() override;
    virtual void VStatGameRecord(SeatPtr winner, RepeatedPtrField<pb::CardGroup>* cards_ptr, pb::iGameRecord& record, std::vector<SeatPtr>& playing_seats, int64_t& win_chips) override;
    virtual void VHandleUserAction(std::shared_ptr<pb::ActionREQ> msg, RoomUserPtr user) override;
};
#endif

