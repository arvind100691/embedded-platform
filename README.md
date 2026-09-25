# Embedded Platform

A modular and portable embedded software platform built around layered architecture, explicit dependencies, host-based testing, reproducible builds, and MCU/board portability.

The first target is **STM32F103**. The platform is intentionally structured so that application code depends on platform abstractions rather than STM32 HAL, MCU registers, or a specific RTOS implementation.

## Project Goals

- Separate application/product behavior from hardware-specific implementation.
- Isolate MCU-specific code behind stable platform abstractions.
- Support FreeRTOS and bare-metal execution where practical.
- Keep reusable middleware independent of a specific MCU.
- Make portable runtime code testable on a Linux host.
- Use CMake as the build system.
- Use Docker Dev Containers to provide a reproducible build/tooling environment.
- Keep unit and integration tests as first-class repository content.
- Use GoogleTest and CTest for host-side verification.
- Provide host-side tools for flashing, debugging, firmware-image processing, logging, and other development tasks.
- Provide a foundation for future communication stacks, diagnostics, software update, bootloader integration, HIL testing, and additional MCU ports.

## Current Status

The project currently has the following foundation implemented:

- Docker-based development environment.
- CMake + Ninja build flow.
- CMake Presets for host and STM32F103 configurations.
- C++17 / C11 project baseline.
- `platform/common` foundation with fixed-width types, error codes, `Result<T>`, and `NonCopyable`.
- Hardware abstraction interfaces under `platform/hal`.
- OS abstraction interfaces under `platform/os`.
- GoogleTest and CTest infrastructure.
- STM32CubeF1 kept as an external dependency.
- STM32F103 GCC target and linker configuration.
- First concrete MCU adapter: `Stm32Gpio`.
- STM32F103 board/BSP layer with clock and board-specific GPIO mapping.
- Firmware ELF/HEX/BIN/MAP generation.

The next development work is focused on verification of the GPIO/application path, followed by additional MCU peripherals and the OS implementation.

## Architecture

The intended dependency direction is:

```text
                         +----------------------+
                         |     Application      |
                         +----------+-----------+
                                    |
                                    v
                 +--------------------------------------+
                 | Platform Abstractions / Interfaces  |
                 +------------------+-------------------+
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
                       | MCU / Platform Port       |
                       | e.g. STM32F103            |
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

The application must not directly include STM32 HAL headers, vendor register definitions, or FreeRTOS APIs when an abstraction exists for that capability.

See [Platform Layer Architecture](docs/architecture/platform_layers.md).

## Repository Structure

```text
embedded-platform/
|
+-- CMakeLists.txt
+-- CMakePresets.json
+-- Dockerfile
+-- .dockerignore
+-- .gitignore
+-- LICENSE
+-- README.md
|
+-- app/                         # Product/application code
|   +-- CMakeLists.txt
|   +-- include/
|   +-- src/
|
+-- platform/
|   +-- common/                  # Portable runtime foundation/utilities
|   +-- hal/                     # Hardware abstraction interfaces
|   +-- os/                      # OS abstractions and implementations
|   +-- middleware/              # Reusable runtime middleware
|   +-- ports/                   # MCU-specific ports
|   |   +-- stm32/
|   |       +-- stm32f103/
|   |
|   +-- bsp/                     # Board-specific configuration/initialization
|
+-- tests/
|   +-- unit/                    # Unit tests
|   +-- integration/             # Integration tests
|   +-- mocks/                   # Test doubles
|
+-- external/                    # Third-party dependencies
|   +-- googletest/
|   +-- stm32cube-f1/
|   +-- freertos/                # Planned/when adopted
|
+-- tools/
|   +-- cmake/                   # Toolchains and CMake helper modules
|   +-- flash/                   # Flash/programming utilities
|   +-- debug/                   # Debug/OpenOCD/GDB configuration
|   +-- scripts/                 # Developer automation
|   +-- host/                    # Linux-native utilities
|
+-- config/                      # Build/platform/board/application configuration
|
+-- docs/
    +-- architecture/
    +-- development/
    +-- testing/
    +-- coding_standard.md
