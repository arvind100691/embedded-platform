# Embedded Platform Layer Architecture

## 1. Purpose

The Embedded Platform provides a reusable software foundation for embedded products while keeping application behavior independent of a specific MCU, board, vendor SDK, or RTOS implementation where practical.

The first target is STM32F103. The architecture is intentionally designed so another MCU or execution environment can be introduced by adding implementations behind stable contracts rather than rewriting application logic.

## 2. Architectural Principles

The architecture is based on these principles:

1. Application code expresses product behavior and policy.
2. Hardware access occurs through platform abstractions.
3. MCU-specific details are contained within platform ports.
4. Board-specific details are contained within BSP/configuration.
5. OS-specific behavior is contained inside the OS implementation.
6. Common runtime code has no MCU/RTOS dependency.
7. Middleware is reusable and built on platform contracts.
8. Portable logic should be testable on Linux.
9. Build, test, flash, and debug tooling is kept separate from target runtime code.
10. Dependencies flow in one direction from higher-level policy toward lower-level mechanisms.

## 3. High-Level Model

```text
                           +----------------------+
                           |     Application      |
                           |   Product behavior   |
                           +----------+-----------+
                                      |
                                      v
                +-------------------------------------------+
                | Platform Abstraction / Service Contracts |
                +---------------------+---------------------+
                                      |
                      +---------------+---------------+
                      |                               |
                      v                               v
               +-------------+                 +-------------+
               | Middleware  |                 |   OS Layer  |
               +------+------+                 +------+------+ 
                      |                               |
                      +---------------+---------------+
                                      |
                                      v
                         +---------------------------+
                         |   MCU / Platform Port     |
                         |       STM32F103           |
                         +-------------+-------------+
                                       |
                                       v
                         +---------------------------+
                         | Vendor HAL / CMSIS / SDK  |
                         +-------------+-------------+
                                       |
                                       v
                                   Hardware
```

The `tests/`, `tools/`, and `external/` areas are supporting domains and are not runtime layers in this dependency stack.

## 4. Repository Mapping

| Directory | Responsibility |
|---|---|
| `app/` | Product/application behavior |
| `platform/common/` | Portable runtime foundation and utilities |
| `platform/hal/` | Hardware abstraction interfaces |
| `platform/os/` | OS abstractions and implementations |
| `platform/middleware/` | Reusable runtime middleware/services |
| `platform/ports/` | MCU/vendor-specific implementations |
| `platform/bsp/` | Board-specific mapping and initialization |
| `tests/` | Unit/integration tests and test doubles |
| `external/` | Third-party dependencies |
| `tools/` | Host-side build/flash/debug/scripts/utilities |
| `docs/` | Architecture, coding, development, and testing documentation |

> Current repository note: the checked-in tree does not include a top-level config/ directory, and the repo currently uses the active source layout above as the canonical structure. Any legacy references to config/ or additional host utility folders should be treated as planned work rather than current implementation.

## 5. Common Layer

`platform/common` is the lowest portable runtime layer.

It must not depend on:

- STM32 HAL/LL headers.
- CMSIS device registers.
- FreeRTOS APIs.
- Application modules.
- Product-specific platform implementations.

The current foundation includes:

```text
platform/common/include/platform/common/
    Types.hpp
    ErrorCode.hpp
    Result.hpp
    NonCopyable.hpp
```

The layer may later contain reusable facilities such as:

- Fixed-capacity containers.
- Ring buffers.
- CRC/checksum algorithms.
- Serialization/deserialization helpers.
- Portable memory utilities.
- Time/value types.
- Other hardware-independent runtime algorithms.

### Software update relationship

Generic update-related algorithms may use `platform/common` facilities, but a complete software-update service belongs at the middleware/service level because it introduces system-level behavior.

A future arrangement can be:

```text
platform/common/
    utilities/
        crc/
        containers/
        memory/

platform/middleware/
    software_update/
        FirmwareImage
        FirmwareMetadata
        SoftwareUpdater
        IUpdateStorage
        IUpdateTransport
```

## 6. Hardware Abstraction Layer

The HAL defines capabilities without exposing vendor details.

Current interface set:

