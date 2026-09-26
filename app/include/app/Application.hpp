#pragma once

#include "platform/hal/IGpio.hpp"

namespace app
{

class Application
{
public:
    explicit Application(platform::hal::IGpio& statusLed);

    bool initialize();

private:
    platform::hal::IGpio& statusLed_;
};

} // namespace app