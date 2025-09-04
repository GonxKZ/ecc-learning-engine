/**
 * @file string_utils.hpp
 * @brief Simple string utilities for ECScope
 */

#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace ecscope::core {

/**
 * @brief Simple string formatting utility to replace complex formatting libraries
 */
class StringFormatter {
private:
    std::ostringstream oss_;

public:
    StringFormatter() = default;
    
    template<typename T>
    StringFormatter& operator<<(const T& value) {
        oss_ << value;
        return *this;
    }
    
    // Special handling for floating point precision
    StringFormatter& precision(int p) {
        oss_ << std::fixed << std::setprecision(p);
        return *this;
    }
    
    std::string str() const {
        return oss_.str();
    }
    
    operator std::string() const {
        return str();
    }
};

// Convenient macro for simple formatting
#define FORMAT(expr) (StringFormatter() << expr).str()

// Helper functions for common formatting tasks
inline std::string format_percentage(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << (value * 100.0) << "%";
    return oss.str();
}

inline std::string format_time_ms(double ms) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << ms << "ms";
    return oss.str();
}

inline std::string format_speedup(double speedup) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << speedup << "x";
    return oss.str();
}

} // namespace ecscope::core