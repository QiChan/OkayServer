#ifndef __ROOM_ACTION_COMPONENT_H__
#define __ROOM_ACTION_COMPONENT_H__

#include "seat_iterator.h"
#include "../pb/table.pb.h"

class Table;
class ActionComponent
{
public:
    ActionComponent (Table*);
    virtual ~ActionComponent ();
    ActionComponent (ActionComponent const&) = delete;
    ActionComponent& operator= (ActionComponent const&) = delete;

public:
    void    Reset();
    void    ChooseDealer(uint32_t seat_num);
    void    NextAction(SeatIterator::SeatPtr curr);
    int32_t GetActionSeatID();
    pb::ActionBrief         GetActionBrief();
    SeatIterator::SeatPtr   GetActionSeat();
    SeatIterator::SeatPtr   GetPrevSeat();

    void    HandleHostSeatEvent(SeatIterator::SeatPtr seat);

    void    ActionTimeout();

private:
    Table*      table_;
    std::shared_ptr<ActionSeatIterator>     action_iter_;
    uint64_t                                start_time_;
    int                                     action_timer_;
};
#endif
