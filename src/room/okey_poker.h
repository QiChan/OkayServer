#ifndef __ROOM_OKEY_POKER_H__
#define __ROOM_OKEY_POKER_H__

#include "poker.h"
#include <set>

#define OKEY_CARD 500

class OkeyPoker : public Poker
{
public:
    OkeyPoker();
    virtual ~OkeyPoker();
    OkeyPoker(OkeyPoker const&) = delete;
    OkeyPoker& operator= (OkeyPoker const&) = delete;

public:
    virtual bool    VIsTripsOrQuads(const std::vector<PokerCard>&) const override;
    virtual bool    VIsFlushPairs(const std::vector<PokerCard>&) const override;
    virtual bool    VIsFlushStraight(const std::vector<PokerCard>&) const override;
    virtual void    VChooseIndicator() override;
    virtual PokerCard   VGetIndicatorCard() const override { return indicator_; }

public:
    PokerCard       GetWildCard() const { return wild_card_; }

protected:
    bool            IsWildCard(PokerCard card) const { return card == wild_card_; }
    bool            IsFlushStraight(const std::multiset<PokerCard>& cards, uint32_t wildcard_num) const;

protected:
    PokerCard       indicator_;
    PokerCard       wild_card_;         // 万能牌
    PokerCard       okey_card_;         // okey牌表示的牌

protected:
    static std::vector<PokerCard>  cards_;
};

class OkeyPoker101 : public OkeyPoker
{
public:
    OkeyPoker101 ();
    virtual ~OkeyPoker101 ();
    OkeyPoker101 (OkeyPoker101 const&) = delete;;
    OkeyPoker101& operator= (OkeyPoker101 const&) = delete; 

public:
    virtual bool    VIsFlushStraight(const std::vector<PokerCard>&) const override;
};

uint32_t GetPokerCardColor(PokerCard);
uint32_t GetPokerCardPoint(PokerCard);
PokerCard GetPokerCard(uint32_t color, uint32_t point);

#endif
