# Embedded Platform Project — Development Summary

## 1. Project Overview

This project is a reusable, portable embedded C/C++ platform designed to keep application code independent of a specific MCU, vendor HAL/SDK, RTOS, and board.

Target architecture:

```text
Application
    ↓
Platform abstractions
    ↓
Middleware / OS abstraction
    ↓
MCU-specific ports
    ↓
Vendor HAL / SDK
    ↓
Hardware
```

Current primary target: STM32F103.

The project uses C++17/C11, CMake + Ninja, Docker/VS Code Dev Containers, GoogleTest + CTest, ARM cross-compilation, OpenOCD, ST-LINK, Cortex-Debug, and GDB multiarch.

The current embedded runtime also includes a startup UART diagnostic log and a layered hard-fault handler that captures register state and prints it over the console UART in the event of a fault.

The application initializes SPI1 and a W25Q128 external NOR flash, then exposes flash info, read, write, and explicit sector erase commands over the console CLI.

---

## 2. Repository Structure

```text
embedded-platform/
├── app/
├── platform/
│   ├── common/
│   ├── hal/
│   ├── os/
│   ├── middleware/
│   ├── ports/
│   └── bsp/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── mocks/
├── external/
│   ├── googletest/
│   └── stm32cube-f1/
├── tools/
│   ├── cmake/
│   ├── flash/
│   ├── debug/
│   └── scripts/
├── docs/
├── CMakeLists.txt
├── CMakePresets.json
├── Dockerfile
├── .dockerignore
├── .gitignore
├── LICENSE
├── README.md
├── summary.md
└── .gitmodules
```

The BSP include path was intentionally preserved as:

```text
platform/bsp/stm32f103_board/include/platform/bsp/stm32f103_board/Board.hpp
```

Do not restructure that path unless the BSP architecture is deliberately revisited.

---

## 3. Development Environment

Development flow:

```text
Windows
   ↓
VS Code
   ↓
Dev Container
   ↓
Ubuntu 24.04
```

Container tooling includes approximately:

```text
CMake
Ninja
GCC/G++
arm-none-eabi-gcc
arm-none-eabi-objcopy
arm-none-eabi-size
gdb-multiarch
OpenOCD
Clang
clang-format
clang-tidy
cppcheck
gcovr
Python
```

The ST-LINK USB path is:

```text
Windows
   ↓
WSL2 / USB path
   ↓
Docker
   ↓
Dev Container
   ↓
OpenOCD
   ↓
ST-LINK
   ↓
STM32F103
```

`--privileged` was required for the development container so the USB device node was usable by libusb/OpenOCD. `lsusb` visibility alone was not sufficient.

---

## 4. Build System

CMake is the primary build system.

Language standards:

```text
C11
C++17
```

Important build options:

```text
BUILD_TESTING
BUILD_STM32F103
BUILD_STM32F103_INTEGRATION_TESTS
```

Important presets:

```text
host-debug
stm32f103-debug
stm32f103-gpio-test
```

Typical commands:

```bash
cmake --preset host-debug
cmake --build --preset host-debug

cmake --preset stm32f103-gpio-test
cmake --build --preset stm32f103-gpio-test
```

STM32-specific compiler/linker configuration was later centralized into:

```text
tools/cmake/modules/Stm32F103.cmake
```

This module owns Cortex-M3 compiler flags, embedded C++ restrictions, linker configuration, startup/linker paths, MAP generation, and HEX/BIN generation.

The generic ARM toolchain remains separate in:

```text
tools/cmake/toolchains/arm-none-eabi-gcc.cmake
```

---

## 5. Coding Rules

Established production rules:

- C++17 for C++ code.
- C11 for C code.
- No exceptions on embedded targets.
- No RTTI on embedded targets.
- Fixed-width integer types are preferred.
- Explicit error handling is preferred.
- Vendor headers must not leak into application code.
- OS-specific types must not leak through HAL/OS abstraction interfaces.
- Low-level callbacks use function pointer + `void*` context.
- ISR callbacks must be short, non-blocking, and allocation-free.
- Long-running work should be deferred to tasks/queues when required.

---

## 6. Platform Common

`platform/common` is target-independent and must remain independent of:

```text
STM32
STM32 HAL
FreeRTOS
vendor SDKs
middleware implementations
```

Implemented foundations include:

### Types

```text
Byte
UInt8
UInt16
UInt32
UInt64
Int8
Int16
Int32
Int64
Size
Address
```

### ErrorCode

