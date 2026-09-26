#include "platform/common/ErrorCode.hpp"

#include <gtest/gtest.h>

namespace
{

TEST(ErrorCodeTest, OkRepresentsSuccess)
{
    EXPECT_EQ(platform::ErrorCode::Ok, platform::ErrorCode::Ok);
}

TEST(ErrorCodeTest, IsSuccessReturnsTrueForOk)
{
    EXPECT_TRUE(platform::isSuccess(platform::ErrorCode::Ok));
}

TEST(ErrorCodeTest, IsSuccessReturnsFalseForError)
{
    EXPECT_FALSE(platform::isSuccess(platform::ErrorCode::Timeout));
}

TEST(ErrorCodeTest, ErrorCodeIsAnEnumClass)
{
    EXPECT_NE(platform::ErrorCode::Ok, platform::ErrorCode::InternalError);
}

} // namespace