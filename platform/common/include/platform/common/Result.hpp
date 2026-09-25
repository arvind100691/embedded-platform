#pragma once

#include <cassert>
#include <optional>
#include <utility>

#include "platform/common/ErrorCode.hpp"

namespace platform
{

template <typename T>
class Result
{
public:
    static Result success(T value)
    {
        return Result(std::move(value), ErrorCode::Ok);
    }

    static Result failure(ErrorCode error)
    {
        assert(error != ErrorCode::Ok);
        return Result(std::nullopt, error);
    }

    [[nodiscard]]
    bool hasValue() const noexcept
    {
        return value_.has_value();
    }

    [[nodiscard]]
    explicit operator bool() const noexcept
    {
        return hasValue();
    }

    [[nodiscard]]
    ErrorCode error() const noexcept
    {
        return error_;
    }

    [[nodiscard]]
    const T* value() const noexcept
    {
        return value_ ? &(*value_) : nullptr;
    }

    [[nodiscard]]
    T* value() noexcept
    {
        return value_ ? &(*value_) : nullptr;
    }

private:
    Result(std::optional<T> value, ErrorCode error)
        : value_(std::move(value)),
          error_(error)
    {
    }

    std::optional<T> value_;
    ErrorCode error_{ErrorCode::InternalError};
};

template <>
class Result<void>
{
public:
    static Result success()
    {
        return Result(ErrorCode::Ok);
    }

    static Result failure(ErrorCode error)
    {
        assert(error != ErrorCode::Ok);
        return Result(error);
    }

    [[nodiscard]]
    bool hasValue() const noexcept
    {
        return error_ == ErrorCode::Ok;
    }

    [[nodiscard]]
    explicit operator bool() const noexcept
    {
        return hasValue();
    }

    [[nodiscard]]
    ErrorCode error() const noexcept
    {
        return error_;
    }

private:
    explicit Result(ErrorCode error)
        : error_(error)
    {
    }

    ErrorCode error_{ErrorCode::InternalError};
};

} // namespace platform