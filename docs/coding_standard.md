# Embedded Platform Coding Standard

**Status:** Initial project standard  
**Applies to:** Embedded Platform source code, tests, platform code, middleware, BSPs, MCU ports, and host-side utilities where practical.

This document defines the coding conventions and engineering practices for the Embedded Platform project. It combines generally accepted embedded-software practices with the conventions explicitly selected for this repository so far.

This document is a project coding standard, not a claim of compliance with MISRA, CERT, a medical-device standard, or any other external standard. Compliance with a regulated or industry-specific standard may require additional rules, reviews, analysis, documentation, and verification.

---

## 1. General Embedded C/C++ Practices

Embedded software should prioritize:

- Deterministic behavior.
- Predictable resource usage.
- Clear ownership of hardware and OS resources.
- Small, testable components.
- Explicit interfaces between layers.
- Portability across MCUs and boards.
- Compile-time checking where practical.
- Minimal hidden runtime behavior.
- Diagnosable failures.
- Reproducible builds.

For resource-constrained or safety-oriented products, project-specific restrictions may be stricter than the rules in this document.

### Common external guidance

Depending on the product domain, teams commonly use standards or guidance such as:

- **MISRA C** for C software.
- **MISRA C++** for C++ software.
- **CERT C / CERT C++** for secure coding guidance.
- **C++ Core Guidelines** for general C++ design guidance.
- Vendor-specific coding rules and hardware errata.
- Product or safety-process-specific rules when required by the domain.

The project should adopt only the rules that are compatible with its compiler, target, architecture, and verification strategy and should document any deviations.

---

# 2. Language Standard

## 2.1 C

C code uses **C11** unless a target-specific restriction requires an older dialect.

CMake currently specifies:

```cmake
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
```

## 2.2 C++

C++ code uses **C++17**.

CMake currently specifies:

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

Do not use C++ features newer than C++17 until the project standard is intentionally updated.

---

# 3. General C++ Rules

## 3.1 Prefer strong types

Prefer explicit types over ambiguous primitive types when the meaning matters.

Good:

```cpp
platform::UInt32 timeoutMs;
platform::Address address;
platform::Byte data[32];
```

Avoid cryptic or implementation-dependent declarations when a project type communicates the intent better.

## 3.2 Use fixed-width integer types for binary and hardware-facing data

Use the platform types defined in:

```text
platform/common/Types.hpp
```

which are based on `<cstdint>` and `<cstddef>`.

Examples:

```cpp
platform::UInt8
platform::UInt16
platform::UInt32
platform::UInt64
platform::Int32
platform::Size
platform::Address
```

This is especially important for protocol packets, register values, flash layouts, firmware metadata, and persistent structures.

## 3.3 Prefer `enum class`

Use scoped enumerations instead of unscoped enums where possible.

Good:

```cpp
enum class GpioState
{
    Low,
    High
};
```

This prevents accidental implicit conversion to unrelated integer values.

## 3.4 Use `const` and `constexpr`

Make data immutable whenever possible.

Prefer:

```cpp
constexpr platform::UInt32 BufferSize = 128U;
```

over a mutable global constant.

Use `const` for values that do not need to change after initialization.

Use `constexpr` when a value or function can be evaluated at compile time.

## 3.5 Prefer RAII for resource lifetime where appropriate

For C++ resources with deterministic lifetime, use objects whose lifetime clearly owns the resource.

Examples include:

- Locks.
- Temporary buffers.
- File/host-side resources.
- Scoped configuration changes.

For hardware resources, ownership must remain explicit and compatible with the target environment.

## 3.6 Avoid unnecessary dynamic allocation

The current platform design assumes:

- No uncontrolled heap allocation in core embedded paths.
- Prefer static or stack storage when the lifetime and size are known.
- Prefer fixed-capacity containers for real-time paths.
- Document unavoidable dynamic allocation.

Examples of preferred building blocks include fixed-size buffers, ring buffers, memory pools, and static queues.

We will add concrete allocation rules before introducing RTOS memory management.

---

# 4. Exceptions and RTTI

