#pragma once

#include "types.hpp"
#include "time.hpp"
#include <iostream>
#include <sstream>
#include <mutex>
#include <string_view>
#include <iomanip>
#include <ctime>

namespace ecscope::core {

// Log levels in order of severity
enum class LogLevel : u8 {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5
};

// Convert log level to string
constexpr std::string_view to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default: return "UNKNOWN";
    }
}

// ANSI color codes for console output
namespace colors {
    constexpr const char* reset = "\033[0m";
    constexpr const char* trace = "\033[37m"; // White
    constexpr const char* debug = "\033[36m"; // Cyan
    constexpr const char* info = "\033[32m";  // Green
    constexpr const char* warn = "\033[33m";  // Yellow
    constexpr const char* error = "\033[31m"; // Red
    constexpr const char* fatal = "\033[35m"; // Magenta
    
    constexpr const char* get_color(LogLevel level) noexcept {
        switch (level) {
            case LogLevel::Trace: return trace;
            case LogLevel::Debug: return debug;
            case LogLevel::Info:  return info;
            case LogLevel::Warn:  return warn;
            case LogLevel::Error: return error;
            case LogLevel::Fatal: return fatal;
            default: return reset;
        }
    }
}

// Logger interface
class Logger {
public:
    virtual ~Logger() = default;
    virtual void log(LogLevel level, std::string_view message, 
                    std::string_view file, int line, std::string_view func) = 0;
    virtual void set_level(LogLevel level) = 0;
    virtual LogLevel level() const = 0;
    virtual void flush() = 0;
};

// Console logger implementation
class ConsoleLogger : public Logger {
private:
    LogLevel min_level_{LogLevel::Info};
    bool use_colors_{true};
    mutable std::mutex mutex_;
    
public:
    explicit ConsoleLogger(LogLevel level = LogLevel::Info, bool colors = true) noexcept
        : min_level_(level), use_colors_(colors) {}
    
    void log(LogLevel level, std::string_view message, 
            std::string_view file, int line, std::string_view func) override {
        if (level < min_level_) return;
        
        std::lock_guard lock(mutex_);
        
        auto now = Time::now();
        auto time_t = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                std::chrono::time_point<std::chrono::system_clock>(
                    std::chrono::duration_cast<std::chrono::system_clock::duration>(
                        now.time_since_epoch()
                    )
                )
            )
        );
        
        // Extract filename from full path
        auto filename = file.substr(file.find_last_of("/\\") + 1);
        
        if (use_colors_) {
            std::cout << colors::get_color(level);
        }
        
        // Format: [TIMESTAMP] [LEVEL] filename:line in func(): message
        std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] ";
        std::cout << "[" << to_string(level) << "] ";
        std::cout << filename << ":" << line << " in " << func << "(): ";
        std::cout << message;
        
        if (use_colors_) {
            std::cout << colors::reset;
        }
        
        std::cout << std::endl;
        
        // Auto-flush for errors and fatal
        if (level >= LogLevel::Error) {
            std::cout.flush();
        }
    }
    
    void set_level(LogLevel level) override {
        std::lock_guard lock(mutex_);
        min_level_ = level;
    }
    
    LogLevel level() const override {
        std::lock_guard lock(mutex_);
        return min_level_;
    }
    
    void flush() override {
        std::lock_guard lock(mutex_);
        std::cout.flush();
    }
    
    void set_colors(bool enable) noexcept {
        std::lock_guard lock(mutex_);
        use_colors_ = enable;
    }
};

// Global logger instance
Logger& get_logger() noexcept;
void set_logger(std::unique_ptr<Logger> logger) noexcept;

// Simple logging functions (message only for now - formatting can be added later)
inline void log_trace(std::string_view file, int line, std::string_view func, 
              std::string_view message) {
    if (get_logger().level() <= LogLevel::Trace) {
        get_logger().log(LogLevel::Trace, message, file, line, func);
    }
}

inline void log_debug(std::string_view file, int line, std::string_view func, 
              std::string_view message) {
    if (get_logger().level() <= LogLevel::Debug) {
        get_logger().log(LogLevel::Debug, message, file, line, func);
    }
}

inline void log_info(std::string_view file, int line, std::string_view func, 
             std::string_view message) {
    if (get_logger().level() <= LogLevel::Info) {
        get_logger().log(LogLevel::Info, message, file, line, func);
    }
}

inline void log_warn(std::string_view file, int line, std::string_view func, 
             std::string_view message) {
    if (get_logger().level() <= LogLevel::Warn) {
        get_logger().log(LogLevel::Warn, message, file, line, func);
    }
}

inline void log_error(std::string_view file, int line, std::string_view func, 
              std::string_view message) {
    if (get_logger().level() <= LogLevel::Error) {
        get_logger().log(LogLevel::Error, message, file, line, func);
    }
}

inline void log_fatal(std::string_view file, int line, std::string_view func, 
              std::string_view message) {
    get_logger().log(LogLevel::Fatal, message, file, line, func);
    get_logger().flush();
}

// Convenience logging macros
#define LOG_TRACE(message) \
    ecscope::core::log_trace(__FILE__, __LINE__, __func__, message)

#define LOG_DEBUG(message) \
    ecscope::core::log_debug(__FILE__, __LINE__, __func__, message)

#define LOG_INFO(message) \
    ecscope::core::log_info(__FILE__, __LINE__, __func__, message)

#define LOG_WARN(message) \
    ecscope::core::log_warn(__FILE__, __LINE__, __func__, message)

#define LOG_ERROR(message) \
    ecscope::core::log_error(__FILE__, __LINE__, __func__, message)

#define LOG_FATAL(message) \
    ecscope::core::log_fatal(__FILE__, __LINE__, __func__, message)

// Conditional compilation macros
#ifdef ECSCOPE_ENABLE_TRACE_LOGGING
    #define TRACE_LOG(message) LOG_TRACE(message)
#else
    #define TRACE_LOG(message) ((void)0)
#endif

#ifdef ECSCOPE_ENABLE_DEBUG_LOGGING
    #define DEBUG_LOG(message) LOG_DEBUG(message)
#else
    #define DEBUG_LOG(message) ((void)0)
#endif

} // namespace ecscope::core