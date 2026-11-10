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

The project currently has the following foundation and verified implementation in place:

- Docker-based development environment.
- CMake + Ninja build flow.
- CMake presets for host and STM32F103 configurations.
- C++17 / C11 project baseline.
- `platform/common` foundation with fixed-width types, error codes, `Result<T>`, `NonCopyable`, and an allocation-free `ArgumentParser`.
- Hardware abstraction interfaces under `platform/hal`.
- OS abstraction interfaces under `platform/os`.
- GoogleTest and CTest infrastructure.
- STM32CubeF1 kept as an external dependency.
- STM32F103 GCC target and linker configuration.
- `Stm32Gpio` implementation for GPIO input/output and interrupt support.
- `Stm32Uart` synchronous and asynchronous UART implementation.
- `Stm32Spi` blocking SPI1/SPI2 implementation for 8-bit frames.
- STM32F103 I²C1 implementation on PB6 (SCL) and PB7 (SDA), configured for 100 kHz.
- Portable MAX30102 driver with cooperative initialization, bounded FIFO reads, 18-bit RED/IR decoding, overflow reporting, and host unit tests.
- Application startup raw-sample stream and `sensor on` / `sensor off` console commands, with retry handling for transient I²C errors.
- `W25Q128` external NOR driver implementing `IFlashStorage`.
- Application CLI commands for W25Q128 info, read, write, and single-sector erase.
- `BufferedAsyncUart` middleware layer using fixed-capacity SPSC ring buffering.
- Buffering and async UART host tests covering startup, retry behavior, overflow, cancellation, and sustained stream semantics.
- STM32F103 board/BSP layer with clock and board-specific GPIO/UART mapping.
- Board-level console UART configuration for USART1 and startup diagnostics.
- Embedded application startup log over the board console UART.
- Layered hard-fault capture path using a platform fault record, STM32 port register capture, and app-level UART dump override.
- Firmware ELF/HEX/BIN/MAP generation.
- Hardware-verified W25Q128 JEDEC identification and erase/program/readback through the application CLI.

GPIO, UART, SPI, and I²C communication paths are implemented. The W25Q128 driver and CLI are host-tested, and hardware readback has been verified. The MAX30102 driver and STM32F103 I²C integration build successfully, are covered by host tests, and have now been hardware-verified with a replacement sensor producing raw RED/IR readings on the serial console.

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
|   +-- drivers/                 # Portable device drivers
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
|
+-- docs/
    +-- architecture/
    +-- coding_standard.md
    +-- MAX30102_Integration_Plan_STM32F103.md

Notes:
- The current checked-in repository does not include a top-level config/ directory.
- Historical documentation may mention planned host-side utilities or config folders; those remain future work rather than active source structure.
- The runtime architecture is still organized around platform/, app/, tests/, external/, and tools/ as the active tree.
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

Every source library in an STM32 firmware must receive the Cortex-M3/Thumb target options, not only the final executable. The STM32 CMake module must be loaded before configuring MCU-specific libraries.

## Testing

Testing is part of the architecture rather than an afterthought.

### Unit tests

Run primarily on the host and isolate the component under test using mocks/fakes where appropriate.

### Clang and Cmake format

Run this  ./tools/scripts/format.sh

### Integration tests

Verify interaction between multiple platform layers, middleware components, or services.

### Hardware integration / HIL

The repository includes STM32F103 firmware tests for GPIO, UART, and SPI. The SPI test reads the W25Q128 JEDEC ID without modifying flash. Application-level flash commands have also been verified on hardware using erase, program, and readback.

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

## SPI NOR Flash CLI

The current board maps SPI1 to PA5/SCK, PA6/MISO, and PA7/MOSI. W25Q128 chip select is a separate GPIO on PB0. Firmware configures the bus for 1 MHz, Mode 0, MSB-first, 8-bit transfers and checks the JEDEC ID before starting the CLI.

```text
flash info
flash read <hex-address> <length 1-16>
flash erase <aligned-sector-address>
flash write <hex-address> <hex-bytes>
```

Example:

```text
flash erase 0x00001000
flash write 0x00001020 DEADBEEF
flash read 0x00001020 4
```

Erase affects exactly one aligned 4 KiB sector. Write only sends page-program data; it does not erase implicitly. Erase the containing sector first when its existing contents must be replaced, and preserve any neighboring data in that sector.

## MAX30102 Raw Sensor Stream

The portable MAX30102 driver is in `platform/drivers/max30102/`. It uses the
existing `II2c` interface, validates part ID `0x15` at seven-bit address
`0x57`, configures RED + IR acquisition, and drains a bounded number of FIFO
samples per poll. RED and IR samples are decoded as unsigned 18-bit values.
The driver reports hardware FIFO overflow and software queue drops; heart-rate
and SpO₂ algorithms are intentionally out of scope.

On the STM32F103 board, I²C1 uses PB6 for SCL and PB7 for SDA at 100 kHz.
Firmware starts the raw stream at boot and prints samples as:

```text
RED=<value> IR=<value> HR=<bpm>
```

