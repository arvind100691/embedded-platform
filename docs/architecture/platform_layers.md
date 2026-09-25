# Embedded Platform Layer Architecture

## 1. Purpose

The Embedded Platform is intended to provide a reusable software foundation for embedded products while keeping application behavior independent of a specific microcontroller, RTOS, or vendor SDK.

The first target is the STM32F103. The architecture must allow another MCU or execution environment to be introduced by adding or replacing implementations behind stable interfaces.

The primary architectural goals are:

1. Hardware portability.
2. Operating-system portability where practical.
3. Host-based testability.
4. Explicit build-time dependencies.
5. Separation of product code from platform code.
6. Reusable middleware and runtime utilities.
7. Reproducible development and build environments.

---

## 2. Architectural Model

The platform follows a layered dependency model:

```text
                         +-------------------+
                         |   Application     |
                         | Product behavior  |
                         +---------+---------+
                                   |
                                   v
                    +---------------------------+
                    | Platform Abstractions     |
                    | Interfaces / Services     |
                    +-------------+-------------+
                                  |
                    +-------------+-------------+
                    |                           |
                    v                           v
          +------------------+        +------------------+
          |    Middleware    |        |     OS Layer     |
          | Communication    |        | FreeRTOS         |
          | Diagnostics      |        | Bare-metal       |
          | Logging          |        | Tasks / Mutexes  |
          | Protocols        |        | Queues / Timers  |
          +--------+---------+        +--------+---------+
                   |                           |
                   +-------------+-------------+
                                 |
                                 v
                    +---------------------------+
                    | Hardware / MCU Port       |
                    | STM32F103 implementation  |
                    +-------------+-------------+
                                  |
                                  v
                    +---------------------------+
                    | Vendor HAL / CMSIS / SDK  |
                    +-------------+-------------+
                                  |
                                  v
                    +---------------------------+
                    |         Hardware          |
                    +---------------------------+
```

### Core rule

Dependencies should flow from higher-level policy toward lower-level mechanisms.

The application must not directly depend on STM32 HAL, STM32 register definitions, or FreeRTOS APIs when an abstraction is intended to shield it.

---

## 3. Repository Layer Mapping

The architectural layers map to repository directories as follows:

| Directory | Responsibility |
|---|---|
| `app/` | Product/application behavior |
| `platform/common/` | Hardware-independent runtime foundation |
| `platform/hal/` | Hardware-related abstractions and definitions |
| `platform/os/` | OS abstractions and OS-specific implementations |
| `platform/middleware/` | Reusable runtime middleware and services |
| `platform/ports/` | MCU/vendor-specific implementations |
| `platform/bsp/` | Board-specific configuration and initialization |
| `tests/` | Unit, integration, and test doubles |
| `external/` | Third-party dependencies |
| `tools/` | Host-side build, flash, debug, automation, and utilities |
| `config/` | Build/platform/board/application configuration |
| `docs/` | Architecture and development documentation |

---

## 4. Common Platform Layer

`platform/common` is the lowest portable runtime layer.

It must not depend on:

- STM32 HAL or LL headers
- FreeRTOS headers
- Application modules
- Product-specific implementations

Current foundation components include:

```text
platform/common/
└── include/platform/common/
    ├── Types.hpp
    ├── ErrorCode.hpp
    ├── Result.hpp
    └── NonCopyable.hpp
```

The layer will later host reusable components such as:

- Fixed-capacity containers
- Ring buffers
- CRC/checksum algorithms
- Serialization helpers
- Memory utilities
- Time/value types
- Portable state-machine support
- Other target-independent runtime utilities

### Software update utilities

Software/firmware update functionality will use the common utilities where appropriate, but the complete update subsystem belongs above the generic utility layer because it introduces system-level concepts such as image metadata, validation, storage, update state, activation, and recovery.

A possible future structure is:

```text
platform/common/
└── utilities/
    ├── crc/
    ├── containers/
    └── memory/

platform/middleware/
└── software_update/
    ├── FirmwareImage
    ├── FirmwareMetadata
    ├── SoftwareUpdater
    ├── IUpdateStorage
    └── IUpdateTransport
```

---

## 5. Platform Abstractions

The abstraction layer provides stable interfaces that can be implemented for different targets.

Planned interfaces include:

```text
Hardware
--------
IGpio
IUart
ISpi
II2c
ICan
IAdc
IPwm
ITimer
IWatchdog
IFlashStorage
IClock

OS
--
ITask
IMutex
ISemaphore
IQueue
IEvent

Services
--------
ILogger
```

