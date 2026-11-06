# MAX30102 Integration --- STM32F103 `embedded-platform`

**Status:** The driver, board integration, UART streaming, host tests, and
initial hardware raw-value validation are complete. A replacement sensor is
producing raw RED/IR values on the serial console. No commit has been made;
the integration remains uncommitted pending the user's commit request.

## 1. Objective

Integrate a MAX30102 pulse-oximeter sensor into the existing portable
embedded C++ platform, starting with **raw RED/IR sample acquisition
only**.

The implementation should:

-   Reuse the existing `II2c` interface and `Result<T>` / `ErrorCode`
    conventions.
-   Support cooperative, bare-metal polling.
-   Keep MAX30102 register handling independent of STM32 HAL headers.
-   Use the existing CMake and GoogleTest setup.
-   Avoid a generic sensor framework, RTOS dependency, and premature
    heart-rate/SpO₂ algorithms.

**Important constraint:** the current `II2c` interface is synchronous. A
cooperative `poll()` method can bound the number of transactions per
call, but each individual I²C transaction may still block until it
completes or times out. True asynchronous I²C would require an
asynchronous interface and STM32-port implementation.

## 2. Architecture

``` text
Application
    |
    v
Max30102 driver
    |  register access, configuration, FIFO decoding
    v
Existing II2c interface
    |
    v
Stm32I2c -> STM32 HAL -> STM32F103 hardware
```

The driver should depend only on platform abstractions and common types.
It must not include STM32 HAL or vendor headers.

## 3. Implemented files

``` text
platform/
└── drivers/
    └── max30102/
        ├── CMakeLists.txt
        ├── include/platform/drivers/max30102/Max30102.hpp
        └── src/Max30102.cpp

tests/
├── mocks/
│   └── MockI2c.hpp
└── unit/
    └── drivers/
        └── max30102/
            ├── CMakeLists.txt
            └── Max30102Test.cpp

platform/ports/stm32/stm32f103/
├── include/platform/stm32f103/Stm32I2c.hpp
└── src/Stm32I2c.cpp
```

The driver remains independent of STM32 HAL. The STM32 port provides the
`II2c` implementation, and the board configures I²C1 on PB6 (SCL) and PB7
(SDA). `MockI2c` is shared under `tests/mocks/` rather than duplicated in the
driver-specific tests.

## 4. Implemented driver API

The implementation follows the API shape below, with project-specific
namespaces, configuration, status counters, and fixed-capacity buffering.
`begin()` starts initialization; `poll()` advances initialization or services
the FIFO without an RTOS dependency.

``` cpp
struct Max30102Sample
{
    std::uint32_t red;
    std::uint32_t infrared;
};

class Max30102
{
public:
    explicit Max30102(platform::hal::II2c& i2c);

    // Starts initialization without a long wait/retry loop.
    platform::Result<void> begin();

    // Advances initialization or services a bounded number of FIFO samples.
    platform::Result<void> poll();

    bool isReady() const;

    bool hasSample() const;
    platform::Result<Max30102Sample> readSample();

private:
    // Keep an II2c reference, initialization state, fixed-capacity
    // sample buffer and private register/FIFO helpers.
};
```

The implemented API follows the project error-code conventions and preserves
underlying I²C errors. Initialization advances through `poll()`; transaction
errors leave the current initialization step available for retry. Invalid
part ID remains terminal.

### Sample format

Each channel is an unsigned 18-bit ADC result in the range `0`--`262143`
(`0x3FFFF`). Do not calculate heart rate or SpO₂ in the driver.

## 5. Hardware and register facts

Consult the official [MAX30102
datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max30102.pdf)
as the source of truth.

-   Standard 7-bit I²C address: `0x57`.
-   Part ID register: `0xFF`; expected part ID: `0x15`.
-   FIFO capacity: 32 samples.
-   In RED + IR mode, each sample occupies six bytes: three RED bytes
    and three IR bytes.
-   FIFO data register: `0x07`. Repeated reads from this register
    consume FIFO data; do not treat it as an ordinary incrementing
    register range.
-   Relevant FIFO registers:
    -   Write pointer: `0x04`
    -   Overflow counter: `0x05`
    -   Read pointer: `0x06`
    -   FIFO data: `0x07`