```text
Ok
InvalidArgument
InvalidState
Timeout
Busy
NotReady
NotFound
BufferFull
BufferEmpty
CommunicationError
StorageError
HardwareFault
Unsupported
InternalError
```

### Result

`Result<T>` and `Result<void>` provide explicit success/error handling without exceptions.

### NonCopyable

Reusable non-copyable base for resource-owning objects.

### ArgumentParser

Allocation-free whitespace tokenization and checked unsigned decimal/hex parsing for shared platform command-line handling.

---

## 7. HAL Interfaces

Current project-owned hardware abstraction interfaces:

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

The principle is:

> The interface describes the hardware capability, not the vendor implementation.

Dependency direction:

```text
Application
    ↓
HAL interface
    ↓
MCU adapter
    ↓
Vendor HAL
```

---

## 8. GPIO

`IGpio` supports:

```text
Input / Output
Low / High
No pull / Pull-up / Pull-down
No interrupt / Rising / Falling / Both
Interrupt callback registration
Interrupt enable / disable
```

`read()` returns:

```cpp
Result<GpioState>
```

The STM32 implementation is:

```text
Stm32Gpio
```

and it uses STM32 HAL internally.

---

## 9. STM32F103 Port and BSP

STM32CubeF1 is maintained under:

```text
external/stm32cube-f1/
```

The selected GCC startup file is:

```text
external/stm32cube-f1/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s
```

The project-owned MCU port is:

```text
platform/ports/stm32/stm32f103/
```

The BSP is:

```text
platform/bsp/stm32f103_board/
```

Current board assumptions include:

```text
PC13 status LED
8 MHz HSE
72 MHz system clock
USART1 for console/UART tests
PA9  = USART1 TX
PA10 = USART1 RX
SPI1 = PA5 SCK, PA6 MISO, PA7 MOSI
PB0  = W25Q128 chip select (software-controlled GPIO)
```

Board-specific constants should eventually move into a dedicated board configuration layer.

---

## 10. STM32 HAL Configuration

Project-specific configuration:

```text
platform/ports/stm32/stm32f103/include/stm32f1xx_hal_conf.h
```

Important values include:

```text
HSE_VALUE = 8000000U
HSI_VALUE = 8000000U
LSI_VALUE = 40000U
LSE_VALUE = 32768U
VDD_VALUE = 3300
TICK_INT_PRIORITY = 0
USE_RTOS = 0
PREFETCH_ENABLE = 1
```

UART support required both:

```text
HAL_UART_MODULE_ENABLED
HAL_DMA_MODULE_ENABLED
```

and the corresponding HAL implementation sources:

```text
stm32f1xx_hal_uart.c
stm32f1xx_hal_dma.c
```

because the STM32 UART HAL data structures contain DMA handle members.

---

## 11. Firmware Artifacts and Memory

STM32 builds produce:

```text
.elf
.hex
.bin
.map
```

The ELF is the main debugger/linker artifact.

The original GPIO firmware baseline was approximately:

```text
FLASH: 14908 B / 64 KB
RAM:    2144 B / 20 KB
```

Later UART-enabled links were approximately:

```text
embedded_firmware
    FLASH: 18664 B
    RAM:    2224 B
```

and:

```text
GPIO integration test
    FLASH: 19900 B
    RAM:    2232 B
```

These values are reference points for future memory-growth analysis.

---

## 12. Host Tests

GoogleTest was added under:

```text
external/googletest/
```

CTest is the host test runner.

Typical command:

```bash
ctest --test-dir build/host-debug --output-on-failure
```

Host tests cover common/platform functionality, application logic, HAL-facing behavior, mocks, and later middleware tests where applicable.

Hardware integration firmware remains separate from normal host CTest execution.

---

## 13. GPIO Hardware Integration Milestone

First complete hardware vertical slice:

```text
Application/Test
      ↓
IGpio
      ↓
Stm32Gpio
      ↓
STM32 HAL
      ↓
STM32F103 GPIO
```

Hardware test:

```text
PC13 = output
PA0  = input + EXTI0 rising edge
PC13 ───── PA0
```

The test validates configuration, output read/write, input configuration, interrupt callback registration, interrupt enablement, and real interrupt execution.

Final status:

```text
0xAA55 = Passed
```

---

## 14. GPIO Interrupt Debugging Lessons

The initial EXTI test fell into the default interrupt handler.

Root cause:

```text
weak startup EXTI handler
        ↓
interrupt source inside static BSP library
        ↓
linker did not extract object as expected
```

The working solution was:

