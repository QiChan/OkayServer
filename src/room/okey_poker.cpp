#include "okey_poker.h"
#include "../tools/real_rand.h"
#include <set>


std::vector<PokerCard>  OkeyPoker::cards_ = {
    101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,      // 黑
    201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,      // 红
    301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313,      // 梅
    401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413,      // 方
    101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
    201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
    301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313,
    401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413,
    OKEY_CARD,  
    OKEY_CARD
};

uint32_t GetPokerCardPoint(PokerCard card)
{
    return card % 100;
}

uint32_t GetPokerCardColor(PokerCard card)
{
    return card / 100;
}

PokerCard GetPokerCard(uint32_t color, uint32_t point)
{
    return color * 100 + point;
}

bool IsSameColor(PokerCard lhs, PokerCard rhs)
{
    return GetPokerCardColor(lhs) == GetPokerCardColor(rhs);
}

bool IsSamePoint(PokerCard lhs, PokerCard rhs)
{
    return GetPokerCardPoint(lhs) == GetPokerCardPoint(rhs);
}

bool IsOkeyCard(PokerCard card)
{
    return card == OKEY_CARD;
}

OkeyPoker::OkeyPoker()
    : Poker(OkeyPoker::cards_),
      indicator_(POKER_CARD_NULL),
      wild_card_(POKER_CARD_NULL),
      okey_card_(POKER_CARD_NULL)
{
}

OkeyPoker::~OkeyPoker()
{
}

void OkeyPoker::VChooseIndicator()
{
    uint32_t color = RealRand::GetInstance()->Rand(1, 4);
    uint32_t point = RealRand::GetInstance()->Rand(1, 13);
    indicator_ = GetPokerCard(color, point);

    if (point == 13)
    {
        point = 1;
    }
    else
    {
        point += 1;
    }
    wild_card_ = GetPokerCard(color, point);
    okey_card_ = wild_card_;
}

bool OkeyPoker::VIsTripsOrQuads(const std::vector<PokerCard>& cards) const
{
    // 花色不一样, 点数一样
    if (cards.size() != 4 && cards.size() != 3)
    {
        return false;
    }

    std::array<uint32_t, 4>   colors = {0};
    uint32_t points = 0;
    for (PokerCard card : cards)
    {
        // 万能牌
        if (IsWildCard(card))
        {
            continue;
        }

        // okey牌
        if (IsOkeyCard(card))
        {
            card = okey_card_;
        }

        uint32_t color = GetPokerCardColor(card) - 1;
        if (colors[color] != 0)
        {
            return false;
        }
        colors[color] = 1;

        uint32_t p = GetPokerCardPoint(card);
        if (points != 0 && p != points)
        {
            return false;
        }
        points = p;
    }

    return true;
}

bool OkeyPoker::VIsFlushPairs(const std::vector<PokerCard>& cards) const
{
    // 同色同点数
    if (cards.size() != 2)
    {
        return false;
    }

    // TODO: 如果两张都是万能牌怎么处理
    PokerCard l_card = cards[0];
    if (IsWildCard(l_card))
    {
        return true;
    }

    if (IsOkeyCard(l_card))
    {
        l_card = okey_card_;
    }

    PokerCard r_card = cards[1];
    if (IsWildCard(r_card))
    {
        return true;
    }

    if (IsOkeyCard(r_card))
    {
        r_card = okey_card_;
    }

    uint32_t l_color = GetPokerCardColor(l_card);
    uint32_t l_point = GetPokerCardPoint(l_card);
    uint32_t r_color = GetPokerCardColor(r_card);
    uint32_t r_point = GetPokerCardPoint(r_card);

    return l_color == r_color && l_point == r_point;
}

bool OkeyPoker::VIsFlushStraight(const std::vector<PokerCard>& cards) const
{
    if (cards.size() < 3 || cards.size() > 13)
    {
        return false;
    }

    std::multiset<PokerCard>  pre_cards;
    uint32_t wildcard_num = 0;

    // 预处理，把OkeyCard直接替换掉，然后把万能牌数量筛选出来，并且从小到大排序好
    for (auto card : cards)
    {
        if (IsWildCard(card))
        {
            wildcard_num++;
            continue;
        }

        if (IsOkeyCard(card))
        {
            card = okey_card_;
        }
        pre_cards.insert(card);
    }

    // 1当作1的情况
    if (IsFlushStraight(pre_cards, wildcard_num))
    {
        return true;
    }

    // 1当作14处理
    auto it = pre_cards.begin();
    auto card = *it;
    uint32_t card_color = GetPokerCardColor(card);
    uint32_t card_point = GetPokerCardPoint(card);
    if (card_point != 1)
    {
        return false;
    }

    pre_cards.erase(it);
    card = GetPokerCard(card_color, 14);
    pre_cards.insert(card);

    return IsFlushStraight(pre_cards, wildcard_num);
}

bool OkeyPoker::IsFlushStraight(const std::multiset<PokerCard>& cards, uint32_t wildcard_num) const
{
    uint32_t verify_color = 0;
    uint32_t verify_point = 0;

    for (auto card : cards)
    {
        uint32_t color = GetPokerCardColor(card);
        if (verify_color != 0 && color != verify_color)
        {
            return false;
        }
        verify_color = color;

        uint32_t point = GetPokerCardPoint(card);
        if (verify_point == 0 || point == verify_point)
        {
            verify_point = point + 1;
            continue;
        }

        if (point < verify_point)
        {
            return false;
        }
        else
        {
            // 使用癞子
            uint32_t interval = point - verify_point;
            if (wildcard_num < interval)
            {
                return false;
            }
            wildcard_num -= interval;
            verify_point = point + 1;
        }
    }

    return true;
}

OkeyPoker101::OkeyPoker101()
{
}

OkeyPoker101::~OkeyPoker101()
{
}

bool OkeyPoker101::VIsFlushStraight(const std::vector<PokerCard>& cards) const
{
    if (cards.size() < 3 || cards.size() > 13)
    {
        return false;
    }

    std::multiset<PokerCard>  pre_cards;
    uint32_t wildcard_num = 0;

    // 预处理，把OkeyCard直接替换掉，然后把万能牌数量筛选出来，并且从小到大排序好
    for (auto card : cards)
    {
        if (IsWildCard(card))
        {
            wildcard_num++;
            continue;
        }

        if (IsOkeyCard(card))
        {
            card = okey_card_;
        }
        pre_cards.insert(card);
    }

    return IsFlushStraight(pre_cards, wildcard_num);
}
