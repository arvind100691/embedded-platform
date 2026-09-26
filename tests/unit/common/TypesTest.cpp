#include "platform/common/Types.hpp"

#include <gtest/gtest.h>
#include <type_traits>

namespace
{

TEST(TypesTest, ByteIsOneByte)
{
    EXPECT_EQ(sizeof(platform::Byte), 1U);
}

TEST(TypesTest, FixedWidthTypesHaveExpectedSize)
{
    EXPECT_EQ(sizeof(platform::UInt8), 1U);
    EXPECT_EQ(sizeof(platform::UInt16), 2U);
    EXPECT_EQ(sizeof(platform::UInt32), 4U);
    EXPECT_EQ(sizeof(platform::UInt64), 8U);

    EXPECT_EQ(sizeof(platform::Int8), 1U);
    EXPECT_EQ(sizeof(platform::Int16), 2U);
    EXPECT_EQ(sizeof(platform::Int32), 4U);
    EXPECT_EQ(sizeof(platform::Int64), 8U);
}

TEST(TypesTest, UInt32IsUnsigned)
{
    EXPECT_TRUE(std::is_unsigned_v<platform::UInt32>);
}

} // namespace