-   Other relevant registers:
    -   FIFO configuration: `0x08`
    -   Mode configuration: `0x09`
    -   SpO₂ configuration: `0x0A`
    -   RED LED current: `0x0C`
    -   IR LED current: `0x0D`

Pass `0x57` to the platform I²C API unless its documented contract says
otherwise. Make sure the STM32 HAL address representation is converted
in the port, not in the sensor driver.

### Breakout-board caution

The bare IC has separate core and LED supply requirements. Breakout
boards may include regulators and level shifting, but this varies. Check
the exact board's schematic and voltage compatibility before connecting
it to the STM32F103.

## 6. Initialization state machine

`begin()` should start initialization rather than perform a long
sequence of retries or delays. Each `poll()` call advances a small
number of steps.

Recommended sequence:

1.  Read and validate part ID.
2.  Request a device reset.
3.  On later polls, check whether reset has completed.
4.  Configure FIFO behavior.
5.  Configure RED + IR mode.
6.  Configure ADC range, sample rate and pulse width.
7.  Set RED and IR LED current.
8.  Clear FIFO write pointer, overflow counter and read pointer.
9.  Enter ready state.

For initial hardware testing, 100 samples/second and a 411 µs pulse
width are reasonable starting points. Keep LED current configurable; the
appropriate value depends on the module, optical setup and power budget.

Use the exact bit fields and reset behavior from the datasheet. Avoid
arbitrary busy-wait loops.

## 7. FIFO service design

On each ready-state `poll()`:

1.  Read FIFO write pointer, read pointer and overflow counter.
2.  Determine the number of pending samples using correct modulo-32
    pointer arithmetic and the device's FIFO semantics.
3.  Read a bounded number of complete samples.
4.  Decode RED and IR channel values.
5.  Place samples in a fixed-capacity software queue.
6.  Track overflow and software-queue drops explicitly.

A starting policy is to drain at most four samples per call and service
the driver approximately every 5--10 ms when sampling at 100
samples/second. Validate this on hardware; it is not a guaranteed
worst-case execution time.

FIFO reads should use the register-address write followed by a read of a
multiple of six bytes, provided the existing `writeRead()` method
supports the required repeated-start behavior. Confirm this against the
actual `II2c` contract.

Do not assume pointer difference alone can always distinguish empty from
full. Track the hardware overflow counter and follow the datasheet's
FIFO semantics. Reset the FIFO pointers when starting acquisition.

## 8. Private helpers and error handling

Likely private helpers:

-   `readRegister()`
-   `writeRegister()`
-   `readRegisters()`
-   `readFifoSamples()`
-   `decodeSample()`

Use existing `platform::Result` and `platform::ErrorCode` types.
Preserve underlying I²C errors where possible. Use existing codes such
as `InvalidArgument`, `Busy`, `NotReady`, or communication-related
errors according to the repository's conventions.

No dynamic allocation is needed. Use a fixed-capacity sample buffer and
make the queue-full policy explicit.

## 9. GoogleTest plan

Use the existing GoogleTest setup and a mock implementing the **actual**
`II2c` interface. The mock should record requested transactions and
return scripted responses/errors.

  Test                       Expected behavior
  -------------------------- ------------------------------------------------
  Correct part ID            Initialization progresses
  Incorrect part ID          Initialization fails with an appropriate error
  I²C read/write failure     Original error is propagated
  Reset still in progress    Driver remains initializing
  Successful configuration   Driver becomes ready
  Empty FIFO                 No samples are queued
  One FIFO sample            Correct RED/IR values
  Multiple samples           Correct ordering and decoding
  18-bit maximum             `0x3FFFF` decoded correctly
  FIFO overflow              Loss detected and recovery policy followed
  Software queue full        No memory corruption; drop is recorded
  Reinitialization           Driver and FIFO state reset consistently

Decoder test vector:

-   RED bytes `0x01, 0x23, 0x45` → `0x12345`
-   IR bytes `0x02, 0x34, 0x56` → `0x23456`

Test that the top six bits of the third byte are masked so results
remain 18-bit.

## 10. CMake integration