The exact ownership of each interface will be finalized before implementation. Hardware-facing interfaces should remain independent of vendor-specific types.

For example, a GPIO interface should expose platform-level concepts rather than `GPIO_TypeDef`, `GPIO_PIN_x`, or `HAL_GPIO_*` symbols.

---

## 6. OS Layer

The OS layer isolates operating-system mechanisms behind selected interfaces.

The initial architecture supports the possibility of:

```text
Application / Middleware
          |
          v
   OS abstraction
      /       \
     v         v
FreeRTOS    Bare-metal
```

The goal is not to reproduce the entire FreeRTOS API. Only capabilities required for portability and architectural isolation should be abstracted.

Examples:

```text
ITask
IMutex
IQueue
ISemaphore
ITimer
```

FreeRTOS-specific APIs should remain inside the FreeRTOS implementation.

---

## 7. Middleware Layer

Middleware contains reusable runtime services that build on platform abstractions.

Expected areas include:

```text
platform/middleware/
├── communication/
├── diagnostics/
├── logging/
├── protocol/
├── storage/
└── software_update/
```

Middleware must not assume a particular MCU implementation.

For example:

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

The protocol implementation remains reusable when `ICan` is implemented by another MCU or a host-side test double.

---

## 8. MCU Port

MCU-specific implementation belongs under:

```text
platform/ports/
```

The initial port is:

```text
platform/ports/stm32/stm32f103/
```

Expected implementation classes include:

```text
Stm32Gpio
Stm32Uart
Stm32Spi
Stm32I2c
Stm32Can
Stm32Timer
Stm32Adc
Stm32Pwm
Stm32Watchdog
Stm32FlashStorage
```

These classes are allowed to include STM32 vendor headers and use STM32 HAL/LL APIs.

The application is not.

---

## 9. BSP / Board Layer

The MCU is not the same thing as the board.

For example:

```text
STM32F103
   |
   +-- Board A
   |      +-- LED -> PC13
   |      +-- Sensor -> I2C1
   |      +-- UART -> USART1
   |
   +-- Board B
          +-- LED -> PB12
          +-- Sensor -> I2C2
          +-- UART -> USART2
```

Board-specific information belongs under:

```text
platform/bsp/
config/
```

Examples:

- Pin mappings
- Clock configuration
- Peripheral selection
- Board initialization
- Sensor addresses
- Board feature configuration

A generic STM32 driver should not know which application-level function a GPIO pin performs.

---

## 10. Application Layer

The application owns product behavior and should consume platform abstractions.

Example dependency:

```text
StatusLed
    |
    v
  IGpio
    |
    v
Stm32Gpio
```

The application should not contain:

```cpp
HAL_GPIO_WritePin(...)
GPIOA
GPIO_PIN_5
xTaskCreate(...)
```

Instead it should interact with abstractions such as:

```cpp
led.turnOn();
```

or:

```cpp
uart.transmit(data, size);
```

Composition of concrete implementations happens at the application/platform startup boundary.

---

## 11. Dependency Injection / Composition Root

Concrete platform objects are assembled at a controlled composition boundary.

Conceptually:

```text
                Composition Root
                       |
          +------------+------------+
          |                         |
          v                         v
     Stm32Gpio                  Stm32Uart
          |                         |
          +------------+------------+
                       |
                       v
                  Application
```

The composition root is allowed to know the selected MCU and board. Lower-level application modules remain independent of those choices.

This supports replacing implementations for tests:

```text
                       IGpio
                      /     \
                     v       v
              Stm32Gpio   MockGpio
```

---

## 12. Build Architecture

CMake targets should represent architectural boundaries.

Examples:

```text
platform_common
platform_os
platform_middleware
platform_stm32f103
application
platform_common_unit_tests
platform_middleware_unit_tests
application_unit_tests
```

A target should expose only the dependencies required by that layer.

The expected dependency direction is approximately:

```text
application
   |
   +--> platform_middleware
   +--> platform_os
   +--> platform_common

platform_middleware
   |
   +--> platform_common
   +--> selected platform interfaces

platform_os
   |
   +--> platform_common
   +--> FreeRTOS / bare-metal implementation

platform_stm32f103
   |
   +--> platform_common
   +--> STM32 HAL / CMSIS / SDK
```

