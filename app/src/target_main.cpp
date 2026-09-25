#include "app/Application.hpp"

#include "platform/bsp/stm32f103_board/Board.hpp"

int main()
{
    using namespace platform::bsp::stm32f103_board;

    init();

    app::Application application(
        statusLed());

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