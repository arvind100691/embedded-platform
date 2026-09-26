#pragma once

#include <cstddef>

#include "platform/hal/IGpio.hpp"

class MockGpio final : public platform::hal::IGpio
{
public:
    platform::Result<void> configure(
        const platform::hal::GpioConfig& config) override
    {
        lastConfig_ = config;
        configureCallCount_++;

        if (!configureShouldSucceed_)
        {
            return platform::Result<void>::failure(
                configureError_);
        }

        state_ = config.initialState;

        return platform::Result<void>::success();
    }

    platform::Result<void> write(
        platform::hal::GpioState state) override
    {
        state_ = state;
        writeCallCount_++;

        return platform::Result<void>::success();
    }

    platform::Result<platform::hal::GpioState> read()
        const override
    {
        return platform::Result<
            platform::hal::GpioState
        >::success(state_);
    }

    platform::Result<void> toggle() override
    {
        toggleCallCount_++;

        state_ =
            state_ == platform::hal::GpioState::High
                ? platform::hal::GpioState::Low
                : platform::hal::GpioState::High;

        return platform::Result<void>::success();
    }

    platform::Result<void> registerInterruptCallback(
        platform::hal::GpioInterruptCallback callback,
        void* context) override
    {
        callback_ = callback;
        callbackContext_ = context;

        return platform::Result<void>::success();
    }

    platform::Result<void> enableInterrupt() override
    {
        interruptEnabled_ = true;

        return platform::Result<void>::success();
    }

    platform::Result<void> disableInterrupt() override
    {
        interruptEnabled_ = false;

        return platform::Result<void>::success();
    }

    void setConfigureFailure(
        platform::ErrorCode error)
    {
        configureShouldSucceed_ = false;
        configureError_ = error;
    }

    void setConfigureSuccess()
    {
        configureShouldSucceed_ = true;
    }

    [[nodiscard]]
    const platform::hal::GpioConfig&
    lastConfig() const
    {
        return lastConfig_;
    }

    [[nodiscard]]
    std::size_t configureCallCount() const
    {
        return configureCallCount_;
    }

    [[nodiscard]]
    std::size_t writeCallCount() const
    {
        return writeCallCount_;
    }

    [[nodiscard]]
    std::size_t toggleCallCount() const
    {
        return toggleCallCount_;
    }

    [[nodiscard]]
    platform::hal::GpioState state() const
    {
        return state_;
    }

    [[nodiscard]]
    bool interruptEnabled() const
    {
        return interruptEnabled_;
    }

private:
    platform::hal::GpioConfig lastConfig_{};

    platform::hal::GpioState state_{
        platform::hal::GpioState::Low
    };

    platform::hal::GpioInterruptCallback callback_{nullptr};

    void* callbackContext_{nullptr};

    platform::ErrorCode configureError_{
        platform::ErrorCode::HardwareFault
    };

    bool configureShouldSucceed_{true};

    bool interruptEnabled_{false};

    std::size_t configureCallCount_{0};
    std::size_t writeCallCount_{0};
    std::size_t toggleCallCount_{0};
};