The implementation is registered as the `platform_max30102` library and
linked to `platform_hal`. It is included by the root build and by the
application and focused host-test targets. STM32 builds also apply the
Cortex-M3 Thumb target configuration to the driver.

The target follows this library structure:

``` cmake
add_library(platform_max30102 STATIC
    src/Max30102.cpp
)

target_include_directories(platform_max30102
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(platform_max30102
    PUBLIC
        platform_hal
)
```

Adapt the library name and dependency targets to the repository. Add the
driver directory to the appropriate parent CMake file and add a host
unit-test executable linked to the driver and GoogleTest.

Do not link the driver directly against STM32 HAL or `stm32cube-f1`.

At the application composition root, construct the driver using the
board's existing I²C implementation. Make sure the firmware target links
the driver library. Avoid linking CLI, storage or other middleware
solely because the new driver exists.

## 11. Hardware verification checklist

-   [ ] Confirm breakout voltage requirements and logic-level compatibility.
-   [ ] Connect SDA, SCL, GND and the module's specified supply.
-   [x] Configure STM32 I²C1 at 100 kHz on PB6 (SCL) and PB7 (SDA).
-   [ ] Verify ACK at address `0x57`.
-   [ ] Read part ID `0x15`.
-   [ ] Verify RED/IR values change when a finger is placed over the sensor.
-   [ ] Run sustained acquisition and check for FIFO overflow.
-   [ ] Check I²C waveforms with a logic analyzer.
-   [ ] Measure poll execution time and verify loop scheduling.

### Current hardware status

The original module reported error code `4` (`Busy`); both SCL and SDA were
measured at approximately 500 mV while idle, rather than the expected pulled-up
high level, normally 3.3 V. After replacing the suspected faulty module, the
firmware booted and raw RED/IR values were received on the serial console.
Initial hardware bring-up is therefore complete.

Continue checking common ground, supply and logic voltage, PB6/PB7 wiring,
breakout pull-ups, sustained acquisition, and FIFO overflow behavior.

Transient `Busy`, `Timeout`, and communication errors are reported and retried
with a delay. Other sensor errors stop operation until `sensor on` requests a
fresh initialization. `sensor off` disables raw output; `sensor on` enables it
and retries initialization after a terminal failure.

## 12. Recommended implementation sequence

1.  [x] Inspect the current `II2c` interface and use its actual signatures.
2.  [x] Add the driver, register handling, and shared scripted I²C mock.
3.  [x] Implement register access and part-ID validation.
4.  [x] Implement initialization as a cooperative state machine.
5.  [x] Implement bounded FIFO acquisition and 18-bit sample decoding.
6.  [x] Add overflow handling and fixed-capacity buffering.
7.  [x] Add host unit tests and integrate them with CMake.
8.  [x] Integrate I²C and sensor streaming into the STM32F103 application.
9.  [x] Build the STM32F103 firmware and host tests.
10. [x] Replace/verify the sensor module and confirm raw readings in the
    serial console.
11. [ ] Only after raw acquisition is stable, consider separately tested
    heart-rate/SpO₂ algorithms.

## 13. Success criteria

The first version is complete when:

-   Host tests cover register access, error propagation, initialization
    and FIFO decoding.
-   The driver builds without STM32 HAL headers.
-   The STM32F103 target builds with the driver included.
-   The sensor is identified correctly over I²C.
-   Raw RED/IR samples are delivered in order without unexplained data
    loss.
-   FIFO overflow and software-queue overflow are observable.
-   No heart-rate/SpO₂ algorithm or RTOS dependency has been added
    prematurely.

## 14. Verification completed

-   STM32F103 firmware build succeeded; latest link used approximately 31.5 KB
    of 64 KB flash and 2.5 KB of 20 KB RAM.
-   All 17 MAX30102 host unit tests passed, including initialization, FIFO
    decoding, overflow, queue capacity, sustained acquisition, and retry after
    `Busy`.
-   Application unit tests passed.
-   Raw sensor output has been validated on hardware with the replacement
    sensor; serial output includes lines such as `RED=<value> IR=<value>`.
-   Sustained acquisition, long-term FIFO behavior, and sensor signal-quality
    validation remain future hardware checks.
