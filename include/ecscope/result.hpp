#pragma once

#include "types.hpp"
#include <variant>
#include <utility>
#include <type_traits>
#include <cassert>

namespace ecscope::core {

// Rust-style Result type for error handling without exceptions
template<typename T, typename E>
class Result {
private:
    std::variant<T, E> data_;
    
public:
    // Constructors
    Result() = delete;
    
    // Create Ok result
    static Result Ok(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) {
        return Result(std::forward<T>(value), true);
    }
    
    static Result Ok(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        return Result(value, true);
    }
    
    // Create Err result  
    static Result Err(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>) {
        return Result(std::forward<E>(error), false);
    }
    
    static Result Err(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>) {
        return Result(error, false);
    }
    
    // Query state
    constexpr bool is_ok() const noexcept {
        return std::holds_alternative<T>(data_);
    }
    
    constexpr bool is_err() const noexcept {
        return std::holds_alternative<E>(data_);
    }
    
    // Explicit bool conversion (true if Ok)
    explicit constexpr operator bool() const noexcept {
        return is_ok();
    }
    
    // Access value (unsafe - asserts if Err)
    T& unwrap() & {
        assert(is_ok() && "Called unwrap() on Err result");
        return std::get<T>(data_);
    }
    
    const T& unwrap() const& {
        assert(is_ok() && "Called unwrap() on Err result");
        return std::get<T>(data_);
    }
    
    T&& unwrap() && {
        assert(is_ok() && "Called unwrap() on Err result");
        return std::get<T>(std::move(data_));
    }
    
    // Access error (unsafe - asserts if Ok)
    E& unwrap_err() & {
        assert(is_err() && "Called unwrap_err() on Ok result");
        return std::get<E>(data_);
    }
    
    const E& unwrap_err() const& {
        assert(is_err() && "Called unwrap_err() on Ok result");
        return std::get<E>(data_);
    }
    
    E&& unwrap_err() && {
        assert(is_err() && "Called unwrap_err() on Ok result");
        return std::get<E>(std::move(data_));
    }
    
    // Safe access with default value
    template<typename U>
    T unwrap_or(U&& default_value) const& {
        if (is_ok()) {
            return std::get<T>(data_);
        }
        return static_cast<T>(std::forward<U>(default_value));
    }
    
    template<typename U>
    T unwrap_or(U&& default_value) && {
        if (is_ok()) {
            return std::get<T>(std::move(data_));
        }
        return static_cast<T>(std::forward<U>(default_value));
    }
    
    // Safe access with function
    template<typename F>
    T unwrap_or_else(F&& func) const& {
        if (is_ok()) {
            return std::get<T>(data_);
        }
        return std::forward<F>(func)(std::get<E>(data_));
    }
    
    template<typename F>
    T unwrap_or_else(F&& func) && {
        if (is_ok()) {
            return std::get<T>(std::move(data_));
        }
        return std::forward<F>(func)(std::get<E>(std::move(data_)));
    }
    
    // Map operations (monadic interface)
    template<typename F>
    auto map(F&& func) const& -> Result<std::invoke_result_t<F, const T&>, E> {
        using U = std::invoke_result_t<F, const T&>;
        if (is_ok()) {
            return Result<U, E>::Ok(std::forward<F>(func)(std::get<T>(data_)));
        }
        return Result<U, E>::Err(std::get<E>(data_));
    }
    
    template<typename F>
    auto map(F&& func) && -> Result<std::invoke_result_t<F, T&&>, E> {
        using U = std::invoke_result_t<F, T&&>;
        if (is_ok()) {
            return Result<U, E>::Ok(std::forward<F>(func)(std::get<T>(std::move(data_))));
        }
        return Result<U, E>::Err(std::get<E>(std::move(data_)));
    }
    
    // Map error
    template<typename F>
    auto map_err(F&& func) const& -> Result<T, std::invoke_result_t<F, const E&>> {
        using F2 = std::invoke_result_t<F, const E&>;
        if (is_err()) {
            return Result<T, F2>::Err(std::forward<F>(func)(std::get<E>(data_)));
        }
        return Result<T, F2>::Ok(std::get<T>(data_));
    }
    
