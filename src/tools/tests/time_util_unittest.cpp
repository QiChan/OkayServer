#include <tools/time_util.h>
#include <gtest/gtest.h>

TEST(TimeUtil, date_to_timestamp)
{
    uint32_t ts_8 = TimeUtil::date_to_timestamp(20200212, 8);
    ASSERT_EQ(ts_8, 1581436800);
    uint32_t ts_0 = TimeUtil::date_to_timestamp(20200211, 0);
    ASSERT_EQ(ts_0, 1581379200);
    uint32_t ts_minus12 = TimeUtil::date_to_timestamp(20200210, -12);
    ASSERT_EQ(ts_minus12, 1581336000);
}

TEST(TimeUtil, timestamp_to_date)
{
    int32_t date_8_1 = TimeUtil::timestamp_to_date(1581456017, 8);    
    ASSERT_EQ(date_8_1, 20200212);
    int32_t date_8_2 = TimeUtil::timestamp_to_date(1581436800, 8); 
    ASSERT_EQ(date_8_2, 20200212);

    int32_t date_0_1 = TimeUtil::timestamp_to_date(1581379200, 0);
    ASSERT_EQ(date_0_1, 20200211);
    int32_t date_0_2 = TimeUtil::timestamp_to_date(1581484817, 0);
    ASSERT_EQ(date_0_2, 20200212);

    int32_t date_minus12_1 = TimeUtil::timestamp_to_date(1581336000, -12);
    ASSERT_EQ(date_minus12_1, 20200210);
    int32_t date_minus12_2 = TimeUtil::timestamp_to_date(1582132817, -12);
    ASSERT_EQ(date_minus12_2, 20200219);
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