The current embedded-platform design assumes that the core target build will avoid:

- C++ exceptions.
- Run-time type information (RTTI).

Use explicit error handling instead.

For example:

```cpp
return platform::Result<void>::failure(
    platform::ErrorCode::Timeout);
```

rather than using exceptions for normal embedded error paths.

The corresponding compiler options will be made explicit in the target toolchain configuration when the STM32 build is added.

---

# 5. Error Handling

## 5.1 Do not use magic error numbers

Avoid:

```cpp
return -1;
```

Prefer the common error model:

```cpp
return platform::ErrorCode::Timeout;
```

or:

```cpp
return platform::Result<Data>::failure(
    platform::ErrorCode::CommunicationError);
```

## 5.2 Do not use `bool` when the caller needs an error reason

A boolean is appropriate when the only required information is success/failure.

When the caller needs to distinguish conditions, use `ErrorCode` or `Result<T>`.

## 5.3 Check error results

A returned error should not be silently ignored unless ignoring it is intentional and documented.

---

# 6. Interfaces and Dependency Injection

Hardware and operating-system dependencies should be represented by interfaces where portability and unit testing benefit from abstraction.

Example:

```cpp
class IGpio
{
public:
    virtual ~IGpio() = default;

    virtual void set(GpioState state) = 0;
    virtual GpioState get() const = 0;
    virtual void toggle() = 0;
};
```

Application code should depend on the abstraction:

```cpp
StatusLed(IGpio& gpio);
```

rather than directly depending on STM32 HAL calls.

This allows the same application logic to work with:

- STM32 implementation.
- Another MCU implementation.
- A fake/mock implementation for host tests.

---

# 7. Class Design

## 7.1 One responsibility per class

Classes should have a focused responsibility.

Examples:

```text
Stm32Uart       -> hardware UART access
SoftwareUpdater -> update-state management
FirmwareImage   -> firmware-image representation/validation
```

Do not create a large class that combines hardware access, protocol handling, application logic, and persistence.

## 7.2 Prefer composition over inheritance

Use inheritance primarily for genuine interfaces/polymorphic boundaries.

Example:

```text
IGpio
  |
  +-- Stm32Gpio
  +-- MockGpio
```

Application functionality should generally be composed from interfaces and services rather than using deep inheritance hierarchies.

## 7.3 Virtual interface classes need virtual destructors

For polymorphic interfaces, use:

```cpp
virtual ~IGpio() = default;
```

## 7.4 Make ownership explicit

Prefer references or pointers that clearly communicate ownership.

For non-owning dependencies, a reference is preferred when a dependency must always exist:

```cpp
StatusLed(IGpio& gpio);
```

Owning relationships should be explicit and reviewed before introducing heap allocation.

---

# 8. Naming Convention

The project currently follows these conventions.

## 8.1 Classes and structs

Use **PascalCase**:

```cpp
class StatusLed;
class SoftwareUpdater;
struct CanFrame;
```

## 8.2 Functions and methods

Use **camelCase**:

```cpp
readTemperature();
start();
getStatus();
```

## 8.3 Variables and parameters

Use **camelCase**:

```cpp
uint32_t timeoutMs;
uint8_t dataSize;
```

## 8.4 Private data members

The current code uses a trailing underscore:

```cpp
GPIO_TypeDef* port_;
uint16_t pin_;
```

Continue using this convention.

## 8.5 Constants

For scoped constants, prefer `constexpr` and a descriptive PascalCase name:

```cpp
constexpr platform::UInt32 MaxFrameSize = 64U;
```

Avoid preprocessor macros for typed constants.

## 8.6 Interfaces

Interfaces currently use the `I` prefix:

```text
IGpio
IUart
ISpi
II2c
ICan
ITimer
IFlashStorage
```

Maintain this convention consistently if the project continues using interface-based abstraction.

---

# 9. File and Header Rules

## 9.1 One main type per header where practical

Prefer focused headers:

```text
IGpio.hpp
IUart.hpp
Result.hpp
ErrorCode.hpp
```

