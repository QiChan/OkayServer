#include <tools/string_util.h>
#include <gtest/gtest.h>

TEST(StringUtil, DivideString)
{
    std::string origin_str = "hello world, abc def";
    std::string str = origin_str;
    std::string first_str;
    std::string second_str;

    std::tie(first_str, second_str) = StringUtil::DivideString(str, ' ');
    ASSERT_EQ(str, origin_str);
    ASSERT_EQ(first_str, "hello");
    ASSERT_EQ(second_str, "world, abc def");

    std::tie(first_str, second_str) = StringUtil::DivideString(str, ',');
    ASSERT_EQ(str, origin_str);
    ASSERT_EQ(first_str, "hello world");
    ASSERT_EQ(second_str, " abc def");

    std::tie(first_str, second_str) = StringUtil::DivideString(str, 'h');
    ASSERT_EQ(str, origin_str);
    ASSERT_TRUE(first_str.empty());
    ASSERT_EQ(second_str, "ello world, abc def");

    std::tie(first_str, second_str) = StringUtil::DivideString(str, 'z');
    ASSERT_EQ(str, origin_str);
    ASSERT_EQ(first_str, origin_str);
    ASSERT_TRUE(second_str.empty());

    std::tie(first_str, second_str) = StringUtil::DivideString(str, '\n');
    ASSERT_EQ(str, origin_str);
    ASSERT_EQ(first_str, origin_str);
    ASSERT_TRUE(second_str.empty());
}

TEST(StringUtil, ParseClientVersion)
{
    {
        std::string verify_head = "3";
        std::string verify_mid = "01";
        std::string verify_tail = "02";
        std::string version = verify_head + "." + verify_mid + "." + verify_tail;
        std::string head, mid, tail;
        std::tie(head, mid) = StringUtil::DivideString(version, '.');
        std::tie(mid, tail) = StringUtil::DivideString(mid, '.');
        ASSERT_EQ(head, verify_head);
        ASSERT_EQ(mid, verify_mid);
        ASSERT_EQ(tail, verify_tail);
    }
    {
        std::string verify_head = "3";
        std::string verify_mid = "";
        std::string verify_tail = "02";
        std::string version = verify_head + "." + verify_mid + "." + verify_tail;
        std::string head, mid, tail;
        std::tie(head, mid) = StringUtil::DivideString(version, '.');
        std::tie(mid, tail) = StringUtil::DivideString(mid, '.');
        ASSERT_EQ(head, verify_head);
        ASSERT_EQ(mid, verify_mid);
        ASSERT_EQ(tail, verify_tail);
    }
    {
        std::string verify_head = "";
        std::string verify_mid = "";
        std::string verify_tail = "";
        std::string version = verify_head + "." + verify_mid + "." + verify_tail;
        std::string head, mid, tail;
        std::tie(head, mid) = StringUtil::DivideString(version, '.');
        std::tie(mid, tail) = StringUtil::DivideString(mid, '.');
        ASSERT_EQ(head, verify_head);
        ASSERT_EQ(mid, verify_mid);
        ASSERT_EQ(tail, verify_tail);
    }
    {
        std::string verify_head = "";
        std::string verify_mid = "01";
        std::string verify_tail = "02";
        std::string version = verify_head + "." + verify_mid + "." + verify_tail;
        std::string head, mid, tail;
        std::tie(head, mid) = StringUtil::DivideString(version, '.');
        std::tie(mid, tail) = StringUtil::DivideString(mid, '.');
        ASSERT_EQ(head, verify_head);
        ASSERT_EQ(mid, verify_mid);
        ASSERT_EQ(tail, verify_tail);
    }
    {
        std::string verify_head = "abcdef";
        std::string verify_mid;
        std::string verify_tail;
        std::string version = verify_head;
        std::string head, mid, tail;
        std::tie(head, mid) = StringUtil::DivideString(version, '.');
        std::tie(mid, tail) = StringUtil::DivideString(mid, '.');
        ASSERT_EQ(head, verify_head);
        ASSERT_EQ(mid, verify_mid);
        ASSERT_EQ(tail, verify_tail);
    }
}

TEST(StringUtil, StringLen)
{
    size_t sz = -1;
    sz = StringUtil::StringLen(nullptr);
    ASSERT_EQ(sz, 0);

    sz = StringUtil::StringLen("hello world");
    ASSERT_EQ(sz, 11);
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
