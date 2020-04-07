#include "poker.h"
#include <tools/real_rand.h>
#include <iostream>

Poker::Poker(const std::vector<PokerCard>& cards)
    : cards_(cards),
      iter_(0)
{
}

void Poker::Shuffle()
{
    // FisherYates洗牌算法
    size_t i = cards_.size();
    while (--i)
    {
        uint32_t index = RealRand::GetInstance()->Rand(0, i);
        PokerCard temp = cards_[i];

        cards_[i] = cards_[index];
        cards_[index] = temp;
    }

    iter_ = 0;
}

PokerCard Poker::Pop()
{
    if (iter_ >= cards_.size())
    {
        return POKER_CARD_NULL;
    }

    PokerCard card = cards_[iter_];
    ++iter_;

    return card;
}