## 9.2 Use `#pragma once`

The current project uses:

```cpp
#pragma once
```

for C++ headers.

## 9.3 Include what you use

A source/header file should include the declarations it directly depends on rather than relying on indirect includes.

## 9.4 Keep include dependencies minimal

Avoid including large framework or MCU headers in portable code.

For example, this is forbidden in `platform/common`:

```cpp
#include "stm32f1xx_hal.h"
```

and:

```cpp
#include "FreeRTOS.h"
```

---

# 10. Namespaces

Platform code uses the `platform` namespace.

Example:

```cpp
namespace platform
{

using UInt32 = std::uint32_t;

} // namespace platform
```

Nested namespaces should communicate ownership clearly.

Avoid importing an entire namespace with:

```cpp
using namespace std;
```

especially in headers.

---

# 11. Braces and Formatting

The project currently uses Allman-style braces for classes, functions, namespaces, and control blocks.

Example:

```cpp
if (value > limit)
{
    handleLimitExceeded();
}
```

Classes:

```cpp
class StatusLed
{
public:
    void turnOn();

private:
    IGpio& gpio_;
};
```

Namespaces:

```cpp
namespace platform
{

// Code

} // namespace platform
```

The repository should eventually enforce formatting with `.clang-format` rather than relying only on manual review.

---

# 12. `nullptr`, `override`, and `noexcept`

Use modern C++ forms:

Prefer:

```cpp
nullptr
```

over:

```cpp
NULL
```

Always use `override` when overriding a virtual method:

```cpp
void toggle() override;
```

Use `noexcept` when a function is guaranteed not to throw and the contract benefits from expressing that fact:

```cpp
bool hasValue() const noexcept;
```

---

# 13. Global State

Avoid mutable global state.

Do not create global hardware objects merely for convenience.

Prefer dependency construction at the composition root:

```text
main()
  |
  +-- create hardware implementations
  +-- create services
  +-- inject dependencies
  +-- start application
```

The composition root is allowed to know about concrete STM32 classes; application code is not.

---

# 14. Interrupt Service Routines

When interrupt-driven code is introduced, ISR rules must be explicit.

Until a detailed ISR standard is added, use these principles:

- Keep ISRs short and deterministic.
- Do not perform blocking operations in an ISR.
- Do not call arbitrary application code from an ISR.
- Avoid heap allocation in an ISR.
- Use a safe ISR-to-task/event handoff mechanism.
- Protect data shared between ISR and foreground/task contexts.
- Understand `volatile` as a visibility tool, not a synchronization primitive.

The ISR-specific rules will be expanded when the timer, DMA, UART, and RTOS layers are implemented.

---

# 15. Concurrency

For RTOS or multithreaded code:

- Define ownership of shared data.
- Minimize shared mutable state.
- Prefer message passing where appropriate.
- Keep critical sections short.
- Never hold a mutex while performing an operation that may block for an unbounded period unless explicitly justified.
- Document lock ordering when multiple locks can be held.
- Avoid deadlock-prone nested locking.

These rules will be refined when the OS abstraction layer is implemented.

---

# 16. Hardware Abstraction Rules

MCU-specific knowledge belongs in the MCU port.

For example:

```text
platform/ports/stm32/stm32f103/
```

may contain:

```cpp
GPIO_TypeDef*
HAL_GPIO_WritePin()
HAL_UART_Transmit()
```

Portable platform/application code must not directly depend on these APIs.

The intended dependency direction is:

```text
Application
    ↓
Platform interfaces
    ↓
Middleware / OS
    ↓
MCU port
    ↓
STM32 HAL / SDK
    ↓
Hardware
```

---

# 17. BSP Rules

The MCU port describes what the MCU can do.

The BSP describes how a particular board/product connects to that MCU.

For example:

```text
STM32F103
    |
    +-- GPIO peripheral
    +-- UART peripheral
    +-- SPI peripheral

STM32F103 Board
    |
    +-- LED -> PC13
    +-- Sensor -> I2C1
    +-- Debug UART -> USART1
```

