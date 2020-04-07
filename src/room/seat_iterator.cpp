#include "seat_iterator.h"
#include "table.h"

SeatIterator::SeatIterator(Table* table, uint32_t begin, bool infinite)
    : table_(table),
      curr_index_(begin),
      begin_(begin),
      is_reset_(false),
      is_infinite_(infinite)
{
    // 处理begin超过范围的情况
    if (*(*this) == nullptr)
    {
        ++(*this);
    }
}

SeatIterator::SeatPtr SeatIterator::operator*() 
{
    if (table_ == nullptr)
    {
        return nullptr;
    }

    if (!is_infinite_ && is_reset_ && (curr_index_ == begin_))
    {
        return nullptr;
    }
    return table_->GetSeat(curr_index_);
}

SeatIterator& SeatIterator::operator++()
{
    if (table_ == nullptr)
    {
        return *this;
    }

    if (is_reset_ && (curr_index_ == begin_) && !is_infinite_)
    {
        return *this;
    }

    curr_index_++;

    auto sp_seat = table_->GetSeat(curr_index_);
    if (sp_seat != nullptr)
    {
        return *this;
    }

    if (!is_reset_ || is_infinite_)
    {
        curr_index_ = 0;
    }

    is_reset_ = true;
    return *this;
}

SitedSeatIterator::SitedSeatIterator(Table* table, uint32_t begin)
    : iter_(table, begin)
{
}

SeatIterator::SeatPtr   SitedSeatIterator::operator*()
{
    auto seat = *iter_;
    while (seat != nullptr)
    {
        if (seat->IsSited())
        {
            break;
        }
        ++iter_;
        seat = *iter_;
    }

    return seat;
}

SitedSeatIterator& SitedSeatIterator::operator++()
{
    ++iter_;
    return *this;
}

ReadySeatIterator::ReadySeatIterator(Table* table, uint32_t begin)
    : iter_(table, begin)
{
    if (*iter_ == nullptr)
    {
        ++iter_;
    }
}

SeatIterator::SeatPtr ReadySeatIterator::operator*()
{
    auto seat = *iter_;

    while (seat != nullptr)
    {
        if (seat->IsReady())
        {
            break;
        }
        ++iter_;
        seat = *iter_;
    }

    return seat;
}

ReadySeatIterator& ReadySeatIterator::operator++()
{
    ++iter_;
    return *this;
}

PlayingSeatIterator::PlayingSeatIterator(Table* table, uint32_t begin)
    : iter_(table, begin)
{
}

SeatIterator::SeatPtr PlayingSeatIterator::operator*()
{
    auto seat = *iter_;
    while (seat != nullptr)
    {
        if (seat->IsPlaying())
        {
            break;
        }
        ++iter_;
        seat = *iter_;
    }

    return seat;
}

PlayingSeatIterator& PlayingSeatIterator::operator++()
{
    ++iter_;
    return *this;
}

ActionSeatIterator::ActionSeatIterator(Table* table, uint32_t begin)
    : iter_(table, begin, true)
{
}

SeatIterator::SeatPtr ActionSeatIterator::operator*()
{
    auto seat = *iter_;
    // 防止死循环
    auto begin = seat;
    while (seat != nullptr)
    {
        if (seat->IsPlaying())
        {
            break;
        }
        ++iter_;
        seat = *iter_;

        // 防止死循环
        if (seat == begin)
        {
            seat = nullptr;
            break;
        }
    }

    return seat;
}

ActionSeatIterator& ActionSeatIterator::operator++()
{
    auto seat = *iter_;
    if (seat != nullptr)
    {
        // idle状态的下一回合还是该位置
        if (seat->IsTakeState())
        {
            return *this;
        }
    }
    ++iter_;
    return *this;
}

SeatIterator::SeatPtr ActionSeatIterator::prev_seat()
{
    auto curr_seat = *(*this);

    PlayingSeatIterator iter(iter_.GetTable());
    auto seat = *iter;
    while (seat != nullptr)
    {
        ++iter;
        auto next_seat = *iter;
        if (next_seat == curr_seat || next_seat == nullptr)
        {
            break;
        }

        seat = next_seat;
    }

    return seat;
}
