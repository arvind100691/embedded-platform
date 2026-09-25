# Embedded Platform

A modular, portable embedded software platform designed around layered architecture, explicit dependencies, testability, and MCU portability.

The platform is being built initially around an **STM32F103** target, with the architecture designed to allow additional MCUs and operating-system implementations to be added without changing application-level code.

## Goals

- Separate product/application logic from hardware-specific implementation.
- Keep MCU-specific code isolated behind platform abstractions.
- Support multiple operating-system implementations, including FreeRTOS and bare-metal execution where practical.
- Make reusable middleware independent of a specific MCU.
- Make common runtime libraries testable on a Linux host.
- Provide reproducible builds through CMake and a Docker-based development environment.
- Keep unit and integration tests in the repository and integrate them with CTest and the VS Code Testing UI.
- Provide host-side utilities for flashing, debugging, firmware-image processing, logging, and other development tasks.
- Establish a foundation that can later support capabilities such as firmware/software update, bootloader integration, diagnostics, communication stacks, and hardware-in-the-loop testing.

## Architecture

The intended dependency direction is:

```text
+-------------------------+
|      Application        |
+------------+------------+
             |
             v
+-------------------------+
| Platform abstractions   |
+------------+------------+
             |
     +-------+--------+
     |                |
     v                v
+----------+     +-----------+
|Middleware|     | OS layer  |
+----+-----+     +-----+-----+
     |                 |
     +--------+--------+
              |
              v
+-------------------------+
| MCU / hardware port    |
| e.g. STM32F103         |
+------------+------------+
             |
             v
+-------------------------+
| MCU HAL / CMSIS / SDK   |
+------------+------------+
             |
             v
+-------------------------+
|        Hardware         |
+-------------------------+
```

The application should depend on interfaces and platform services rather than directly including MCU HAL or OS headers.

For more detail, see [Platform Layer Architecture](docs/architecture/platform_layers.md).

## Repository Structure

```text
embedded-platform/
|
+-- CMakeLists.txt
+-- CMakePresets.json
+-- Dockerfile
+-- .dockerignore
+-- .gitignore
+-- README.md
|
+-- app/                         # Product/application code
|   +-- CMakeLists.txt
|   +-- include/
|   +-- src/
|
+-- platform/
|   +-- common/                  # Hardware/OS-independent runtime foundation
|   +-- hal/                     # Hardware abstraction interfaces and related definitions
|   +-- os/                      # OS abstractions and implementations
|   +-- middleware/              # Reusable runtime middleware
|   +-- ports/                   # MCU-specific ports
|   |   +-- stm32/
|   |       +-- stm32f103/
|   |
|   +-- bsp/                     # Board-specific configuration and initialization
|
+-- tests/
|   +-- unit/                    # Unit tests
|   +-- integration/             # Integration tests
|   +-- mocks/                   # Test doubles and mocks
|
+-- external/                    # Third-party dependencies
|   +-- googletest/
|   +-- freertos/                # Added as the platform adopts FreeRTOS
|
+-- tools/
|   +-- cmake/                   # Toolchains and CMake helper modules
|   +-- flash/                   # Flashing/programming utilities
|   +-- debug/                   # Debug server and debugger configuration
|   +-- scripts/                 # Developer/build automation scripts
|   +-- host/                    # Linux-native utilities
|
+-- config/                      # Platform, board, and application configuration
|
+-- docs/
    +-- architecture/           # Architecture documentation
```

## Build Environment

The development and build environment is provided through Docker.

The container is intended to provide a consistent set of tools for:

- C/C++ development
- CMake and Ninja builds
- ARM GNU Embedded Toolchain
- GDB / GDB Multiarch
- OpenOCD
- GoogleTest / CTest
- Clang tooling
- Static analysis
- Code coverage
- Python-based host utilities

The recommended workflow is to open the repository in the VS Code Dev Container so the same containerized toolchain is used for development, testing, and target builds.

## Build Configurations

The repository is intended to support at least two classes of builds:

### Host build

Used for:

- Unit tests
- Host-side integration tests
- Algorithm and state-machine testing
- Static analysis
- Other Linux-native validation

Example:

```bash
cmake --preset host-debug
cmake --build --preset host-debug
ctest --test-dir build/host-debug --output-on-failure
```

### Target build

Used for MCU firmware, initially STM32F103.

The target build will use the ARM GNU Embedded Toolchain through a CMake toolchain file under `tools/cmake/toolchains/`.

## Testing

Testing is a first-class part of the platform architecture.

The repository separates:

- **Unit tests** — verify individual components in isolation, normally on the host.
- **Integration tests** — verify interactions between platform components and middleware.
- **Hardware integration / HIL** — planned for later stages where real target hardware is required.

GoogleTest is kept under `external/`, while CTest provides the top-level test execution integration.

## Development Rules

### Dependency direction

Higher-level code may depend on lower-level abstractions, but lower-level platform code must not depend on application code.

In particular:

```text
Application -> Platform abstractions -> Platform implementations -> MCU/HAL
```

The reverse dependency is not allowed.

### Common layer

`platform/common` is reserved for genuinely portable runtime code such as:

- Fixed-width types
- Error handling
- Result types
- Containers
- Algorithms
- Serialization helpers
- Checksums / CRC implementations
- Memory utilities
- Other hardware-independent runtime utilities

Subsystems such as software update may use these utilities but should not turn `common` into a collection of product-specific services.

### MCU isolation

MCU-specific implementation belongs under `platform/ports/`.

For the initial target:

```text
platform/ports/stm32/stm32f103/
```

Application code must not directly depend on STM32 HAL symbols.

### Board isolation

Board-specific pin mappings, peripheral selections, clock choices, and initialization belong under `platform/bsp/` and configuration directories, not inside generic MCU drivers.

## Current Progress

### Step 1 — Build Environment

Completed:

- Docker-based development environment
- CMake project bootstrap
- CMake presets
- Host and ARM GNU Embedded toolchain availability
- VS Code Dev Container configuration

### Step 2 — Common Platform Foundation

Completed:

- Fixed-width platform types
- `ErrorCode`
- `Result<T>` / `Result<void>`
- `NonCopyable`
- `platform_common` CMake target

### Step 3 — Testing Foundation

Completed:

- GoogleTest as an external dependency
- CTest integration
- Unit-test target structure
- Initial tests for the common platform foundation

### Next

Step 4 will define the platform abstraction interfaces and their ownership boundaries before implementing STM32F103-specific drivers.

## Planned Evolution

The platform is expected to evolve through the following areas:

```text
Common foundation
      |
      v
Hardware / OS interfaces
      |
      v
STM32F103 port
      |
      +---- FreeRTOS implementation
      |
      +---- Bare-metal implementation
      |
      v
Middleware
      |
      +---- Communication
      +---- Diagnostics
      +---- Logging
      +---- Protocols
      +---- Software update
      |
      v
Application
      |
      v
Hardware integration / HIL
```

Additional MCU ports can be introduced by implementing the established platform interfaces rather than changing application logic.

## License

This project is licensed under the **GNU General Public License v3.0 or later**. See [LICENSE](LICENSE).

Third-party components under `external/` retain their own licenses and notices.