- include `stm32f1xx_it.cpp` directly in the integration-test executable;
- retain `EXTI0_IRQHandler` with:

```cmake
-Wl,-u,EXTI0_IRQHandler
```

- verify the final ELF symbol with `arm-none-eabi-nm`;
- verify the vector entry using GDB.

The final symbol was a strong `T EXTI0_IRQHandler`.

This established an important rule for future interrupt/vector integration: verify the final ELF, not only the object file or static library.

---

## 15. SysTick Debugging Lesson

During UART debugging the debugger appeared to show:

```text
WWDG_IRQHandler
```

while the test was in `smallDelay()`.

The actual exception was SysTick; the weak/default interrupt handler address caused the debugger symbol display to be misleading.

The BSP was corrected to provide:

```cpp
extern "C" void SysTick_Handler(void)
{
    HAL_IncTick();
}
```

This restored the normal HAL SysTick time base and prevented the delay loop from falling into the default handler.

This was another important vector-table/debugging lesson.

---

## 16. Debug and Flash Workflow

Cortex-Debug extension:

```text
marus25.cortex-debug
```

Debugger:

```text
gdb-multiarch
```

OpenOCD is launched manually and Cortex-Debug attaches to the external GDB server.

Ports:

```text
GDB    50000
TCL    50001
Telnet 50002
```

Typical OpenOCD command:

```bash
openocd \
  -c "gdb_port 50000" \
  -c "tcl_port 50001" \
  -c "telnet_port 50002" \
  -f tools/debug/openocd/stm32f103.cfg \
  -c "init"
```

The debugger loads the ELF and allows stepping through the real hardware integration path.

---

## 17. Synchronous UART Stage

The first UART implementation followed the existing project-owned `IUart` abstraction.

UART configuration covers:

```text
baud rate
word length
parity
stop bits
flow control
```

Operations are synchronous/blocking:

```text
configure
transmit
receive
```

STM32 implementation:

```text
Stm32Uart
```

The implementation keeps `UART_HandleTypeDef` and STM32 HAL calls inside the MCU port.

USART1 was selected for the first hardware test:

```text
PA9  = TX
PA10 = RX
```

Hardware loopback:

```text
PA9  USART1_TX ───── PA10 USART1_RX
```

Configuration used for the test:

```text
115200
8 data bits
no parity
1 stop bit
no flow control
```

The synchronous UART hardware verification completed successfully.

---

## 18. Asynchronous UART Stage

`IAsyncUart` is kept separate from `IUart` so the basic UART abstraction does not become unnecessarily large.

The asynchronous implementation uses the STM32 HAL interrupt APIs.

Conceptual path:

```text
IAsyncUart
    ↓
Stm32Uart
    ↓
HAL_UART_Transmit_IT()
HAL_UART_Receive_IT()
    ↓
USART1 interrupt
    ↓
USART1_IRQHandler()
    ↓
HAL_UART_IRQHandler()
    ↓
HAL UART completion callbacks
    ↓
project callback
```

The BSP provides the relevant interrupt path, including:

```text
SysTick_Handler
USART1_IRQHandler
EXTI0_IRQHandler
```

Callbacks follow the existing ISR rules: short, non-blocking, and allocation-free.

---

## 19. Asynchronous UART Verification Milestone

The asynchronous UART Stage 2 hardware verification was completed successfully.

Verified sequence:

```text
receiveAsync()
        ↓
transmitAsync()
        ↓
USART1_IRQHandler()
        ↓
HAL_UART_IRQHandler()
        ↓
TX/RX completion callbacks
        ↓
data comparison
        ↓
0xAA55
```

This established the second major communication capability in the platform:

```text
UART synchronous       ✅
UART asynchronous      ✅
```

---

## 20. Fixed-Size Buffering Foundation

A fixed-size byte ring-buffer foundation was introduced to support future continuous asynchronous communication.

The intended properties are:

```text
fixed-size
heap-free
deterministic
portable
host-testable
```

The buffering concept covers operations such as:

```text
push
pop
write
read
empty
full
capacity
free space
clear
wrap-around
```

An SPSC-oriented design was also introduced as groundwork for ISR/application communication, with one producer and one consumer per buffer.

The buffer should not be treated as a general multi-producer/multi-consumer synchronization primitive.

---

## 21. Current Completed Milestones

