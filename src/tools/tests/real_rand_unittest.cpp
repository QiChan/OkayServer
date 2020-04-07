#include <tools/real_rand.h>
#include <gtest/gtest.h>
#include <unordered_set>
#include <iostream>

TEST(RealRand, Rand)
{
    ASSERT_TRUE(RealRand::GetInstance()->Init());

    std::unordered_set<uint32_t>    data_sets; 
    size_t count = 0;

    for (size_t i = 0; i < 100; i++)
    {
        uint32_t r = RealRand::GetInstance()->Rand(0, UINT32_MAX - 1);
        auto res = data_sets.insert(r);
        if (!res.second)
        {
            ++count;
        }
    }
    ASSERT_LT(count, 3);

    size_t min = 1002;
    size_t max = 1012;
    for (size_t i = 0; i < 1000000; i++)
    {
        uint32_t r = RealRand::GetInstance()->Rand(min, max);
        ASSERT_LE(r, max);
        ASSERT_GE(r, min);
    }
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
