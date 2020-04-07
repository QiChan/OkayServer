#include "action_component.h"
#include "table.h"
#include <tools/real_rand.h>

ActionComponent::ActionComponent(Table* table)
    : table_(table)
{
    Reset();
}

ActionComponent::~ActionComponent()
{
    Reset();
}

void ActionComponent::Reset()
{
    table_->StopTimer(action_timer_);
    action_iter_ = nullptr;
    start_time_ = 0;
}

void ActionComponent::ChooseDealer(uint32_t seat_num)
{
    uint32_t begin = RealRand::GetInstance()->Rand(0, seat_num - 1);
    action_iter_ = std::make_shared<ActionSeatIterator>(table_, begin);
}

SeatIterator::SeatPtr ActionComponent::GetActionSeat()
{
    if (action_iter_ == nullptr)
    {
        return nullptr;
    }

    return *(*action_iter_);
}

SeatIterator::SeatPtr ActionComponent::GetPrevSeat()
{
    if (action_iter_ == nullptr)
    {
        return nullptr;
    }

    return action_iter_->prev_seat();
}

void ActionComponent::NextAction(SeatIterator::SeatPtr curr)
{
    if (action_iter_ == nullptr)
    {
        return;
    }
    ++(*action_iter_);

    int timeout = 0;


    auto action_seat = GetActionSeat();
    if (curr == nullptr || action_seat->IsIdleState())     // 第一阶段
    {
        start_time_ = time(NULL);

        timeout = table_->GetActionTime();
    }
    else            // 第二阶段
    {
        uint64_t now = time(NULL);
        auto last_time = start_time_ + table_->GetActionTime();
        if (now >= last_time)
        {
            timeout = 0;
        }
        else
        {
            timeout = last_time - now;
        }
    }

    // 托管的直接走超时处理逻辑
    if (action_seat->IsHost())
    {
        timeout = 0;
    }

    if (timeout <= 0)
    {
        timeout = 1;
    }

    table_->StopTimer(action_timer_);
    action_timer_ = table_->StartTimer(timeout * 100, std::bind(&ActionComponent::ActionTimeout, this));
}

pb::ActionBrief ActionComponent::GetActionBrief()
{
    pb::ActionBrief brief;
    brief.set_seatid(-1);
    brief.set_action_start_time(start_time_);

    auto action_seat = GetActionSeat();
    if (action_seat != nullptr)
    {
        brief.set_seatid(action_seat->GetSeatID());
    }

    return brief;
}

void ActionComponent::ActionTimeout()
{
    table_->VSystemAction();
}

void ActionComponent::HandleHostSeatEvent(SeatIterator::SeatPtr seat)
{
    auto action_seat = GetActionSeat();
    if (action_seat == nullptr || action_seat != seat)
    {
        return;
    }

    // 开始托管直接走超时处理
    ActionTimeout();
}

int32_t ActionComponent::GetActionSeatID()
{
    auto action_seat = GetActionSeat();
    if (action_seat == nullptr)
    {
        return -1;
    }

    return action_seat->GetSeatID();
}
