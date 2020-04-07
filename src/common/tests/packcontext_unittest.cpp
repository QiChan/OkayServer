#include <common/pack.h>
#include <gtest/gtest.h>

TEST(PackContext, ResetContext)
{
    PackContext ctx;
    uint64_t uid = 12;
    uint16_t  harborid = 0xffff;
    uint64_t tick = 14567890;

    ctx.ResetContext(uid, harborid, tick);

    uint64_t version = 0;
    version = static_cast<uint64_t>(harborid & 0xffff) << 48;
    version |= static_cast<uint64_t>((ctx.receive_time_ / 1000000) & 0xffffffffffff);

    ASSERT_EQ(ctx.process_uid_, uid);
    ASSERT_EQ(ctx.receive_agent_tick_, tick);
    ASSERT_EQ(ctx.version_, version);
    ASSERT_EQ(ctx.harborid(), harborid);
}

TEST(PackContext, Parse_And_Serialize)
{
    PackContext ctx;
    uint64_t uid = 14;
    uint8_t  harborid = 0;
    uint64_t tick = 214567890;

    ctx.ResetContext(uid, harborid, tick);

    char* data = static_cast<char*>(malloc(sizeof(PackContext)));
    ctx.SerializeToStream(data, sizeof(PackContext));

    PackContext verify_ctx;
    verify_ctx.ParseFromStream(data);

    ASSERT_EQ(ctx.version_, verify_ctx.version_);
    ASSERT_EQ(ctx.receive_time_, verify_ctx.receive_time_);
    ASSERT_EQ(ctx.receive_agent_tick_, verify_ctx.receive_agent_tick_);
    ASSERT_EQ(ctx.process_uid_, verify_ctx.process_uid_);
    ASSERT_EQ(harborid, verify_ctx.harborid());
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
