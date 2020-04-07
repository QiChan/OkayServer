#include "../okey_poker.h"
#include <gtest/gtest.h>
#include <tools/real_rand.h>

class OkeyPokerTest : public testing::Test
{
public:
    OkeyPokerTest() = default;
    virtual ~OkeyPokerTest() = default;
    OkeyPokerTest(const OkeyPokerTest&) = delete;
    OkeyPokerTest& operator= (const OkeyPokerTest&) = delete;

public:
    virtual void SetUp() override
    {
        ASSERT_TRUE(RealRand::GetInstance()->Init());
        poker_.Shuffle();
        poker_.VChooseIndicator();
    }

protected:
    OkeyPoker   poker_;
};

TEST_F(OkeyPokerTest, VChooseIndicator)
{
    for (int i = 0; i < 100; i++)
    {
        poker_.VChooseIndicator();
        PokerCard indicator = poker_.VGetIndicatorCard();
        ASSERT_NE(indicator, POKER_CARD_NULL);
        PokerCard wildcard = poker_.GetWildCard();
        ASSERT_NE(wildcard, POKER_CARD_NULL);

        uint32_t i_color = GetPokerCardColor(indicator);
        uint32_t i_point = GetPokerCardPoint(indicator);
        uint32_t w_color = GetPokerCardColor(wildcard);
        uint32_t w_point = GetPokerCardPoint(wildcard);

        ASSERT_LE(i_point, 13);
        ASSERT_GE(i_point, 1);
        ASSERT_EQ(i_color, w_color);

        if (i_point == 13)
        {
            i_point = 1;
        }
        else
        {
            i_point += 1;
        }

        ASSERT_EQ(i_point, w_point);
    }
}

TEST_F(OkeyPokerTest, VIsFlushStraight)
{
    for (int i = 0; i < 200; i++)
    {
        poker_.VChooseIndicator();
        PokerCard indicator = poker_.VGetIndicatorCard();
        PokerCard wildcard = poker_.GetWildCard();
        uint32_t wild_color = GetPokerCardColor(wildcard);

        {
            if (wild_color == 3 || wild_color == 4)
            {
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{101, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{104, 105, 106, 107, 108, 109, 110, 111, 112, 113}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{104, 105, 106}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{113, 112, 101}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{101, 102, 113}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{101, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{101, 102, 103, 101}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{101, 102, 103, 105}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{104, 205, 106}));
            }
            else
            {
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{304, 305, 306, 307, 308, 309, 310, 311, 312, 313}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{304, 305, 306}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{313, 312, 301}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 302, 313}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 302, 303, 301}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 302, 303, 305}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{304, 405, 306}));
            }
        }

        {
            // okey_card
            uint32_t color = GetPokerCardColor(indicator);
            uint32_t point = GetPokerCardPoint(indicator);
            if (point == 1)
            {
                point = 3;
            }
            else
            {
                point = point - 1;
            }

            PokerCard card = GetPokerCard(color, point);
            ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{indicator, card, OKEY_CARD}));
            ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{indicator, card, OKEY_CARD, wildcard}));
            ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{indicator, card, OKEY_CARD, OKEY_CARD, wildcard}));
        }

        {
            if (wild_color != 2)
            {
                // wild_card
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 203, 204, 205, 206, 207, 208, 209, 210, 211, wildcard, 212}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 203, 204, 205, 206, 207, 208, 209, 210, 211, wildcard, 213}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 204, 205, 206, 207, 208, 209, 210, 211, wildcard, 212}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 204, 205, 206, 207, 208, 209, 210, 211, wildcard, 213}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 203, 204, 205, 206, 207, 208, 209, 210, 211, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 204, 205, 206, 207, 208, 209, 210, 211, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 204, 205, 206, 207, 208, 209, 210, 213, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 204, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 202, 213, wildcard}));

                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 205, 206, 207, 208, 209, wildcard, 210, 211, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 205, 206, wildcard, 207, 208, 209, 210, 213, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 201, 204, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{201, 205, 202, 207, 204, 208, 209, wildcard, 210, 211, 212, 213, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 201, 203, 205, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 205, 206, 207, 208, 209, 210, 211, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 205, 206, 207, 208, 209, 210, 211, 212, 213, 204, 203, wildcard}));

                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 201, 202, 213, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 201, 205, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 201, 201, 205, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 201, 203, 208, 205, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 202, 201, 206, 207, 208, 209, 210, 211, 212, 213, 204, 203, wildcard}));
            }
            else
            {
                // wild_card
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 303, 304, 305, 306, 307, 308, 309, 310, 311, wildcard, 312}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 303, 304, 305, 306, 307, 308, 309, 310, 311, wildcard, 313}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 304, 305, 306, 307, 308, 309, 310, 311, wildcard, 312}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 304, 305, 306, 307, 308, 309, 310, 311, wildcard, 313}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 303, 304, 305, 306, 307, 308, 309, 310, 311, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 304, 305, 306, 307, 308, 309, 310, 311, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 304, 305, 306, 307, 308, 309, 310, 313, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 304, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 302, 313, wildcard}));

                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 305, 306, 307, 308, 309, wildcard, 310, 311, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 305, 306, wildcard, 307, 308, 309, 310, 313, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 301, 304, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{301, 305, 302, 307, 304, 308, 309, wildcard, 310, 311, 312, 313, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 301, 303, 305, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 305, 306, 307, 308, 309, 310, 311, wildcard}));
                ASSERT_TRUE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 305, 306, 307, 308, 309, 310, 311, 312, 313, 304, 303, wildcard}));

                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 301, 302, 313, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 301, 305, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 301, 301, 305, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 301, 303, 308, 305, wildcard}));
                ASSERT_FALSE(poker_.VIsFlushStraight(std::initializer_list<PokerCard>{wildcard, 302, 301, 306, 307, 308, 309, 310, 311, 312, 313, 304, 303, wildcard}));
            }
        }
    }
}

