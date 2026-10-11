#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace platform
{

class ArgumentParser
{
public:
    explicit ArgumentParser(std::string_view input) noexcept
        : input_(input)
    {
    }

    bool next(std::string_view& token) noexcept
    {
        skipWhitespace();
        if (position_ == input_.size())
        {
            token = {};
            return false;
        }

        const std::size_t start = position_;
        while (position_ < input_.size() && !isWhitespace(input_[position_]))
        {
            ++position_;
        }

        token = input_.substr(start, position_ - start);
        return true;
    }

    [[nodiscard]] std::string_view remaining() noexcept
    {
        skipWhitespace();
        return input_.substr(position_);
    }

    [[nodiscard]] bool hasRemaining() noexcept
    {
        skipWhitespace();
        return position_ < input_.size();
    }

    static bool parseUnsigned(std::string_view token, unsigned int base,
                              std::uint64_t& value) noexcept
    {
        if (base != 10U && base != 16U)
        {
            return false;
        }

        if (base == 16U && token.size() >= 2U && token[0] == '0' &&
            (token[1] == 'x' || token[1] == 'X'))
        {
            token.remove_prefix(2U);
        }
        if (token.empty())
        {
            return false;
        }

        std::uint64_t parsed = 0U;
        for (const char character : token)
        {
            const int digit = digitValue(character);
            if (digit < 0 || static_cast<unsigned int>(digit) >= base ||
                parsed > (std::numeric_limits<std::uint64_t>::max() -
                          static_cast<std::uint64_t>(digit)) /
                             base)
            {
                return false;
            }
            parsed = (parsed * base) + static_cast<std::uint64_t>(digit);
        }

        value = parsed;
        return true;
    }

private:
    static bool isWhitespace(char character) noexcept
    {
        return character == ' ' || character == '\t';
    }

    static int digitValue(char character) noexcept
    {
        if (character >= '0' && character <= '9')
        {
            return character - '0';
        }
        if (character >= 'A' && character <= 'F')
        {
            return character - 'A' + 10;
        }
        if (character >= 'a' && character <= 'f')
        {
            return character - 'a' + 10;
        }
        return -1;
    }

    void skipWhitespace() noexcept
    {
        while (position_ < input_.size() && isWhitespace(input_[position_]))
        {
            ++position_;
        }
    }

    std::string_view input_;
    std::size_t position_{0U};
};

} // namespace platform