#include <common/pack.h>
#include <gtest/gtest.h>
#include "test_pack.pb.h"

class PackTest : public testing::Test
{
    public:
        PackTest() = default;
        virtual ~PackTest() = default;
        PackTest(const PackTest&) = delete;
        PackTest& operator= (const PackTest&) = delete;

    public:
        virtual void SetUp() override
        {
            pack_data_.set_uid(123);
            pack_data_.set_name("hello");
            pack_data_.add_items(5);
            pack_data_.add_items(6);
            pack_data_.add_items(-4);
            opack_.reset(pack_data_);
            ASSERT_EQ(opack_.type_name(), pack_data_.GetTypeName());
        }

    protected:
        pb::TestPack    pack_data_;
        OutPack         opack_;
};

TEST_F(PackTest, CreateMessage)
{
    InPack ipack;
    auto msg = ipack.CreateMessage();
    ASSERT_EQ(msg, nullptr);
}

TEST_F(PackTest, ParseFromNetData)
{
    char* data = nullptr;
    uint32_t size = 0;
    opack_.SerializeToNetData(data, size);

    InPack  ipack;
    PackContext verify_ctx;
    verify_ctx.ResetContext(15, 8, 18993456);
    ipack.ParseFromNetData(verify_ctx, data + PACK_HEAD, size - PACK_HEAD);

    ASSERT_EQ(ipack.pack_context(), verify_ctx);

    std::shared_ptr<pb::TestPack> verify_pack_data = std::dynamic_pointer_cast<pb::TestPack>(ipack.CreateMessage());
    std::string str_pb1;
    std::string str_pb2;
    pack_data_.SerializeToString(&str_pb1);
    verify_pack_data->SerializeToString(&str_pb2);
    ASSERT_EQ(str_pb1, str_pb2);
}

TEST_F(PackTest, ParseFromInnerData)
{
    char* data = nullptr;
    uint32_t size = 0;

    PackContext ctx;
    ctx.ResetContext(12, 3, 11567803);
    opack_.SerializeToInnerData(data, size, ctx);

    InPack ipack;
    ipack.ParseFromInnerData(data, size);
    std::shared_ptr<pb::TestPack> verify_pack_data = std::static_pointer_cast<pb::TestPack>(ipack.CreateMessage());

    std::string str_pb1;
    std::string str_pb2;
    pack_data_.SerializeToString(&str_pb1);
    verify_pack_data->SerializeToString(&str_pb2);
    ASSERT_EQ(str_pb1, str_pb2);

    PackContext verify_ctx = ipack.pack_context();
    ASSERT_EQ(ctx, verify_ctx);
}

TEST_F(PackTest, InnerToNet)
{
    char* data = nullptr;
    uint32_t size = 0;
    PackContext ctx;
    ctx.ResetContext(12, 3, 11567803);
    opack_.SerializeToInnerData(data, size, ctx);

    char* verify_data = nullptr;
    uint32_t verify_size = 0;
    opack_.SerializeToNetData(verify_data, verify_size);

    char* net_data = nullptr;
    uint32_t net_size = 0;
    InnerDataToNetData(data, size, net_data, net_size);

    ASSERT_EQ(net_size, verify_size);
    ASSERT_STREQ(net_data, verify_data);
}

TEST_F(PackTest, NetToInner)
{
    char* data = nullptr;
    uint32_t size = 0;
    opack_.SerializeToNetData(data, size);

    char* verify_data = nullptr;
    uint32_t verify_size = 0;
    PackContext ctx;
    ctx.ResetContext(12, 3, 15556009);
    opack_.SerializeToInnerData(verify_data, verify_size, ctx);

    char* inner_data = nullptr;
    uint32_t inner_size = 0;
    NetDataToInnerData(data + PACK_HEAD, size - PACK_HEAD, ctx, inner_data, inner_size);

    ASSERT_STREQ(inner_data, verify_data);
    ASSERT_EQ(inner_size, verify_size);
}

TEST_F(PackTest, SerializeToInnerData)
{
    char* data = nullptr;
    uint32_t size = 0;
    PackContext ctx;
    ctx.ResetContext(12, 3, 11567803);
    SerializeToInnerData(pack_data_, data, size, ctx);

    char* verify_data = nullptr;
    uint32_t verify_size = 0;
    opack_.SerializeToInnerData(verify_data, verify_size,ctx);

    ASSERT_STREQ(data, verify_data);
    ASSERT_EQ(size, verify_size);
}

TEST_F(PackTest, SerializeToNetData)
{
    char* data = nullptr;
    uint32_t size = 0;
    SerializeToNetData(pack_data_, data, size);

    char* verify_data = nullptr;
    uint32_t verify_size = 0;
    opack_.SerializeToNetData(verify_data, verify_size);

    ASSERT_STREQ(data, verify_data);
    ASSERT_EQ(size, verify_size);
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
