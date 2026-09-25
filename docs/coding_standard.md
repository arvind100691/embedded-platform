# Embedded Platform Coding Standard

**Status:** Initial project standard  
**Applies to:** Embedded C/C++, platform code, middleware, BSPs, MCU ports, tests, and host-side utilities where practical.

This document combines generally accepted embedded-software practices with the conventions explicitly selected for this repository. It is a project standard, not a claim of compliance with MISRA, CERT, a medical-device standard, or another external standard.

## 1. General Embedded Principles

Code should prioritize:

- Deterministic behavior.
- Predictable resource usage.
- Explicit ownership and lifetime.
- Clear layer boundaries.
- Small, testable components.
- Portability across boards and MCUs.
- Compile-time checking where practical.
- Diagnosable errors.
- Reproducible builds.

Relevant external guidance may include MISRA C/C++, CERT C/C++, C++ Core Guidelines, vendor coding guidance, and product-specific safety/security processes. Adoption of any external standard must be explicit and include project-specific deviations where required.

## 2. Language Standards

C uses C11:

```cmake
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
```

C++ uses C++17:

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

Do not use C++20/C++23 language features until the project standard is deliberately changed.

## 3. Types

Use fixed-width integer types for protocol, hardware, memory-map, and binary-format data.

The platform provides aliases such as:

```cpp
platform::Byte
platform::UInt8
platform::UInt16
platform::UInt32
platform::UInt64
platform::Int32
platform::Size
platform::Address
```

Prefer `std::uint32_t`/the platform aliases over implementation-dependent types such as `unsigned long` when exact width matters.

Use `enum class` for scoped enumerations:

```cpp
enum class GpioState
{
    Low,
    High
};
```

## 4. `const` and `constexpr`

Use `const` for values that do not change and `constexpr` for compile-time constants where appropriate.

Example:

```cpp
constexpr platform::UInt32 BufferSize = 128U;
```

Avoid mutable global state unless it represents a deliberately shared hardware/runtime resource.

## 5. C++17 Usage

Use C++17 features that improve clarity and safety, but maintain embedded constraints.

Do not use C++20 designated initialization in this project because the language baseline is C++17.

Preferred:

```cpp
platform::hal::GpioConfig config{};
config.direction = platform::hal::GpioDirection::Output;
config.initialState = platform::hal::GpioState::High;
```

Instead of C++20-only designated initialization.

## 6. Exceptions and RTTI

The current target policy is:

- No exceptions in embedded target code.
- No RTTI in embedded target code.
- Use explicit result/error handling.

Normal error handling should use:

```cpp
platform::ErrorCode
platform::Result<T>
```

rather than exceptions.

Host-only tools may use richer C++ facilities when there is no impact on the target runtime architecture.

## 7. Dynamic Memory

Avoid uncontrolled dynamic allocation in target runtime code.

Prefer:

- Static storage where lifetime is global/system-level and justified.
- Stack storage for bounded short-lived objects.
- Fixed-capacity containers.
- Memory pools when dynamic lifetime is genuinely required.

Any required heap usage should have an explicit ownership, lifetime, fragmentation, and failure strategy.

## 8. Error Handling

Do not use unexplained magic return values:

```cpp
return -1;
```

Use the common error model:

```cpp
return platform::Result<void>::failure(
    platform::ErrorCode::Timeout);
```

Use `bool` only when success/failure is the only information required.

Callers should inspect returned errors and intentionally document ignored results where appropriate.

## 9. Interfaces and Dependency Injection

Hardware/OS dependencies should be represented by interfaces when abstraction and unit testing provide value.

Example:

```cpp
class StatusLed
{
public:
    explicit StatusLed(platform::hal::IGpio& gpio);

private:
    platform::hal::IGpio& gpio_;
};
```

Application code should depend on `IGpio`, not `Stm32Gpio` or STM32 HAL functions.

Use composition rather than deep inheritance hierarchies.

Polymorphic interfaces must have virtual destructors.

## 10. Header Dependency Rules

### Portable code may include

- Standard C/C++ headers.
- Platform-independent project headers appropriate to its layer.

### Portable code must not include

```text
stm32f1xx_hal.h
stm32f1xx.h
FreeRTOS.h
cmsis_os.h
```

unless the file belongs to the corresponding implementation boundary.

Vendor types such as:

```text
GPIO_TypeDef
UART_HandleTypeDef
HAL_StatusTypeDef
```

must remain inside MCU-specific code.

## 11. MCU Port vs BSP

The MCU port defines **how** a peripheral works on the MCU.

Example:

```text
Stm32Gpio
Stm32Uart
Stm32Spi
```

The BSP defines **where/how the specific board is wired and initialized**.

Example:

```text
PC13 -> Status LED
I2C1 -> Board Sensor
USART1 -> Debug UART
```

Do not hard-code product-board pin mappings into generic MCU drivers.

## 12. Naming

### Classes/structs

PascalCase:

```cpp
class SoftwareUpdater;
struct CanFrame;
```

### Functions/methods

camelCase:

```cpp
readStatus();
setDutyCycle();
```

### Variables/parameters

camelCase:

```cpp
uint32_t timeoutMs;
uint8_t dataSize;
```

### Private data members

Trailing underscore:

```cpp
GPIO_TypeDef* port_;
std::uint16_t pin_;
```

### Interfaces

Use the `I` prefix consistently for platform contracts:

```text
IGpio
IUart
ITimer
IFlashStorage
```

## 13. Formatting

Use a consistent formatter configuration across the repository.

Recommended baseline:

- 4-space indentation for C/C++.
- Braces on the same style throughout a file.
- One logical declaration per line when it improves readability.
- Keep functions short enough to understand locally.
- Avoid excessive horizontal line length.