```

## Build Environment

The project is developed in a VS Code Dev Container built from the repository `Dockerfile`.

The container provides, among other tools:

- GCC/G++
- CMake
- Ninja
- ARM GNU Embedded Toolchain
- GDB Multiarch
- OpenOCD
- Clang/clangd/clang-tidy
- cppcheck
- gcovr
- Python

The intended workflow is to clone/open the repository in the Dev Container so host and target builds use the same toolchain environment.

## Build Configurations

### Host Debug

Used for portable application/platform development and host-side tests.

```bash
cmake --preset host-debug
cmake --build --preset host-debug
ctest --test-dir build/host-debug --output-on-failure
```

### STM32F103 Debug

Uses the ARM GNU Embedded Toolchain and builds the target firmware.

```bash
cmake --preset stm32f103-debug
cmake --build --preset stm32f103-debug
```

The target build currently produces:

```text
embedded_firmware.elf
embedded_firmware.hex
embedded_firmware.bin
embedded_firmware.map
```

The `.elf` is the debugger/symbol-rich build artifact. HEX and BIN are generated target images. The MAP file is used for link/memory inspection.

## Testing

Testing is part of the architecture rather than an afterthought.

### Unit tests

Run primarily on the host and isolate the component under test using mocks/fakes where appropriate.

### Integration tests

Verify interaction between multiple platform layers, middleware components, or services.

### Hardware integration / HIL

Planned for later stages when real STM32 hardware and host-side automation are introduced.

GoogleTest is kept under `external/` and CTest is used as the repository-level test runner.

## Hardware Abstraction

The current HAL contracts include:

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

Interfaces use platform-level types and semantics. MCU/vendor types are restricted to the MCU port.

Examples:

```text
Application
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

GPIO currently models input/output direction, pull configuration, initial state, interrupt edge selection, callback registration, and interrupt enable/disable.

## STM32F103 Port and BSP

The MCU port is located at:

```text
platform/ports/stm32/stm32f103/
```

It contains MCU-specific adapters, STM32 HAL configuration, CMSIS startup/system files, and the STM32F103 linker configuration.

The board layer is separate:

```text
platform/bsp/stm32f103_board/
```

Board code owns details such as physical pin mapping and board clock configuration.

For example, the current example maps the status LED to `PC13`. Application code receives an `IGpio` reference and does not need to know the physical pin.

## Common Layer

`platform/common` is reserved for code that is genuinely independent of MCU, vendor HAL, and RTOS implementation.

Current foundation:

```text
Types.hpp
ErrorCode.hpp
Result.hpp
NonCopyable.hpp
```

Future portable utilities may include fixed-capacity containers, ring buffers, CRC/checksum, serialization, memory helpers, and reusable firmware-image algorithms.

Complete software-update functionality belongs above the generic utility layer because it introduces system-level concepts such as image metadata, validation, storage, activation, recovery, and update state.

## Tools

`tools/` is for host-side development and build infrastructure:

```text
tools/
+-- cmake/
|   +-- toolchains/
|   +-- modules/
|
+-- flash/
+-- debug/
+-- scripts/
+-- host/
```

The firmware itself must not depend on host tools.

## External Dependencies

Third-party dependencies are kept under `external/` and their versions should be pinned and documented.

Current dependencies include:

- GoogleTest for unit testing.
- STM32CubeF1 for STM32F1 CMSIS/HAL/device support.

Vendor and third-party code remains under its upstream licensing terms. The project license does not automatically replace the licenses of bundled third-party components.

## Development Principles

### Dependency direction

```text
Application
    -> Platform abstractions
        -> Platform implementations
            -> Vendor HAL / SDK
                -> Hardware
```

The reverse direction is not allowed.

### MCU and board separation

```text
MCU port
    -> How a peripheral is implemented on the MCU

BSP
    -> Which peripherals/pins/clocks the specific board uses
```

### Host testability

Portable logic should compile and execute on Linux whenever practical. Hardware-specific code should remain behind interfaces or dedicated integration-test boundaries.

### Determinism

Avoid uncontrolled dynamic allocation, hidden blocking, exceptions, and other behavior that makes embedded timing/resource usage unpredictable.

See [Coding Standard](docs/coding_standard.md).

## License

This project is released under the **GNU General Public License v3.0 or later**. See [LICENSE](LICENSE).

Third-party dependencies under `external/` may have separate licenses; consult the respective upstream license files.