The exact CMake target graph will be refined as the interfaces are introduced.

---

## 13. Host vs Target Builds

The project has two important execution environments.

### Host

```text
Linux container
    |
    v
GCC / Clang
    |
    v
Platform + tests
```

Used for:

- Unit tests
- Portable middleware testing
- Algorithms
- Parsers
- State machines
- Host utilities
- Static analysis

### Target

```text
Linux container
    |
    v
arm-none-eabi-gcc
    |
    v
STM32F103 firmware
```

Used for:

- MCU-specific drivers
- Board integration
- Firmware image generation
- Hardware integration tests
- Debugging / flashing

The project should maximize the amount of code that can be validated in the host environment.

---

## 14. Testing Architecture

Testing is external to the production dependency hierarchy.

```text
                 +---------------------+
                 |       tests/        |
                 +----------+----------+
                            |
                  +---------+---------+
                  |                   |
                  v                   v
              Unit tests        Integration tests
                  |                   |
                  +---------+---------+
                            |
                            v
                        Production
                         platform
```

### Unit tests

Unit tests isolate a single component and generally use mocks/fakes where hardware or OS functionality would otherwise be required.

### Integration tests

Integration tests verify interactions between multiple platform components, middleware modules, or services.

### Hardware integration

Later stages can add hardware-in-the-loop or board-based verification without changing the conceptual unit-test structure.

---

## 15. External Dependencies

Third-party source code belongs in:

```text
external/
```

Examples include:

```text
external/
├── googletest/
├── freertos/
└── other approved libraries
```

Each dependency should have:

- Pinned version or commit
- License information
- Source provenance
- Configuration needed to reproduce the build

Third-party dependencies retain their own licenses and notices.

---

## 16. Tools Architecture

The `tools` directory is for host-side developer and build tooling.

```text
tools/
├── cmake/
│   ├── toolchains/
│   └── modules/
├── flash/
├── debug/
├── scripts/
└── host/
```

### CMake tools

Contains toolchain files and reusable CMake helper modules.

### Flash tools

Contains utilities for programming and verifying firmware on target hardware.

### Debug tools

Contains debugger scripts, OpenOCD configurations, GDB commands, and related helpers.

### Scripts

Contains developer automation such as build, test, formatting, and packaging helpers.

### Host tools

Contains Linux-native utilities such as firmware-image inspection, packaging, protocol tools, log parsers, and CRC/image utilities.

---

## 17. Firmware / Software Update Architecture

Software update is considered a platform middleware/service rather than a low-level generic utility.

A future design can follow:

```text
                 SoftwareUpdater
                        |
            +-----------+-----------+
            |           |           |
            v           v           v
      IUpdateStorage ITransport  IVerifier
            |           |           |
            v           v           v
         Flash       UART/CAN    CRC/Hash/Signature
```

The platform can then support bootloader/application schemes such as single-slot or dual-slot updates depending on target constraints.

The reusable algorithms needed by the update service, such as CRC or serialization, remain in lower portable layers.

---

## 18. Architectural Rules

The following rules should be treated as design constraints:

1. Application code does not include vendor HAL headers.
2. Common portable code does not include MCU or RTOS headers.
3. MCU-specific code is isolated under `platform/ports`.
4. Board-specific configuration is isolated from generic MCU drivers.
5. Production code does not depend on test frameworks.
6. Tests may depend on production code, mocks, and GoogleTest.
7. Host tools remain separate from target runtime code.
8. CMake targets represent meaningful architectural boundaries.
9. Third-party libraries remain under `external/` and keep their licensing notices.
10. New abstractions should be introduced where they provide a real portability or testability boundary, not merely to wrap every underlying API.

---

## 19. Current Implementation Status

### Completed

- Docker-based development environment
- CMake project bootstrap
- Host CMake preset
- ARM GNU Embedded toolchain availability
- VS Code Dev Container configuration
- `platform_common` target
- Fixed-width platform types
- Common error codes
- `Result<T>` / `Result<void>`
- `NonCopyable`
- GoogleTest integration
- CTest integration
- Initial unit tests for the common layer

### Next architectural step

Define and review the platform abstraction interfaces before implementing the STM32F103 port.

The next set of interfaces will be evaluated for:

- ownership
- lifetime
- synchronous vs asynchronous behavior
- ISR safety
- error reporting
- timeout semantics
- buffer ownership
- concurrency requirements
- portability across different MCUs and host tests