```text
Repository / architecture setup                          ✅
Docker development environment                            ✅
CMake + Ninja                                             ✅
Host GoogleTest + CTest                                  ✅
Common layer                                             ✅
HAL interfaces                                           ✅
STM32F103 port                                           ✅
STM32F103 BSP                                            ✅
STM32 build/debug infrastructure                          ✅
GPIO synchronous implementation                           ✅
GPIO interrupt integration test                           ✅
Synchronous UART implementation                           ✅
Synchronous UART hardware test                            ✅
Asynchronous UART implementation                          ✅
Asynchronous UART hardware test                           ✅
Fixed-size buffering foundation                          ✅
Buffered async UART middleware                          ✅
RX/TX overflow, retry, and cancellation behavior         ✅
Continuous stream semantics for buffered UART             ✅
Boot UART diagnostics                                     ✅
Hard-fault register capture and UART dump                ✅
STM32F103 SPI1/SPI2 adapter and HAL integration            ✅
W25Q128 IFlashStorage driver and host tests                 ✅
Common allocation-free CLI argument parser                 ✅
Flash CLI read/write/explicit sector erase                  ✅
W25Q128 hardware JEDEC ID and readback verification         ✅
STM32 middleware targets configured for Cortex-M3 Thumb    ✅
Host validation: 74 CTest cases passing                     ✅
```

### SPI and W25Q128 Hardware Milestone

The STM32F103 board currently uses SPI1 with PA5/SCK, PA6/MISO, PA7/MOSI, and PB0 as GPIO-controlled chip select. Firmware configures Mode 0, MSB-first, 8-bit frames at 1 MHz and validates JEDEC ID `EF 40 18` during startup.

The W25Q128 driver implements `IFlashStorage`: 16 MiB capacity, 256-byte page programming, 4 KiB sector erase, address-range validation, and bounded busy polling. Host tests cover JEDEC validation, reads, page-boundary writes, erase alignment, and busy timeout behavior.

The CLI supports `flash info`, `flash read <hex-address> <length 1-16>`, `flash erase <aligned-sector-address>`, and `flash write <hex-address> <hex-bytes>`. Writes do not erase implicitly. Erase affects exactly one 4 KiB sector, so preserve neighboring contents before issuing it. Hardware verification completed with erase at `0x1000`, program at `0x1020`, and readback of `DE AD BE EF`.

The portable `platform::ArgumentParser` performs allocation-free tokenization and checked unsigned decimal/hex parsing. The CLI registry retains fixed-command behavior while supporting argument-bearing commands.

The first firmware build exposed an important cross-build rule: every compiled STM32 library, not only the final executable, must receive `-mcpu=cortex-m3 -mthumb`. The W25Q128 object initially used ARM-state instructions and faulted at startup; applying the STM32 target configuration corrected the object attributes and reduced linked firmware size from about 58.8 KiB to 47.0 KiB.

---

## 22. Current Architecture

```text
                         Application
                              │
                              ▼
                    Platform abstractions
                       ┌──────┴──────┐
                       │             │
                      GPIO          UART
                       │         ┌───┴───┐
                       │         │       │
                       │      IUart  IAsyncUart
                       │         │       │
                       └────┬────┴───┬───┘
                            │        │
                            ▼        ▼
                       STM32 project adapters
                              │
                              ▼
                         STM32 HAL
                              │
                              ▼
                          STM32F103
```

Board-specific responsibilities remain below the generic HAL contracts:

```text
BSP
 ↓
clock / pin / peripheral routing / IRQ support
```

The flash command path follows the same dependency direction:

```text
Application CLI / FlashCommands
    ↓
IFlashStorage
    ↓
W25Q128 (platform/middleware/storage)
    ↓
ISpi + IGpio chip select
    ↓
Stm32Spi + Stm32Gpio
    ↓
STM32 HAL / SPI1 / W25Q128
```

---

## 23. Known Remaining Cleanup

### Board configuration

Move hard-coded board properties into dedicated configuration, while preserving the current BSP ownership boundary:

```text
PC13 status LED
8 MHz HSE
72 MHz clock
USART1 routing
SPI1 routing (PA5/PA6/PA7)
W25Q128 chip select (PB0)
```

The board continues to own hard-coded pin and clock facts, which is acceptable for the current milestone but should be separated from reusable MCU-port logic.

Suggested split:

```text
platform ports: MCU-specific implementation details
BSP: board-specific pin/routing/clock assumptions
user config: product-specific board policy and runtime settings
```

### Interrupt/vector ownership

The BSP owns vector implementations in the dedicated `bsp_stm32f103_board_vectors` library. The STM32 CMake module retains this library in firmware and integration images.

Current state:

