#include "../poker.h"
#include <gtest/gtest.h>
#include <unordered_map>
#include <tools/real_rand.h>

class TestPoker : public Poker 
{
public:
    TestPoker(const std::vector<PokerCard>& cards)
        : Poker(cards)
    {
    }

public:
    // 判断是否四条
    virtual bool    VIsTripsOrQuads(const std::vector<PokerCard>&) const override { return true; }
    // 判断是否同色对子
    virtual bool    VIsFlushPairs(const std::vector<PokerCard>&) const override { return true; }
    // 判断是否同色顺
    virtual bool    VIsFlushStraight(const std::vector<PokerCard>&) const override { return true; }

    virtual void VChooseIndicator() override {}
    virtual PokerCard VGetIndicatorCard() const override { return POKER_CARD_NULL; }
};

TEST(Poker, Shuffle)
{
    ASSERT_TRUE(RealRand::GetInstance()->Init());
    std::vector<PokerCard>  cards = {1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013};
    // 表示牌cards[i]在第j个位置出现的次数
    std::vector<std::vector<size_t>>    test_result;
    for (size_t i = 0; i < cards.size(); i++)
    {
        std::vector<size_t>     vec;
        for (size_t j = 0; j < cards.size(); j++)
        {
            vec.push_back(0);
        }
        test_result.push_back(vec);
    }

    std::unordered_map<PokerCard, uint32_t>     arr_map;    // 用于查找手牌在原来数组中的位置
    for (size_t i = 0; i < cards.size(); i++)
    {
        arr_map[cards[i]] = i;
    }

    TestPoker   poker(cards);

    uint32_t    counts = 500000;
    uint32_t    total_counts = counts * cards.size();
    for (size_t i = 0; i < counts; i++)
    {
        poker.Shuffle();

        std::unordered_map<PokerCard, uint32_t>     verify_cards;
        size_t j = 0;
        PokerCard card;
        while ((card = poker.Pop()) != POKER_CARD_NULL)
        {
            test_result[arr_map[card]][j] += 1;
            j++;
            verify_cards[card] += 1;
        }

        for (auto& elem : verify_cards)
        {
            if (verify_cards.size() != cards.size())
            {
                std::cout << "elem: " << elem.first << " ----- " << elem.second << std::endl; 
            }
            ASSERT_EQ(elem.second, 1);
        }
        ASSERT_EQ(cards.size(), verify_cards.size());
    }

    uint32_t verify_counts = 0;
    for (size_t i = 0; i < cards.size(); i++)
    {
        std::cout << cards[i] << ": ";
        for (size_t j = 0; j < cards.size(); j++)
        {
            verify_counts += test_result[i][j];
            std::cout << test_result[i][j] << " ";
        }
        std::cout << std::endl;
    }

    ASSERT_EQ(total_counts, verify_counts);
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