Formatting should eventually be enforced automatically with `.clang-format` and CI.

## 14. `noexcept`

Use `noexcept` where an operation is intentionally non-throwing and the guarantee is useful to the design.

Examples include small status/query functions:

```cpp
bool isSuccess(ErrorCode error) noexcept;
```

Do not mechanically add `noexcept` everywhere; the guarantee should be truthful.

## 15. ISR and Callback Rules

Interrupt callbacks are a special execution context.

Unless explicitly documented otherwise, a hardware interrupt callback must be treated as **ISR context**.

ISR callbacks must:

- Return quickly.
- Avoid blocking.
- Avoid sleeping/delaying.
- Avoid uncontrolled dynamic allocation.
- Use ISR-safe synchronization APIs where an RTOS requires them.
- Defer complex processing to a task where practical.

Example pattern:

```text
Hardware interrupt
       |
       v
Short ISR callback
       |
       v
Queue/Event
       |
       v
OS task
       |
       v
Longer processing
```

Every callback interface should document whether callbacks execute in ISR, interrupt-deferred, or task/thread context.

## 16. Callback Lifetime

When an interface accepts:

```cpp
callback
context
```

the owner of the callback context must guarantee that it remains valid for as long as the callback can occur.

Registration and unregistration rules must be explicit.

## 17. Blocking Semantics

API contracts should make blocking behavior clear.

For example:

```cpp
transmit(data, size, timeoutMs);
```

is a synchronous operation and may block until completion or timeout.

Asynchronous behavior should use explicit capability interfaces such as:

```text
IAsyncUart
```

rather than silently changing the semantics of a basic interface.

## 18. Concurrency

Shared state must have an explicit concurrency strategy.

Possible mechanisms:

- Critical sections.
- Atomic variables.
- Mutexes.
- Semaphores.
- Queues/events.
- Ownership transfer.

Do not protect a data structure with a mutex merely because concurrent access exists; first determine whether the access can be architected as single-owner or message-based.

## 19. Time

Avoid scattering raw time assumptions throughout application logic.

Prefer platform time abstractions such as `IClock` and explicit units:

```cpp
timeoutMs
delayUs
timestampMs
```

Never use an unlabelled integer for a time quantity if the unit is not obvious.

## 20. Hardware Register Access

Direct register access is allowed only in the appropriate MCU/platform implementation layer.

Application and portable middleware should never access MCU registers directly.

When direct register access is necessary:

- Keep it local to the MCU port.
- Document why the vendor HAL is insufficient.
- Follow vendor reference-manual requirements.
- Avoid mixing register access with product logic.

## 21. Testing

Production code must not depend on GoogleTest.

Tests may depend on:

```text
GoogleTest
GoogleMock
CTest
Test doubles
```

Unit tests should be deterministic and avoid real hardware whenever practical.

Test names should describe observable behavior:

```cpp
TEST(ResultTest, FailedResultContainsError)
```

rather than implementation details.

## 22. Test Doubles

Use the lightest test double that satisfies the test:

- Fake for simple deterministic behavior.
- Stub for fixed responses.
- Mock when interaction verification is important.

Test doubles belong under `tests/` and must not become production dependencies.

## 23. CMake Rules

Use targets and target dependencies rather than global compiler/include configuration wherever practical.

Prefer:

```cmake
target_link_libraries(my_target PRIVATE platform_common)
```

and:

```cmake
target_include_directories(my_target PRIVATE ...)
```

over global include paths or global libraries.

Targets should expose only the dependencies that consumers actually need.

## 24. Docker and Toolchain Rules

The Dev Container is the reference development environment.

Target compiler configuration belongs under:

```text
tools/cmake/toolchains/
```

Host tools, flashing, debug scripts, and Linux-native utilities belong under `tools/`.

Runtime target code must not depend on host-side tools.

## 25. Third-Party Dependencies

External dependencies belong under:

```text
external/
```

Dependencies should have:

- Pinned versions/commits.
- Recorded upstream source.
- License information.
- Minimal integration surface.

Do not modify vendor code to implement product-specific behavior.

## 26. Documentation

Non-obvious architectural decisions should be documented near the affected subsystem and/or under `docs/`.

Public interfaces should document:

- Parameters.
- Return values.
- Ownership.
- Blocking behavior.
- Context (ISR/task/thread).
- Lifetime requirements.
- Error conditions.

## 27. Current Project Conventions

The conventions explicitly adopted so far are:

```text
Language       C11 / C++17
Build          CMake + Ninja
Environment    Docker Dev Container
Target         ARM GCC / STM32F103
Testing        GoogleTest + CTest
Formatting     clang-format planned/enforced
Analysis       clang-tidy / cppcheck available
Debugging      GDB Multiarch / OpenOCD / Cortex-Debug
Errors         ErrorCode + Result<T>
Interfaces     I-prefixed polymorphic contracts
Private data   trailing underscore
Exceptions     disabled for embedded target
RTTI           disabled for embedded target
```

## 28. Rules That Will Be Added Later

As the platform grows, this document should explicitly define:

- MISRA/CERT adoption level if required.
- Maximum stack usage rules.
- Heap policy and memory budget ownership.
- ISR execution-time budget.
- Task priorities and scheduling rules.
- Atomic/lock-free policy.
- DMA buffer alignment/cache rules for MCUs where applicable.
- Logging policy and allowed logging contexts.
- Firmware-update security requirements.
- Cryptographic API requirements.
- Secure-boot and key-management rules.
- HIL test conventions.
- Static-analysis thresholds.
- Code coverage thresholds.
- Release/build reproducibility requirements.