TEST_F(OkeyPokerTest, VIsFlushPairs)
{
    for (int i = 0; i < 200; i++)
    {
        poker_.VChooseIndicator();
        PokerCard indicator = poker_.VGetIndicatorCard();
        PokerCard wildcard = poker_.GetWildCard();

        {
            ASSERT_TRUE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{101, 101}));
            ASSERT_FALSE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{101, 101, 101}));
            if (wildcard != 201 && wildcard != 101)
            {
                ASSERT_FALSE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{101, 201}));
            }
            else
            {
                ASSERT_TRUE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{101, 201}));
            }
            if (wildcard != 101 && wildcard != 102)
            {
                ASSERT_FALSE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{101, 102}));
            }
            else
            {
                ASSERT_TRUE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{101, 102}));
            }
        }

        {
            // wild card
            ASSERT_TRUE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{101, wildcard}));
            ASSERT_TRUE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{indicator, wildcard}));
            ASSERT_TRUE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{OKEY_CARD, wildcard}));
            ASSERT_TRUE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{OKEY_CARD, OKEY_CARD}));
            ASSERT_TRUE(poker_.VIsFlushPairs(std::initializer_list<PokerCard>{wildcard, wildcard}));
        }
    }
}

TEST_F(OkeyPokerTest, VIsTripsOrQuads)
{
    for (int i = 0; i < 200; i++)
    {
        poker_.VChooseIndicator();
        PokerCard   indicator = poker_.VGetIndicatorCard();
        PokerCard   wildcard = poker_.GetWildCard();
        ASSERT_NE(indicator, POKER_CARD_NULL);
        ASSERT_NE(wildcard, POKER_CARD_NULL);

        {
            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, 301}));
            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, 301, 401}));
            ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101}));
            ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, 301, 401, 501}));
            if (wildcard != 101)
            {
                ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, 101}));
            }
            else
            {
                ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, 101}));
            }
            if (wildcard != 201)
            {
                ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, 301, 201}));
            }
            else
            {
                ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, 301, 201}));
            }
        }

        {
            // okey card
            uint32_t color = GetPokerCardColor(wildcard);
            uint32_t point = GetPokerCardPoint(wildcard);

            color = color + 1;
            if (color > 4)
            {
                color = 1;
            }
            PokerCard card1 = GetPokerCard(color, point);
            color = color + 1;
            if (color > 4)
            {
                color = 1;
            }
            PokerCard card2 = GetPokerCard(color, point);
            color = color + 1;
            if (color > 4)
            {
                color = 1;
            }
            PokerCard card3 = GetPokerCard(color, point);


            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{card1, card2, OKEY_CARD}));
            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{card1, card2, OKEY_CARD, card3}));
            ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{card1, card2, OKEY_CARD, card2}));
            ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{card2, OKEY_CARD, card2}));
            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{OKEY_CARD, card1, card2}));
        }

        {
            // wild card
            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, wildcard}));
            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, wildcard, 301}));
            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, wildcard, wildcard}));
            ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, wildcard, wildcard}));
            if (wildcard != 101)
            {
                ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, wildcard, 101, 201}));
            }
            else
            {
                ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, wildcard, 101, 201}));
            }
            if (wildcard != 201)
            {
                ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, wildcard, 201}));
            }
            else
            {
                ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, 201, wildcard, 201}));
            }
            if (wildcard != 101)
            {
                ASSERT_FALSE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, wildcard, 101}));
            }
            else
            {
                ASSERT_TRUE(poker_.VIsTripsOrQuads(std::initializer_list<PokerCard>{101, wildcard, 101}));
            }
        }
    }
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