```text
IGpio
IUart
IAsyncUart
ISpi
II2c
ICan
IAdc
IPwm
ITimer
IWatchdog
IFlashStorage
IClock
```

Interfaces should use platform-owned types/enums and standard C++ types rather than `GPIO_TypeDef`, `UART_HandleTypeDef`, `HAL_StatusTypeDef`, or other vendor types.

### GPIO contract

GPIO currently models:

```text
Direction
    Input
    Output

Pull
    None
    PullUp
    PullDown

State
    Low
    High

Interrupt edge
    None
    Rising
    Falling
    Both

Interrupt callback
Enable / Disable interrupt
```

The callback is represented by a function pointer plus a context pointer to avoid requiring dynamic allocation at this low-level boundary.

### Asynchronous capability

Asynchronous behavior is separated where it would otherwise make a basic interface unnecessarily complicated. For example, `IUart` provides synchronous transfer while `IAsyncUart` provides asynchronous transfer and callbacks.

This capability-oriented approach prevents every implementation from having to support every possible transfer model.

## 7. OS Layer

The OS layer isolates operating-system primitives behind selected abstractions.

Current planned contracts include:

```text
ITask
IMutex
ISemaphore
IQueue
IEvent
```

The goal is not to reproduce an entire RTOS API. Only platform/application capabilities that benefit from portability and testing should be abstracted.

Potential implementations:

```text
platform/os/
    freertos/
    baremetal/
```

FreeRTOS-specific types such as `TaskHandle_t` and `SemaphoreHandle_t` must remain inside the FreeRTOS implementation.

## 8. Implementation Notes for Current Structure

The current codebase is already demonstrating the intended layering, but a few boundaries still need cleanup so the architecture remains explicit as the project grows.

### 8.1 Board configuration vs MCU port ownership

The BSP currently owns board-specific pin routing and clock setup while the STM32 port owns more generic MCU implementation details. This is the correct direction, but the boundary should remain explicit:

```text
MCU port: HAL adaptation, driver logic, vendor integration
BSP: board-specific clocks, pin mapping, routing, IRQ context
user/application config: product policy and hardware selection
```

The project should avoid letting board facts leak into portable platform interfaces or application code.

### 8.2 Vector/interrupt ownership

The current integration-test strategy includes IRQ source files directly and relies on linker retention for specific handlers. This is workable for the current milestone, but it is not a final ownership model.

The architectural goal should be:

```text
one well-defined place for MCU interrupt vectors
clear ownership of each IRQ handler
linker retention handled by the vector layer, not by each individual test target
```

This keeps interrupt ownership aligned with the lower-level BSP/port boundary instead of distributing workarounds throughout the test tree.

### 8.3 Fault handling boundary

The project now uses a layered fault-handling structure that keeps the exception path cleanly separated by ownership:

```text
platform/common/Fault.hpp
    -> portable fault record structure

platform/ports/stm32/stm32f103/HardFaultHandler.hpp
    -> MCU-specific register capture contract

platform/ports/stm32/stm32f103/src/HardFaultHandler.cpp
    -> capture stack and SCB fault state

app/src/FaultHandler.cpp
    -> final HardFault_Handler override
    -> console UART dump and infinite fault loop
```

This design keeps the port layer independent from BSP code while still letting the final exception vector be a strong override in the application layer. The CPU still reaches the handler through the standard Cortex-M exception vector table; the strong symbol definition replaces the weak startup handler during linking.

## 9. Middleware

Middleware provides reusable runtime services above the primitive hardware/OS abstractions.

Planned areas include:

```text
communication/
diagnostics/
logging/
protocol/
storage/
software_update/
```

Example dependency:

```text
DiagnosticProtocol
        |
        v
       ICan
        |
        v
    Stm32Can
        |
        v
   STM32 HAL
```

The protocol itself remains independent of the STM32 implementation.

### Console and CLI boundary

The CLI-facing console contract is transport-independent. The application uses
`IConsole`, while `UartConsole` adapts that contract to the HAL's `IUart`:

```text
Application / CLI
                |
                v
         IConsole
                ^
                |
     UartConsole
                |
                v
             IUart
                |
                v
    MCU UART driver
```

