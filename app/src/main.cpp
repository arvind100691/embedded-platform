#include <iostream>

#include "platform/common/ErrorCode.hpp"
#include "platform/common/Result.hpp"
#include "platform/common/Types.hpp"

platform::Result<platform::UInt32> getExampleValue()
{
    return platform::Result<platform::UInt32>::success(42U);
}

int main()
{
    const auto result = getExampleValue();

    if (!result)
    {
        std::cout << "Error: "
                  << static_cast<int>(result.error())
                  << '\n';

        return 1;
    }

    std::cout << "Embedded Platform \n";
    std::cout << "Value: " << *result.value() << '\n';

    return 0;
}