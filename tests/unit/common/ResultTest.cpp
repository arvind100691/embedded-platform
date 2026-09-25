#include <gtest/gtest.h>

#include "platform/common/ErrorCode.hpp"
#include "platform/common/Result.hpp"

namespace
{

TEST(ResultTest, SuccessfulResultContainsValue)
{
    const auto result =
        platform::Result<int>::success(42);

    EXPECT_TRUE(result.hasValue());
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(result.error(),
              platform::ErrorCode::Ok);

    ASSERT_NE(result.value(), nullptr);
    EXPECT_EQ(*result.value(), 42);
}

TEST(ResultTest, FailedResultContainsError)
{
    const auto result =
        platform::Result<int>::failure(
            platform::ErrorCode::Timeout);

    EXPECT_FALSE(result.hasValue());
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error(),
              platform::ErrorCode::Timeout);

    EXPECT_EQ(result.value(), nullptr);
}

TEST(ResultTest, VoidSuccessWorks)
{
    const auto result =
        platform::Result<void>::success();

    EXPECT_TRUE(result.hasValue());
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(result.error(),
              platform::ErrorCode::Ok);
}

TEST(ResultTest, VoidFailureWorks)
{
    const auto result =
        platform::Result<void>::failure(
            platform::ErrorCode::CommunicationError);

    EXPECT_FALSE(result.hasValue());
    EXPECT_FALSE(static_cast<bool>(result));

    EXPECT_EQ(
        result.error(),
        platform::ErrorCode::CommunicationError);
}

} // namespace