The composition root selects the board's UART and constructs `UartConsole`.
`ConsoleCli` polls the console, edits a fixed-capacity input line, echoes input,
and dispatches completed lines through a fixed-capacity `CommandRegistry`.
Application command groups live under `app/.../console/commands/` and register
their handlers with the registry; each group can be maintained independently.
The current LED command group supports `blink on` and `blink off`.

## 9. MCU Port

The first MCU port is:

```text
platform/ports/stm32/stm32f103/
```

The port contains:

- STM32-specific adapter classes.
- STM32 HAL configuration.
- CMSIS device support.
- Startup/system files.
- MCU-specific linker configuration.
- Other details that are inherently STM32F103-specific.

Current first adapter:

```text
Stm32Gpio
```

The port may include vendor headers because it is the implementation boundary.

## 10. BSP / Board Layer

An MCU and a physical board are different architectural concepts.

Example:

```text
                         STM32F103
                             |
                  +----------+----------+
                  |                     |
               Board A               Board B
                  |                     |
              LED -> PC13          LED -> PB12
              I2C -> I2C1          I2C -> I2C2
              UART -> USART1       UART -> USART2
```

The BSP owns physical configuration and board initialization. Generic STM32 peripheral classes should not assume which pin or peripheral instance a product uses.

Current board example:

```text
platform/bsp/stm32f103_board/
```

The board layer currently owns the example system clock configuration and status LED mapping.

## 11. Composition Root

Concrete implementations are assembled at startup in the platform/BSP/application composition area.

Conceptually:

```text
BSP / Composition Root
        |
        +--> Stm32Gpio
        |
        +--> Stm32Uart
        |
        +--> FreeRtosTask
        |
        +--> other implementations
        |
        v
Application
```

The application receives interfaces such as `IGpio&` rather than constructing STM32-specific implementations itself.

## 12. Dependency Rules

Allowed:

```text
Application
    -> Platform interfaces

Middleware
    -> Platform interfaces
    -> Common
    -> OS abstractions where required

MCU port
    -> Platform interfaces
    -> Vendor HAL/CMSIS

BSP
    -> Platform interfaces
    -> MCU port
    -> Vendor initialization when necessary

Tests
    -> Code under test
    -> Test doubles
    -> GoogleTest
```

Not allowed:

```text
Application -> STM32 HAL
Application -> FreeRTOS API
Common     -> STM32 HAL
Common     -> FreeRTOS
Vendor/HAL -> Application
```

## 13. Testing Architecture

```text
                         Tests
                       /       \
                      v         v
                    Unit    Integration
                      |         |
                      +----+----+
                           |
                           v
                        Platform
```

### Unit tests

Use host builds wherever practical and replace hardware/OS dependencies with fakes or mocks.

### Integration tests

Test interaction between multiple abstractions and implementations.

### Hardware integration/HIL

A later layer can connect host test tooling to a physical STM32 target.

## 14. Build Architecture

The project supports separate host and target configurations.

```text
                         CMake
                           |
                 +---------+---------+
                 |                   |
                 v                   v
             Host build         STM32 build
                 |                   |
              GCC/G++          ARM GCC
                 |                   |
          Unit/integration      Firmware ELF
              tests           HEX/BIN/MAP
```

`tools/cmake/toolchains/` contains cross-compilation toolchain files.

## 15. Vendor Dependency Boundary

STM32CubeF1 is kept under:

```text
external/stm32cube-f1/
```

The vendor code is consumed by our STM32F103 port but is not modified to implement product logic.

The current GCC startup file comes from the vendor CMSIS template:

```text
Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s
```

This is selected because the project uses the GNU ARM Embedded toolchain.

## 16. Current Vertical Slice

The first completed runtime path is:

```text
main()
  |
  v
STM32F103 Board BSP
  |
  v
IGpio
  |
  v
Stm32Gpio
  |
  v
STM32 HAL
  |
  v
STM32F103
```

The target currently builds successfully with the STM32F103 GCC preset and produces a firmware ELF plus derived HEX/BIN/MAP artifacts.

## 17. Future Extension

A second MCU should be introduced by adding a new port:

```text
platform/ports/
    stm32/
        stm32f103/
        stm32g4/
    nxp/
        ...
    renesas/
        ...
```

Application dependencies should remain unchanged where the selected abstraction remains semantically compatible.
