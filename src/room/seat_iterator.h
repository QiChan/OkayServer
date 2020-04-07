#ifndef __ROOM_SEAT_ITERATOR_H__
#define __ROOM_SEAT_ITERATOR_H__

#include <cstdint>
#include <memory>

class Seat;
class Table;
class SeatIterator final
{
public:
    using SeatPtr = std::shared_ptr<Seat>;

public:
    SeatIterator (Table*, uint32_t begin, bool infinite = false);
    ~SeatIterator () = default;
    SeatIterator (SeatIterator const&) = delete;
    SeatIterator& operator= (SeatIterator const&) = delete;

public:
    SeatPtr         operator*();
    SeatIterator&   operator++();
    Table*          GetTable() { return table_; }

private:
    Table*      table_;
    uint32_t    curr_index_;
    uint32_t    begin_;
    bool        is_reset_;              // 是否重置过
    bool        is_infinite_;           // 是否是无限循环
};


class SitedSeatIterator final
{
public:
    SitedSeatIterator (Table* table, uint32_t begin = 0);
    ~SitedSeatIterator () = default;
    SitedSeatIterator (SitedSeatIterator const&) = delete;
    SitedSeatIterator& operator= (SitedSeatIterator const&) = delete;

public:
    SeatIterator::SeatPtr   operator*();
    SitedSeatIterator&      operator++();

private:
    SeatIterator    iter_;
};

// 准备好玩牌的座位迭代器
class ReadySeatIterator final
{
public:
    ReadySeatIterator(Table* table, uint32_t begin = 0);
    ~ReadySeatIterator() = default;
    ReadySeatIterator(ReadySeatIterator const&) = delete;
    ReadySeatIterator& operator= (ReadySeatIterator const&) = delete;

public:
    SeatIterator::SeatPtr   operator*();
    ReadySeatIterator&      operator++();

private:
    SeatIterator    iter_;
};

// 正在玩牌的座位迭代器
class PlayingSeatIterator final
{
public:
    PlayingSeatIterator (Table* table, uint32_t begin = 0);
    ~PlayingSeatIterator () = default;
    PlayingSeatIterator (PlayingSeatIterator const&) = delete;
    PlayingSeatIterator& operator= (PlayingSeatIterator const&) = delete; 

public:
    SeatIterator::SeatPtr   operator*();
    PlayingSeatIterator&    operator++();

private:
    SeatIterator    iter_;
};

// 行动的座位迭代器
class ActionSeatIterator final
{
public:
    ActionSeatIterator (Table* table, uint32_t begin);
    ~ActionSeatIterator () = default;
    ActionSeatIterator (ActionSeatIterator const&) = delete;
    ActionSeatIterator& operator= (ActionSeatIterator const&) = delete; 

public:
    SeatIterator::SeatPtr   operator*();
    ActionSeatIterator&    operator++();
    SeatIterator::SeatPtr   prev_seat();    // 前一个行动的玩家

private:
    SeatIterator    iter_;
};


#endif