`HR=--` is printed until enough valid windows have been detected. The estimate
is derived from rolling-window autocorrelation, not individual peak counting,
and is a signal-processing result rather than a medical measurement. Use
`sensor off` to stop output and `sensor on` to resume or reinitialize the sensor
after a terminal error. Transient `Busy`, `Timeout`, and communication errors
are reported and retried with a delay.

To view the console from Linux, use a serial terminal such as `tio` at 115200
baud, for example `tio -b 115200 /dev/ttyUSB0`. Only one program can own the
serial port at a time.

**Hardware validation completed.** The original module reported I²C `Busy`, and
both SCL and SDA measured approximately 500 mV while idle. After replacing the
sensor module, raw RED/IR values were received on the serial console. Continue
to verify module supply, common ground, wiring, logic levels, pull-ups, and
sustained acquisition behavior as additional hardware testing proceeds. See
[the integration plan](docs/MAX30102_Integration_Plan_STM32F103.md) for
implementation and verification details.

The initial heart-rate processing foundation is in
`platform/algorithms/heartrate/`. It is independent of the sensor driver,
accepts configurable-rate IR samples, removes a slowly varying baseline with
an EMA divisor of 64, and searches autocorrelation lags within a 40–180 BPM
range over a 768-sample window. It analyzes every 100 new samples, requires
three qualified windows meeting the default 100-count RMS amplitude and
0.350 correlation thresholds, and smooths periods with a five-window median.
Occasional weak windows do not restart the qualified-window count; three
consecutive weak windows clear the reported rate. A greater-than-10% period
change resets the stability history. Thresholds and rate limits are configurable.

The estimator uses a Strategy-style `IHeartRateAlgorithm` contract. Its
default constructor keeps autocorrelation behavior unchanged; applications can
inject `PeakDetectionHeartRateAlgorithm` (or another implementation) by
reference without heap allocation. The estimator owns shared sample
preprocessing, confidence-independent interval validation, stability gating,
and median smoothing, while each strategy only finds a candidate beat interval.
Injected strategies must outlive the estimator. This keeps algorithm-specific
logic independently testable and makes additional methods straightforward to
add without duplicating the streaming and validation pipeline.

```cpp
platform::algorithms::heartrate::PeakDetectionHeartRateAlgorithm peakDetection{};
platform::algorithms::heartrate::HeartRateEstimator estimator{peakDetection};
```

Keep the strategy object alive for at least as long as its estimator.

### Benchmarking against BIDMC

The host-side benchmark runs both strategies on BIDMC PLETH at 125 Hz and
compares estimates with the dataset's 1 Hz PPG pulse rate (`PULSE`) and
ECG-derived heart rate (`HR`). It reports per-recording and aggregate
coverage, mean absolute error, signed bias, and the percentage of estimates
within 5 BPM. BIDMC is ICU data, and its monitor numerics are a reference
signal rather than a clinical gold standard.

Download the Signals and Numerics CSV files from the
[BIDMC dataset](https://physionet.org/content/bidmc/1.0.0/) into one directory.
The benchmark does not download or commit the dataset. Build and run it with:

```sh
mkdir -p build/bidmc_csv
wget -r -np -nd -A '*_Signals.csv,*_Numerics.csv' \
  -P build/bidmc_csv \
  https://physionet.org/files/bidmc/1.0.0/bidmc_csv/
cmake --preset host-debug -DBUILD_HEARTRATE_BENCHMARKS=ON
cmake --build build/host-debug --target bidmc_heart_rate_benchmark
build/host-debug/tools/benchmarks/heartrate/bidmc_heart_rate_benchmark build/bidmc_csv
```

The dataset is distributed under the terms described in its
[license](https://physionet.org/content/bidmc/1.0.0/LICENSE); follow its
attribution requirements when sharing benchmark results.

#### Initial BIDMC run

The unmodified default thresholds were evaluated on all 53 recordings. These
are pooled one-second reference comparisons; coverage is the fraction of valid
reference points with an estimate, and MAE/bias/within-5 BPM are computed only
where an estimate was present.

| Strategy | Reference | Coverage | MAE | Bias | Within 5 BPM |
|---|---|---:|---:|---:|---:|
| Autocorrelation | PPG `PULSE` | 92.7% | 7.0 BPM | -5.0 BPM | 83.5% |
| Autocorrelation | ECG `HR` | 92.7% | 6.8 BPM | -5.7 BPM | 83.5% |
| Peak detection | PPG `PULSE` | 96.3% | 3.0 BPM | -1.5 BPM | 89.6% |
| Peak detection | ECG `HR` | 96.2% | 3.0 BPM | -2.1 BPM | 88.7% |

Several recordings remain difficult: autocorrelation MAE exceeded 30 BPM on
some records, and peak detection also had subject-level outliers. This is a
baseline comparison on the same dataset used to compare strategies, not a
held-out validation set or a clinical accuracy claim. Inspect per-record
output and use separate subjects for any future tuning and final evaluation.

A 42-second USB0 capture after flashing the current 768-sample implementation
produced numeric estimates between 66 and 71 BPM, with a median of 68 BPM.
Independent autocorrelation checks on settled raw-data windows were consistent
with roughly 64–71 BPM. This is promising repeatability evidence, not
ground-truth validation. The nominal 100 Hz sample-rate assumption and sensor
readings should be checked against a trusted reference before relying on the
estimate.

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
