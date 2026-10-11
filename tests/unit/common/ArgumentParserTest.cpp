#include "platform/common/ArgumentParser.hpp"

#include <gtest/gtest.h>
#include <string_view>

TEST(ArgumentParserTest, ReadsWhitespaceDelimitedTokens)
{
    platform::ArgumentParser parser("  flash\tread 0x1234  16  ");
    std::string_view token;

    ASSERT_TRUE(parser.next(token));
    EXPECT_EQ(token, "flash");
    ASSERT_TRUE(parser.next(token));
    EXPECT_EQ(token, "read");
    ASSERT_TRUE(parser.next(token));
    EXPECT_EQ(token, "0x1234");
    ASSERT_TRUE(parser.next(token));
    EXPECT_EQ(token, "16");
    EXPECT_FALSE(parser.next(token));
}

TEST(ArgumentParserTest, ReturnsRemainingArgumentsAfterNextToken)
{
    platform::ArgumentParser parser("flash   read 100 4");
    std::string_view token;

    ASSERT_TRUE(parser.next(token));
    EXPECT_EQ(token, "flash");
    EXPECT_EQ(parser.remaining(), "read 100 4");
}

TEST(ArgumentParserTest, ParsesDecimalAndHexUnsignedValues)
{
    std::uint64_t value = 0U;

    ASSERT_TRUE(platform::ArgumentParser::parseUnsigned("65535", 10U, value));
    EXPECT_EQ(value, 65535U);
    ASSERT_TRUE(platform::ArgumentParser::parseUnsigned("0x12aB", 16U, value));
    EXPECT_EQ(value, 0x12ABU);
    ASSERT_TRUE(platform::ArgumentParser::parseUnsigned("ff", 16U, value));
    EXPECT_EQ(value, 0xFFU);
}

TEST(ArgumentParserTest, RejectsInvalidOrOverflowingNumbers)
{
    std::uint64_t value = 0U;

    EXPECT_FALSE(platform::ArgumentParser::parseUnsigned("", 10U, value));
    EXPECT_FALSE(platform::ArgumentParser::parseUnsigned("-1", 10U, value));
    EXPECT_FALSE(platform::ArgumentParser::parseUnsigned("1G", 16U, value));
    EXPECT_FALSE(platform::ArgumentParser::parseUnsigned("18446744073709551616", 10U, value));
    EXPECT_FALSE(platform::ArgumentParser::parseUnsigned("10", 8U, value));
}