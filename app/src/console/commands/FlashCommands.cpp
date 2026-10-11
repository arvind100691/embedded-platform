#include "app/console/commands/FlashCommands.hpp"

#include <array>
#include <limits>

#include "platform/common/ArgumentParser.hpp"

namespace
{

constexpr char kHexDigits[] = "0123456789ABCDEF";

void appendHexByte(char* output, std::size_t& size, std::uint8_t value)
{
    output[size++] = kHexDigits[value >> 4U];
    output[size++] = kHexDigits[value & 0x0FU];
}

} // namespace

namespace app::console::commands
{

FlashCommands::FlashCommands(platform::middleware::cli::IConsole& console,
                             platform::hal::IFlashStorage& flash) noexcept
    : console_(console)
    , flash_(flash)
{
}

platform::Result<void>
FlashCommands::registerCommands(platform::middleware::cli::CommandRegistry& registry) noexcept
{
    return registry.registerCommandWithArgs("flash", &FlashCommands::handle, this);
}

void FlashCommands::handle(void* context, std::string_view arguments)
{
    static_cast<FlashCommands*>(context)->execute(arguments);
}

void FlashCommands::execute(std::string_view arguments)
{
    platform::ArgumentParser parser(arguments);
    std::string_view operation;
    if (!parser.next(operation))
    {
        writeUsage();
        return;
    }

    if (operation == "info" && !parser.hasRemaining())
    {
        const auto infoResult = flash_.getInfo();
        if (!infoResult)
        {
            writeError(infoResult.error());
            return;
        }
        static_cast<void>(console_.write("W25Q128: 16 MiB, 4096-byte erase sectors\r\n"));
        return;
    }

    std::string_view addressToken;
    std::uint64_t addressValue = 0U;
    if (!parser.next(addressToken) ||
        !platform::ArgumentParser::parseUnsigned(addressToken, 16U, addressValue) ||
        addressValue > std::numeric_limits<std::uint32_t>::max())
    {
        writeUsage();
        return;
    }
    const auto address = static_cast<std::uint32_t>(addressValue);

    if (operation == "read")
    {
        std::string_view lengthToken;
        std::uint64_t lengthValue = 0U;
        if (!parser.next(lengthToken) ||
            !platform::ArgumentParser::parseUnsigned(lengthToken, 10U, lengthValue) ||
            parser.hasRemaining() || lengthValue == 0U || lengthValue > kMaxReadBytes)
        {
            writeUsage();
            return;
        }

        std::array<std::uint8_t, kMaxReadBytes> data{};
        const std::size_t length = static_cast<std::size_t>(lengthValue);
        const auto readResult = flash_.read(address, data.data(), length);
        if (!readResult)
        {
            writeError(readResult.error());
            return;
        }
        writeReadData(address, data.data(), length);
        return;
    }

    if (operation == "write")
    {
        std::string_view bytesToken;
        if (!parser.next(bytesToken) || parser.hasRemaining() || bytesToken.empty() ||
            (bytesToken.size() % 2U) != 0U || bytesToken.size() > kMaxWriteBytes * 2U)
        {
            writeUsage();
            return;
        }

        std::array<std::uint8_t, kMaxWriteBytes> data{};
        const std::size_t size = bytesToken.size() / 2U;
        for (std::size_t index = 0U; index < size; ++index)
        {
            std::uint64_t byteValue = 0U;
            const std::string_view byteToken = bytesToken.substr(index * 2U, 2U);
            if (!platform::ArgumentParser::parseUnsigned(byteToken, 16U, byteValue))
            {
                writeUsage();
                return;
            }
            data[index] = static_cast<std::uint8_t>(byteValue);
        }

        const auto writeResult = flash_.write(address, data.data(), size);
        if (!writeResult)
        {
            writeError(writeResult.error());
            return;
        }
        static_cast<void>(console_.write("flash write complete; no erase was performed\r\n"));
        return;
    }

    if (operation == "erase" && !parser.hasRemaining())
    {
        const auto infoResult = flash_.getInfo();
        if (!infoResult)
        {
            writeError(infoResult.error());
            return;
        }
        const platform::hal::FlashInfo* const info = infoResult.value();
        if (info == nullptr || info->eraseBlockSize == 0U || (address % info->eraseBlockSize) != 0U)
        {
            writeUsage();
            return;
        }

        const auto eraseResult = flash_.erase(address, info->eraseBlockSize);
        if (!eraseResult)
        {
            writeError(eraseResult.error());
            return;
        }
        static_cast<void>(console_.write("flash sector erased\r\n"));
        return;
    }

    writeUsage();
}

void FlashCommands::writeUsage()
{
    constexpr std::string_view usage{
        "flash info | flash read <hex-address> <length 1-16> | "
        "flash write <hex-address> <hex-bytes> | flash erase <aligned-sector-address>\r\n"
        "Write does not erase. Erase affects one 4 KiB sector.\r\n"};
    static_cast<void>(console_.write(usage));
}

void FlashCommands::writeError(platform::ErrorCode error)
{
    std::string_view message{"flash operation failed\r\n"};
    switch (error)
    {
    case platform::ErrorCode::NotReady:
        message = "flash is not ready\r\n";
        break;
    case platform::ErrorCode::Timeout:
        message = "flash operation timed out\r\n";
        break;
    case platform::ErrorCode::InvalidArgument:
        message = "invalid flash address or data\r\n";
        break;
    case platform::ErrorCode::CommunicationError:
        message = "flash communication failed\r\n";
        break;
    case platform::ErrorCode::NotFound:
        message = "flash device not found\r\n";
        break;
    default:
        break;
    }
    static_cast<void>(console_.write(message));
}

void FlashCommands::writeReadData(std::uint32_t address, const std::uint8_t* data, std::size_t size)
{
    std::array<char, 3U + 8U + (kMaxReadBytes * 3U) + 2U> output{};
    std::size_t outputSize = 0U;
    output[outputSize++] = '0';
    output[outputSize++] = 'x';
    for (int shift = 28; shift >= 0; shift -= 4)
    {
        output[outputSize++] = kHexDigits[(address >> static_cast<unsigned int>(shift)) & 0x0FU];
    }
    output[outputSize++] = ':';
    for (std::size_t index = 0U; index < size; ++index)
    {
        output[outputSize++] = ' ';
        appendHexByte(output.data(), outputSize, data[index]);
    }
    output[outputSize++] = '\r';
    output[outputSize++] = '\n';
    static_cast<void>(console_.write(std::string_view(output.data(), outputSize)));
}

} // namespace app::console::commands