Do not place board-specific pin mappings in generic STM32 driver code.

---

# 18. Logging and Diagnostics

Logging should eventually use a platform abstraction rather than direct `printf()` calls throughout application code.

Example intended direction:

```text
Application
    ↓
ILogger
    ↓
Logging implementation
    ↓
UART / RTT / ITM / file / host output
```

Do not introduce unrestricted logging into interrupt or hard real-time paths.

Log messages should be useful for diagnosis without changing timing-sensitive behavior unnecessarily.

---

# 19. Protocol and Binary Data

For protocol packets, flash metadata, firmware images, and persistent structures:

- Use explicitly sized integer types.
- Define endianness explicitly.
- Do not rely on compiler structure padding for a wire format.
- Prefer explicit serialization/deserialization.
- Validate lengths before indexing buffers.
- Validate ranges before converting values.
- Check CRC/hash/signature results before accepting critical data.

Binary formats should have a single shared definition where host tools and firmware both consume the same format.

---

# 20. Buffer Safety

Every externally controlled length must be checked before accessing a buffer.

Prefer APIs that carry both pointer and size:

```cpp
bool transmit(
    const uint8_t* data,
    size_t size);
```

Avoid APIs that rely on null termination when the data is binary.

Use bounded operations and fixed-capacity buffers in real-time paths whenever practical.

---

# 21. Testing Standard

Every reusable module should be designed so that its behavior can be tested independently.

The repository uses:

```text
GoogleTest
CTest
```

Unit tests belong under:

```text
tests/unit/
```

Integration tests belong under:

```text
tests/integration/
```

Mocks/fakes belong under:

```text
tests/mocks/
```

Production code must not depend on GoogleTest.

The preferred direction is:

```text
tests
    ↓
production target
```

not the reverse.

---

# 22. Host-Testability

Portable logic should be testable on Linux without requiring the physical STM32 target.

This applies especially to:

- Algorithms.
- Buffers.
- Parsers.
- State machines.
- Protocol handling.
- CRC/hash processing.
- Firmware image validation.
- Software-update state management.

Hardware-specific code should be isolated so that it can be tested independently or with hardware/integration tests.

---

# 23. CMake and Build Rules

The project uses CMake as the build system.

Prefer target-based CMake:

```cmake
target_link_libraries(my_target
    PRIVATE
        platform_common
)
```

Avoid global compiler/include/linker settings where a target-specific setting is sufficient.

Use CMake targets to express architecture and dependency boundaries.

Examples of intended targets include:

```text
platform_common
platform_os
platform_middleware
platform_stm32f103
application
platform_common_unit_tests
```

The project uses CMake Presets for repeatable build configurations.

---

# 24. Toolchain and Host Tools

Developer/build tools belong under:

```text
tools/
```

Examples:

```text
tools/cmake/toolchains/
tools/flash/
tools/debug/
tools/scripts/
tools/host/
```

The following distinction should remain clear:

```text
Host-side tool
    -> tools/

Target runtime library
    -> platform/
```

For example, a firmware packaging command-line application belongs under `tools/host`, while the firmware image parser/validator used by the target belongs in the platform runtime layer.

---

# 25. External Dependencies

Third-party libraries belong under:

```text
external/
```

Examples include:

```text
GoogleTest
FreeRTOS
```

Dependencies should be version-pinned and documented.

Production code should not silently download an uncontrolled dependency during a normal build.

---

# 26. Comments and Documentation

Comments should explain **why**, not merely restate **what** the code does.

Avoid:

```cpp
// Increment i
++i;
```

Prefer comments that explain an architectural or hardware reason:

```cpp
// DMA requires the buffer to remain valid until the transfer-complete
// interrupt is received.
```

Document:

- Hardware assumptions.
- Timing assumptions.
- Ownership.
- Concurrency requirements.
- Units.
- Protocol formats.
- Non-obvious workarounds.
- Hardware errata.

Use explicit units in identifiers:

```cpp
timeoutMs
frequencyHz
sizeBytes
voltageMv
```

---

# 27. Magic Numbers