```text
BSP vector library owns IRQ handler entry points
port drivers dispatch peripheral IRQs into the STM32 HAL
the STM32 CMake module includes the vector library in final images
```

Continue keeping handler ownership explicit as additional peripherals add IRQs.

### Syscalls

The target currently uses:

```text
nano.specs
nosys.specs
```

and has had expected newlib syscall warnings such as:

```text
_write
_read
_close
_fstat
_lseek
_isatty
_getpid
_kill
```

A future decision is needed between project-specific stubs, UART-backed logging, or prohibiting stdio in firmware.

### Build metadata

Continue improving portability so MCU-specific build configuration remains centralized without mixing it with board-specific configuration.

### Fault handler ownership

The fault-handling path is now intentionally layered so that register capture is MCU-specific, while the final vector override and UART output remain at the app boundary.

Suggested ownership split:

```text
platform/common: portable fault record structure
platform/port/stm32f103: fault register capture from stack + SCB
app: strong HardFault_Handler override and UART dump output
```

This preserves the platform/port/application dependency hierarchy while still allowing a strong vector override in the final firmware image.

---

# 24. Future Development Plan

## Phase 1 — UART communication path status

The UART communication path is now considered complete for the current platform architecture and validated in host tests.

Completed work includes:

```text
synchronous UART
asynchronous UART
buffered async communication
retry logic
overflow detection
cancellation semantics
error reporting and recovery
stream processing across repeated poll cycles
```

## Phase 2 — Next peripheral vertical slice

The SPI peripheral and W25Q128 external-flash vertical slice are implemented and hardware-verified. I2C is a natural next peripheral candidate and should follow the established pattern:

```text
HAL contract
    ↓
host tests / mocks
    ↓
MCU adapter
    ↓
BSP configuration
    ↓
hardware integration test
```

## Phase 3 — DMA follow-on

DMA remains a meaningful follow-on optimization after the interrupt-driven path is stable, but it is no longer the immediate next milestone.

```text
UART interrupt mode
        ↓
UART DMA mode
```

Keep DMA-specific details out of generic abstractions unless they represent a real platform-wide capability.

## Phase 4 — Additional peripherals

Proceed with:

```text
I2C
Timer
Watchdog
ADC
PWM
CAN
Clock
```

Additional external flash devices and MCU-internal flash storage remain future work.

Each should follow the established pattern:

```text
HAL contract
    ↓
host tests / mocks
    ↓
MCU adapter
    ↓
BSP configuration
    ↓
hardware integration test
```

## Phase 5 — OS abstraction implementations

Implement:

```text
Bare-metal scheduler
FreeRTOS adapter
```

behind:

```text
ITask
IMutex
ISemaphore
IQueue
IEvent
```

Potential scheduler work:

```text
periodic tasks
priority
1 ms tick
missed deadlines
WCET awareness
ISR-to-task handoff
```

## Phase 6 — Middleware

Future reusable middleware candidates:

```text
logging
diagnostics
communication/protocol
serialization
storage
software update
```

## Phase 7 — Software update

Planned architecture:

```text
SoftwareUpdater
      ↓
IFlashStorage
      ↓
W25Q128 or a future MCU-internal flash implementation
```

Reusable building blocks:

```text
CRC
hashing
image metadata
versioning
validation
update state
rollback/recovery
```

## Phase 8 — Platform evolution

Continue extending the platform with additional high-value capabilities as the architecture matures, including deeper diagnostics, telemetry, test automation, and product-specific service layers.

---

## 25. Architectural Rules to Preserve

```text
Application must not know vendor HAL types.

platform/common must remain MCU/vendor/RTOS independent.

HAL interfaces describe capabilities, not implementations.

BSP owns board-specific facts.

MCU ports own vendor integration.

Middleware depends on project abstractions.

ISR code must be short, bounded, and non-blocking.

No dynamic allocation in ISR context.

Hardware tests remain separate from host CTest tests.

New peripherals receive both host-side and hardware-level verification.

Build/debug configuration remains centralized and reusable.
```

---

## 26. Immediate Next Milestone

The SPI/W25Q128 path is complete through hardware readback. The next technical milestone is an I2C peripheral vertical slice, following the established pattern:

```text
HAL contract
    ↓
host tests / mocks
    ↓
MCU adapter
    ↓
BSP configuration
    ↓
hardware integration test
```

DMA remains a follow-on optimization for communication peripherals. The W25Q128 busy-poll loop is bounded by a configurable poll count; replacing that count with a time-based timeout and adding an appropriate inter-poll delay remains a storage-driver cleanup item.
