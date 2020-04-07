/*
 * TODO: 
 * 1. sahte_card自己本身可以组成对子吗
 * 2. 123456789ABCD1是合法的吗
 * 3. 123456789ABCD0是合法的吗
 */

#ifndef __ROOM_POKER_H__
#define __ROOM_POKER_H__

#include <vector>
#include <cstdint>
#include <cstddef>

#define POKER_CARD_NULL     0

using PokerCard = uint16_t;

class Poker
{
public:
    Poker (const std::vector<PokerCard>& cards);
    virtual ~Poker () = default;
    Poker (Poker const&) = delete;
    Poker& operator= (Poker const&) = delete;

public:
    void        Shuffle();
    PokerCard   Pop();
    uint32_t    GetLastCardNum() const { return cards_.size() - iter_; }

public:
    // 判断是否三条或四条
    virtual bool    VIsTripsOrQuads(const std::vector<PokerCard>&) const = 0;
    // 判断是否同色对子
    virtual bool    VIsFlushPairs(const std::vector<PokerCard>&) const = 0;
    // 判断是否同色顺
    virtual bool    VIsFlushStraight(const std::vector<PokerCard>&) const = 0;

    virtual void    VChooseIndicator() = 0;
    virtual PokerCard    VGetIndicatorCard() const = 0;

private:
    std::vector<PokerCard>      cards_;
    size_t                      iter_;
};

#endif 