    template<typename F>
    auto map_err(F&& func) && -> Result<T, std::invoke_result_t<F, E&&>> {
        using F2 = std::invoke_result_t<F, E&&>;
        if (is_err()) {
            return Result<T, F2>::Err(std::forward<F>(func)(std::get<E>(std::move(data_))));
        }
        return Result<T, F2>::Ok(std::get<T>(std::move(data_)));
    }
    
    // And then (flatmap)
    template<typename F>
    auto and_then(F&& func) const& -> std::invoke_result_t<F, const T&> {
        if (is_ok()) {
            return std::forward<F>(func)(std::get<T>(data_));
        }
        using ResultType = std::invoke_result_t<F, const T&>;
        return ResultType::Err(std::get<E>(data_));
    }
    
    template<typename F>
    auto and_then(F&& func) && -> std::invoke_result_t<F, T&&> {
        if (is_ok()) {
            return std::forward<F>(func)(std::get<T>(std::move(data_)));
        }
        using ResultType = std::invoke_result_t<F, T&&>;
        return ResultType::Err(std::get<E>(std::move(data_)));
    }
    
private:
    template<typename U>
    explicit Result(U&& value, bool is_ok) 
        : data_(is_ok ? std::variant<T, E>{T(std::forward<U>(value))} 
                      : std::variant<T, E>{E(std::forward<U>(value))}) {}
};

// Specialization for void success type (common case)
template<typename E>
class Result<void, E> {
private:
    std::variant<std::monostate, E> data_;
    
public:
    Result() = delete;
    
    static Result Ok() noexcept {
        return Result(true);
    }
    
    static Result Err(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>) {
        return Result(std::forward<E>(error));
    }
    
    static Result Err(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>) {
        return Result(error);
    }
    
    constexpr bool is_ok() const noexcept {
        return std::holds_alternative<std::monostate>(data_);
    }
    
    constexpr bool is_err() const noexcept {
        return std::holds_alternative<E>(data_);
    }
    
    explicit constexpr operator bool() const noexcept {
        return is_ok();
    }
    
    void unwrap() const {
        assert(is_ok() && "Called unwrap() on Err result");
    }
    
    E& unwrap_err() & {
        assert(is_err() && "Called unwrap_err() on Ok result");
        return std::get<E>(data_);
    }
    
    const E& unwrap_err() const& {
        assert(is_err() && "Called unwrap_err() on Ok result");
        return std::get<E>(data_);
    }
    
    E&& unwrap_err() && {
        assert(is_err() && "Called unwrap_err() on Ok result");
        return std::get<E>(std::move(data_));
    }
    
private:
    explicit Result(bool is_ok) 
        : data_(is_ok ? std::variant<std::monostate, E>{std::monostate{}} 
                      : std::variant<std::monostate, E>{}) {}
                      
    explicit Result(E&& error)
        : data_(std::forward<E>(error)) {}
        
    explicit Result(const E& error)
        : data_(error) {}
};

// Common error types for the engine
enum class CoreError : u32 {
    Success = 0,
    InvalidArgument,
    OutOfMemory,
    FileNotFound,
    PermissionDenied,
    InvalidState,
    Timeout,
    NotImplemented,
    Unknown
};

// String representation of core errors
constexpr const char* to_string(CoreError error) noexcept {
    switch (error) {
        case CoreError::Success:           return "Success";
        case CoreError::InvalidArgument:   return "Invalid argument";
        case CoreError::OutOfMemory:       return "Out of memory";
        case CoreError::FileNotFound:      return "File not found";
        case CoreError::PermissionDenied:  return "Permission denied";
        case CoreError::InvalidState:      return "Invalid state";
        case CoreError::Timeout:           return "Timeout";
        case CoreError::NotImplemented:    return "Not implemented";
        case CoreError::Unknown:           return "Unknown error";
        default:                           return "Undefined error";
    }
}

// Common type aliases
template<typename T>
using CoreResult = Result<T, CoreError>;

using VoidResult = Result<void, CoreError>;

// Helper macros for easier error handling
#define TRY(expr) \
    do { \
        auto result = (expr); \
        if (!result) { \
            return decltype(result)::Err(std::move(result).unwrap_err()); \
        } \
    } while(0)

#define TRY_ASSIGN(var, expr) \
    do { \
        auto result = (expr); \
        if (!result) { \
            return decltype(result)::Err(std::move(result).unwrap_err()); \
        } \
        var = std::move(result).unwrap(); \
    } while(0)

} // namespace ecscope::core