Avoid unexplained numeric literals.

Prefer:

```cpp
constexpr platform::UInt32 HeartbeatPeriodMs = 1000U;
```

over:

```cpp
startTimer(1000U);
```

Protocol constants should preferably be named according to their meaning:

```cpp
constexpr platform::UInt32 HeartbeatMessageId = 0x100U;
```

---

# 28. Assertions and Defensive Programming

Assertions may be used to detect programmer errors and impossible states during development.

Assertions must not replace runtime validation of external or untrusted data.

For example, protocol length validation must still occur in production builds even if an assertion exists during development.

---

# 29. Static Analysis and Formatting

The Docker development environment is intended to provide:

```text
clang-format
clang-tidy
cppcheck
```

These tools should eventually be integrated into CMake/CI with project-specific configuration files.

The project should maintain:

```text
.clang-format
.clang-tidy
```

once the initial formatting and analysis rules are finalized.

Static analysis warnings should generally be treated as defects unless there is a documented and reviewed deviation.

---

# 30. Git and Code Review

Changes should be small enough to review and should preserve the architectural dependency direction.

Before submitting a change, developers should run at minimum:

```bash
cmake --preset host-debug
cmake --build --preset host-debug
ctest --test-dir build/host-debug --output-on-failure
```

As the CI pipeline grows, it should additionally run formatting checks, static analysis, and target builds.

Do not commit generated build output.

---

# 31. Rules We Have Explicitly Adopted So Far

The following are the project decisions established during the initial architecture work:

| Area | Current project rule |
|---|---|
| C | C11 |
| C++ | C++17 |
| Build | CMake |
| Build generator | Ninja in container presets |
| Build environment | Linux Docker/Dev Container |
| Target compiler | `arm-none-eabi-gcc` for ARM target builds |
| Host compiler | GCC/G++ in Linux container |
| Integer types | `<cstdint>`-based fixed-width project aliases |
| Error handling | `ErrorCode` / `Result<T>` for explicit errors |
| Exceptions | Not planned for the core embedded target |
| RTTI | Not planned for the core embedded target |
| Interfaces | `I...` naming convention |
| Polymorphism | Virtual interfaces with `override` and virtual destructors |
| Header protection | `#pragma once` |
| Naming | PascalCase types, camelCase functions/variables, `_` for private members |
| Namespace | `platform` for platform-level types/components |
| Hardware abstraction | Application must not directly depend on STM32 HAL |
| MCU-specific code | `platform/ports/...` |
| Board-specific code | `platform/bsp/...` |
| Tests | GoogleTest + CTest |
| Unit tests | `tests/unit/` |
| Integration tests | `tests/integration/` |
| Mocks | `tests/mocks/` |
| Third-party libraries | `external/` |
| Host tooling | `tools/host/` |
| Toolchains | `tools/cmake/toolchains/` |
| Flash/debug scripts | `tools/flash/`, `tools/debug/` |
| Reproducibility | Docker + CMake Presets |

---

# 32. Rules Still To Be Defined

The following should be finalized as the corresponding platform layers are implemented:

- ISR-safe API rules.
- RTOS task and synchronization rules.
- Locking and lock-order rules.
- Memory-allocation policy.
- DMA buffer ownership and cache rules where applicable.
- Register-access conventions.
- Volatile/atomic usage rules.
- Bit-field policy.
- Packed-structure policy.
- Compiler warning policy.
- MISRA/CERT adoption level and deviations, if required.
- Logging severity and formatting.
- Traceability requirements for safety/regulatory builds.
- Firmware-update and bootloader security rules.
- Cryptography API and key-management rules.

These should be added only when the architecture reaches those areas so the rules describe real project behavior instead of speculative restrictions.

---

# 33. Guiding Principle

The coding standard should make the system:

```text
Readable
   ↓
Predictable
   ↓
Testable
   ↓
Portable
   ↓
Deterministic
   ↓
Maintainable
```

The platform should prefer explicit design over hidden behavior, clear ownership over implicit ownership, and compile-time architectural boundaries over conventions that exist only in documentation.
