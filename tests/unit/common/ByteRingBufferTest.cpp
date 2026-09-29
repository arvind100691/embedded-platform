#include "platform/common/ByteRingBuffer.hpp"

#include <array>
#include <gtest/gtest.h>

namespace
{

using TestBuffer = platform::ByteRingBuffer<4U>;

} // namespace

TEST(ByteRingBufferTest, StartsEmpty)
{
    TestBuffer buffer;

    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 0U);
    EXPECT_EQ(buffer.freeSpace(), buffer.capacity());
}

TEST(ByteRingBufferTest, PushAndPop)
{
    TestBuffer buffer;
    platform::Byte value = 0U;

    EXPECT_TRUE(buffer.push('A'));
    EXPECT_TRUE(buffer.push('B'));

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 'A');

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 'B');

    EXPECT_TRUE(buffer.empty());
}

TEST(ByteRingBufferTest, RejectsPushWhenFull)
{
    TestBuffer buffer;

    EXPECT_TRUE(buffer.push('A'));
    EXPECT_TRUE(buffer.push('B'));
    EXPECT_TRUE(buffer.push('C'));
    EXPECT_TRUE(buffer.push('D'));

    EXPECT_TRUE(buffer.full());
    EXPECT_FALSE(buffer.push('E'));
}

TEST(ByteRingBufferTest, RejectsPopWhenEmpty)
{
    TestBuffer buffer;
    platform::Byte value = 0U;

    EXPECT_FALSE(buffer.pop(value));
}

TEST(ByteRingBufferTest, WriteAndRead)
{
    TestBuffer buffer;

    constexpr std::array<platform::Byte, 3U> input{'A', 'B', 'C'};
    std::array<platform::Byte, 3U> output{};

    EXPECT_EQ(buffer.write(input.data(), input.size()), input.size());
    EXPECT_EQ(buffer.read(output.data(), output.size()), output.size());
    EXPECT_EQ(output, input);
}

TEST(ByteRingBufferTest, WrapsAround)
{
    TestBuffer buffer;
    platform::Byte value = 0U;

    EXPECT_TRUE(buffer.push('A'));
    EXPECT_TRUE(buffer.push('B'));

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 'A');

    EXPECT_TRUE(buffer.push('C'));
    EXPECT_TRUE(buffer.push('D'));
    EXPECT_TRUE(buffer.push('E'));

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 'B');

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 'C');

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 'D');

    EXPECT_TRUE(buffer.pop(value));
    EXPECT_EQ(value, 'E');

    EXPECT_TRUE(buffer.empty());
}

TEST(ByteRingBufferTest, ClearResetsBuffer)
{
    TestBuffer buffer;

    EXPECT_TRUE(buffer.push('A'));
    EXPECT_TRUE(buffer.push('B'));

    buffer.clear();

    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0U);
}
