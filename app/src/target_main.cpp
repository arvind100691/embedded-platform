#include "platform/bsp/stm32f103_board/Board.hpp"

#include "app/Application.hpp"

int main()
{
    platform::bsp::stm32f103_board::init();

    app::Application application(platform::bsp::stm32f103_board::statusLed());

    if (!application.initialize())
    {
        while (true)
        {
        }
    }

    while (true)
    {
        // Application scheduler will be added later.
    }
}