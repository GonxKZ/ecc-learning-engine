#pragma once

#include "../core/types.hpp"
#include "../core/log.hpp"
#include "spatial_queries.hpp"

#include <functional>
#include <type_traits>
#include <concepts>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <optional>
#include <variant>

namespace ecscope::ecs::query::operators {

/**
 * @brief Type concepts for query operations
 */
template<typename T>
concept Comparable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept HasPosition = requires(T t) {
    { t.position } -> std::convertible_to<spatial::Vec3>;
} || requires(T t) {
    { t.x } -> std::convertible_to<f32>;
    { t.y } -> std::convertible_to<f32>;
    { t.z } -> std::convertible_to<f32>;
};

/**
 * @brief Comparison operators for query predicates
 */
template<typename T>
struct ComparisonOp {
    enum class Type { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };
    
    Type op_type;
    T value;
    
    ComparisonOp(Type type, const T& val) : op_type(type), value(val) {}
    
    bool operator()(const T& other) const {
        switch (op_type) {
            case Type::Equal: return other == value;
            case Type::NotEqual: return other != value;
            case Type::Less: return other < value;
            case Type::LessEqual: return other <= value;
            case Type::Greater: return other > value;
            case Type::GreaterEqual: return other >= value;
            default: return false;
        }
    }
    
    std::string describe() const {
        switch (op_type) {
            case Type::Equal: return "==";
            case Type::NotEqual: return "!=";
            case Type::Less: return "<";
            case Type::LessEqual: return "<=";
            case Type::Greater: return ">";
            case Type::GreaterEqual: return ">=";
            default: return "unknown";
        }
    }
};

// Convenience factory functions
template<Comparable T>
ComparisonOp<T> equal_to(const T& value) {
    return ComparisonOp<T>(ComparisonOp<T>::Type::Equal, value);
}

template<Comparable T>
ComparisonOp<T> not_equal_to(const T& value) {
    return ComparisonOp<T>(ComparisonOp<T>::Type::NotEqual, value);
}

template<Comparable T>
ComparisonOp<T> less_than(const T& value) {
    return ComparisonOp<T>(ComparisonOp<T>::Type::Less, value);
}

template<Comparable T>
ComparisonOp<T> less_equal(const T& value) {
    return ComparisonOp<T>(ComparisonOp<T>::Type::LessEqual, value);
}

template<Comparable T>
ComparisonOp<T> greater_than(const T& value) {
    return ComparisonOp<T>(ComparisonOp<T>::Type::Greater, value);
}

template<Comparable T>
ComparisonOp<T> greater_equal(const T& value) {
    return ComparisonOp<T>(ComparisonOp<T>::Type::GreaterEqual, value);
}

/**
 * @brief Range operators for numeric types
 */
template<Comparable T>
struct RangeOp {
    T min_value;
    T max_value;
    bool inclusive_min;
    bool inclusive_max;
    
    RangeOp(const T& min_val, const T& max_val, bool inc_min = true, bool inc_max = true)
        : min_value(min_val), max_value(max_val), inclusive_min(inc_min), inclusive_max(inc_max) {}
    
    bool operator()(const T& value) const {
        bool above_min = inclusive_min ? (value >= min_value) : (value > min_value);
        bool below_max = inclusive_max ? (value <= max_value) : (value < max_value);
        return above_min && below_max;
    }
    
    std::string describe() const {
        std::ostringstream oss;
        oss << (inclusive_min ? "[" : "(") << min_value << ", " << max_value << (inclusive_max ? "]" : ")");
        return oss.str();
    }
};

// Convenience factory functions
template<Comparable T>
RangeOp<T> in_range(const T& min_val, const T& max_val) {
    return RangeOp<T>(min_val, max_val, true, true);
}

template<Comparable T>
RangeOp<T> in_range_exclusive(const T& min_val, const T& max_val) {
    return RangeOp<T>(min_val, max_val, false, false);
}

template<Comparable T>
RangeOp<T> in_range_left_open(const T& min_val, const T& max_val) {
    return RangeOp<T>(min_val, max_val, false, true);
}

template<Comparable T>
RangeOp<T> in_range_right_open(const T& min_val, const T& max_val) {
    return RangeOp<T>(min_val, max_val, true, false);
}

/**
 * @brief String matching operators
 */
struct StringOp {
    enum class Type { Contains, StartsWith, EndsWith, Matches, Empty, NotEmpty };
    
    Type op_type;
    std::string pattern;
    bool case_sensitive;
    
    StringOp(Type type, const std::string& pat = "", bool case_sens = true)
        : op_type(type), pattern(pat), case_sensitive(case_sens) {}
    
    bool operator()(const std::string& str) const {
        std::string search_str = case_sensitive ? str : to_lower(str);
        std::string search_pattern = case_sensitive ? pattern : to_lower(pattern);
        
        switch (op_type) {
            case Type::Contains:
                return search_str.find(search_pattern) != std::string::npos;
            case Type::StartsWith:
                return search_str.size() >= search_pattern.size() &&
                       search_str.substr(0, search_pattern.size()) == search_pattern;
            case Type::EndsWith:
                return search_str.size() >= search_pattern.size() &&
                       search_str.substr(search_str.size() - search_pattern.size()) == search_pattern;
            case Type::Matches:
                return search_str == search_pattern;
            case Type::Empty:
                return str.empty();
            case Type::NotEmpty:
                return !str.empty();
            default:
                return false;
        }
    }
    
    std::string describe() const {
        switch (op_type) {
            case Type::Contains: return "contains('" + pattern + "')";
            case Type::StartsWith: return "starts_with('" + pattern + "')";
            case Type::EndsWith: return "ends_with('" + pattern + "')";
            case Type::Matches: return "matches('" + pattern + "')";
            case Type::Empty: return "is_empty()";
            case Type::NotEmpty: return "is_not_empty()";
            default: return "unknown";
        }
    }
    
private:
    static std::string to_lower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
};

// Convenience factory functions
StringOp contains(const std::string& pattern, bool case_sensitive = true) {
    return StringOp(StringOp::Type::Contains, pattern, case_sensitive);
}

StringOp starts_with(const std::string& pattern, bool case_sensitive = true) {
    return StringOp(StringOp::Type::StartsWith, pattern, case_sensitive);
}

StringOp ends_with(const std::string& pattern, bool case_sensitive = true) {
    return StringOp(StringOp::Type::EndsWith, pattern, case_sensitive);
}

StringOp matches(const std::string& pattern, bool case_sensitive = true) {
    return StringOp(StringOp::Type::Matches, pattern, case_sensitive);
}

StringOp is_empty() {
    return StringOp(StringOp::Type::Empty);
}

StringOp is_not_empty() {
    return StringOp(StringOp::Type::NotEmpty);
}

/**
 * @brief Collection operators for array/vector-like components
 */
template<typename Container>
struct CollectionOp {
    enum class Type { HasSize, IsEmpty, NotEmpty, Contains, All, Any, None };
    
    Type op_type;
    usize target_size;
    typename Container::value_type target_value;
    std::function<bool(const typename Container::value_type&)> predicate;
    
    CollectionOp(Type type) : op_type(type), target_size(0) {}
    
    CollectionOp(Type type, usize size) : op_type(type), target_size(size) {}
    
    CollectionOp(Type type, const typename Container::value_type& value) 
        : op_type(type), target_size(0), target_value(value) {}
    
    CollectionOp(Type type, std::function<bool(const typename Container::value_type&)> pred)
        : op_type(type), target_size(0), predicate(std::move(pred)) {}
    
    bool operator()(const Container& container) const {
        switch (op_type) {
            case Type::HasSize:
                return container.size() == target_size;
            case Type::IsEmpty:
                return container.empty();
            case Type::NotEmpty:
                return !container.empty();
            case Type::Contains:
                return std::find(container.begin(), container.end(), target_value) != container.end();
            case Type::All:
                return std::all_of(container.begin(), container.end(), predicate);
            case Type::Any:
                return std::any_of(container.begin(), container.end(), predicate);
            case Type::None:
                return std::none_of(container.begin(), container.end(), predicate);
            default:
                return false;
        }
    }
    
    std::string describe() const {
        switch (op_type) {
            case Type::HasSize: return "has_size(" + std::to_string(target_size) + ")";
            case Type::IsEmpty: return "is_empty()";
            case Type::NotEmpty: return "not_empty()";
            case Type::Contains: return "contains(value)";
            case Type::All: return "all(predicate)";
            case Type::Any: return "any(predicate)";
            case Type::None: return "none(predicate)";
            default: return "unknown";
        }
    }
};

// Convenience factory functions
template<typename Container>
CollectionOp<Container> has_size(usize size) {
    return CollectionOp<Container>(CollectionOp<Container>::Type::HasSize, size);
}

template<typename Container>
CollectionOp<Container> is_empty() {
    return CollectionOp<Container>(CollectionOp<Container>::Type::IsEmpty);
}

template<typename Container>
CollectionOp<Container> not_empty() {
    return CollectionOp<Container>(CollectionOp<Container>::Type::NotEmpty);
}

template<typename Container>
CollectionOp<Container> contains(const typename Container::value_type& value) {
    return CollectionOp<Container>(CollectionOp<Container>::Type::Contains, value);
}

template<typename Container>
CollectionOp<Container> all_match(std::function<bool(const typename Container::value_type&)> predicate) {
    return CollectionOp<Container>(CollectionOp<Container>::Type::All, std::move(predicate));
}

template<typename Container>
CollectionOp<Container> any_match(std::function<bool(const typename Container::value_type&)> predicate) {
    return CollectionOp<Container>(CollectionOp<Container>::Type::Any, std::move(predicate));
}

template<typename Container>
CollectionOp<Container> none_match(std::function<bool(const typename Container::value_type&)> predicate) {
    return CollectionOp<Container>(CollectionOp<Container>::Type::None, std::move(predicate));
}

/**
 * @brief Spatial operators for position-based queries
 */
template<typename T>
    requires HasPosition<T>
struct SpatialOp {
    enum class Type { WithinRadius, WithinBox, WithinRegion, Nearest, Farthest };
    
    Type op_type;
    spatial::Vec3 reference_point;
    f32 radius;
    spatial::AABB bounding_box;
    spatial::Region region;
    usize count; // For nearest/farthest operations
    
    SpatialOp(Type type, const spatial::Vec3& point, f32 rad)
        : op_type(type), reference_point(point), radius(rad), count(1) {}
    
    SpatialOp(Type type, const spatial::AABB& box)
        : op_type(type), bounding_box(box), radius(0), count(1) {}
    
    SpatialOp(Type type, const spatial::Region& reg)
        : op_type(type), region(reg), radius(0), count(1) {}
    
    SpatialOp(Type type, const spatial::Vec3& point, usize n)
        : op_type(type), reference_point(point), radius(0), count(n) {}
    
    bool operator()(const T& component) const {
        spatial::Vec3 pos = extract_position(component);
        
        switch (op_type) {
            case Type::WithinRadius: {
                f32 dist_sq = (pos - reference_point).length_squared();
                return dist_sq <= radius * radius;
            }
            case Type::WithinBox:
                return bounding_box.contains(pos);
            case Type::WithinRegion:
                return region.contains(pos);
            case Type::Nearest:
            case Type::Farthest:
                // These need to be handled differently as they require sorting
                return true; // Always pass, sorting happens elsewhere
            default:
                return false;
        }
    }
    
    f32 distance_to(const T& component) const {
        spatial::Vec3 pos = extract_position(component);
        return (pos - reference_point).length();
    }
    
    std::string describe() const {
        switch (op_type) {
            case Type::WithinRadius: return "within_radius(" + std::to_string(radius) + ")";
            case Type::WithinBox: return "within_box()";
            case Type::WithinRegion: return "within_region()";
            case Type::Nearest: return "nearest(" + std::to_string(count) + ")";
            case Type::Farthest: return "farthest(" + std::to_string(count) + ")";
            default: return "unknown";
        }
    }
    
private:
    spatial::Vec3 extract_position(const T& component) const {
        if constexpr (requires { component.position; }) {
            return component.position;
        } else if constexpr (requires { component.x; component.y; component.z; }) {
            return spatial::Vec3(component.x, component.y, component.z);
        } else {
            return spatial::Vec3(0, 0, 0);
        }
    }
};

// Convenience factory functions
template<HasPosition T>
SpatialOp<T> within_radius(const spatial::Vec3& center, f32 radius) {
    return SpatialOp<T>(SpatialOp<T>::Type::WithinRadius, center, radius);
}

template<HasPosition T>
SpatialOp<T> within_box(const spatial::AABB& box) {
    return SpatialOp<T>(SpatialOp<T>::Type::WithinBox, box);
}

template<HasPosition T>
SpatialOp<T> within_region(const spatial::Region& region) {
    return SpatialOp<T>(SpatialOp<T>::Type::WithinRegion, region);
}

template<HasPosition T>
SpatialOp<T> nearest_to(const spatial::Vec3& point, usize count = 1) {
    return SpatialOp<T>(SpatialOp<T>::Type::Nearest, point, count);
}

template<HasPosition T>
SpatialOp<T> farthest_from(const spatial::Vec3& point, usize count = 1) {
    return SpatialOp<T>(SpatialOp<T>::Type::Farthest, point, count);
}

/**
 * @brief Logical operators for combining predicates
 */
template<typename Predicate>
class LogicalOp {
private:
    std::vector<Predicate> predicates_;
    enum class Type { And, Or, Not } op_type_;
    
public:
    explicit LogicalOp(Type type) : op_type_(type) {}
    
    LogicalOp& add(const Predicate& predicate) {
        predicates_.push_back(predicate);
        return *this;
    }
    
    template<typename T>
    bool operator()(const T& value) const {
        switch (op_type_) {
            case Type::And:
                return std::all_of(predicates_.begin(), predicates_.end(),
                    [&value](const auto& pred) { return pred(value); });
            case Type::Or:
                return std::any_of(predicates_.begin(), predicates_.end(),
                    [&value](const auto& pred) { return pred(value); });
            case Type::Not:
                // For NOT, we typically have one predicate
                return predicates_.empty() || !predicates_[0](value);
            default:
                return false;
        }
    }
    
    std::string describe() const {
        std::ostringstream oss;
        switch (op_type_) {
            case Type::And:
                oss << "AND(";
                for (usize i = 0; i < predicates_.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << "pred" << i;
                }
                oss << ")";
                break;
            case Type::Or:
                oss << "OR(";
                for (usize i = 0; i < predicates_.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << "pred" << i;
                }
                oss << ")";
                break;
            case Type::Not:
                oss << "NOT(pred)";
                break;
        }
        return oss.str();
    }
};

// Convenience factory functions
template<typename Predicate>
LogicalOp<Predicate> logical_and() {
    return LogicalOp<Predicate>(LogicalOp<Predicate>::Type::And);
}

template<typename Predicate>
LogicalOp<Predicate> logical_or() {
    return LogicalOp<Predicate>(LogicalOp<Predicate>::Type::Or);
}

template<typename Predicate>
LogicalOp<Predicate> logical_not() {
    return LogicalOp<Predicate>(LogicalOp<Predicate>::Type::Not);
}

/**
 * @brief Mathematical operators for numeric calculations
 */
template<Arithmetic T>
struct MathOp {
    enum class Type { 
        Sum, Average, Min, Max, Count, Variance, StandardDeviation,
        Median, Mode, Range, Percentile
    };
    
    Type op_type;
    f64 percentile_value; // For percentile calculations (0.0 to 1.0)
    
    explicit MathOp(Type type, f64 percentile = 0.5) 
        : op_type(type), percentile_value(percentile) {}
    
    template<typename Container>
    std::optional<f64> operator()(const Container& values) const {
        if (values.empty()) return std::nullopt;
        
        std::vector<T> sorted_values;
        if (op_type == Type::Median || op_type == Type::Percentile) {
            sorted_values.assign(values.begin(), values.end());
            std::sort(sorted_values.begin(), sorted_values.end());
        }
        
        switch (op_type) {
            case Type::Sum:
                return static_cast<f64>(std::accumulate(values.begin(), values.end(), T{}));
                
            case Type::Average: {
                f64 sum = std::accumulate(values.begin(), values.end(), 0.0);
                return sum / values.size();
            }
            
            case Type::Min:
                return static_cast<f64>(*std::min_element(values.begin(), values.end()));
                
            case Type::Max:
                return static_cast<f64>(*std::max_element(values.begin(), values.end()));
                
            case Type::Count:
                return static_cast<f64>(values.size());
                
            case Type::Variance: {
                f64 mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
                f64 sq_sum = std::accumulate(values.begin(), values.end(), 0.0,
                    [mean](f64 sum, T val) {
                        f64 diff = static_cast<f64>(val) - mean;
                        return sum + diff * diff;
                    });
                return sq_sum / values.size();
            }
            
            case Type::StandardDeviation: {
                auto variance = MathOp<T>(Type::Variance)(values);
                return variance ? std::sqrt(*variance) : std::nullopt;
            }
            
            case Type::Median: {
                usize mid = sorted_values.size() / 2;
                if (sorted_values.size() % 2 == 0) {
                    return static_cast<f64>(sorted_values[mid - 1] + sorted_values[mid]) / 2.0;
                } else {
                    return static_cast<f64>(sorted_values[mid]);
                }
            }
            
            case Type::Mode: {
                std::unordered_map<T, usize> counts;
                for (const auto& val : values) {
                    counts[val]++;
                }
                
                auto max_count_it = std::max_element(counts.begin(), counts.end(),
                    [](const auto& a, const auto& b) { return a.second < b.second; });
                
                return max_count_it != counts.end() ? 
                    static_cast<f64>(max_count_it->first) : std::nullopt;
            }
            
            case Type::Range: {
                auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
                return static_cast<f64>(*max_it - *min_it);
            }
            
            case Type::Percentile: {
                f64 rank = percentile_value * (sorted_values.size() - 1);
                usize lower = static_cast<usize>(std::floor(rank));
                usize upper = static_cast<usize>(std::ceil(rank));
                
                if (lower == upper) {
                    return static_cast<f64>(sorted_values[lower]);
                } else {
                    f64 weight = rank - lower;
                    return static_cast<f64>(sorted_values[lower]) * (1.0 - weight) +
                           static_cast<f64>(sorted_values[upper]) * weight;
                }
            }
            
            default:
                return std::nullopt;
        }
    }
    
    std::string describe() const {
        switch (op_type) {
            case Type::Sum: return "sum()";
            case Type::Average: return "average()";
            case Type::Min: return "min()";
            case Type::Max: return "max()";
            case Type::Count: return "count()";
            case Type::Variance: return "variance()";
            case Type::StandardDeviation: return "std_dev()";
            case Type::Median: return "median()";
            case Type::Mode: return "mode()";
            case Type::Range: return "range()";
            case Type::Percentile: 
                return "percentile(" + std::to_string(percentile_value * 100.0) + "%)";
            default: return "unknown";
        }
    }
};

// Convenience factory functions
template<Arithmetic T>
MathOp<T> sum() { return MathOp<T>(MathOp<T>::Type::Sum); }

template<Arithmetic T>
MathOp<T> average() { return MathOp<T>(MathOp<T>::Type::Average); }

template<Arithmetic T>
MathOp<T> minimum() { return MathOp<T>(MathOp<T>::Type::Min); }

template<Arithmetic T>
MathOp<T> maximum() { return MathOp<T>(MathOp<T>::Type::Max); }

template<Arithmetic T>
MathOp<T> count() { return MathOp<T>(MathOp<T>::Type::Count); }

template<Arithmetic T>
MathOp<T> variance() { return MathOp<T>(MathOp<T>::Type::Variance); }

template<Arithmetic T>
MathOp<T> standard_deviation() { return MathOp<T>(MathOp<T>::Type::StandardDeviation); }

template<Arithmetic T>
MathOp<T> median() { return MathOp<T>(MathOp<T>::Type::Median); }

template<Arithmetic T>
MathOp<T> mode() { return MathOp<T>(MathOp<T>::Type::Mode); }

template<Arithmetic T>
MathOp<T> range() { return MathOp<T>(MathOp<T>::Type::Range); }

template<Arithmetic T>
MathOp<T> percentile(f64 p) { return MathOp<T>(MathOp<T>::Type::Percentile, p); }

/**
 * @brief Temporal operators for time-based queries
 */
template<typename TimeType>
struct TemporalOp {
    enum class Type { 
        Before, After, Between, WithinLast, OlderThan,
        SameDay, SameWeek, SameMonth, SameYear
    };
    
    Type op_type;
    TimeType reference_time;
    TimeType end_time; // For range operations
    std::chrono::duration<f64> time_span; // For relative operations
    
    TemporalOp(Type type, const TimeType& ref_time)
        : op_type(type), reference_time(ref_time) {}
    
    TemporalOp(Type type, const TimeType& ref_time, const TimeType& end_time_)
        : op_type(type), reference_time(ref_time), end_time(end_time_) {}
    
    TemporalOp(Type type, std::chrono::duration<f64> span)
        : op_type(type), time_span(span) {}
    
    bool operator()(const TimeType& time) const {
        switch (op_type) {
            case Type::Before:
                return time < reference_time;
            case Type::After:
                return time > reference_time;
            case Type::Between:
                return time >= reference_time && time <= end_time;
            case Type::WithinLast: {
                auto now = std::chrono::steady_clock::now();
                auto threshold = now - time_span;
                return time >= threshold;
            }
            case Type::OlderThan: {
                auto now = std::chrono::steady_clock::now();
                auto threshold = now - time_span;
                return time < threshold;
            }
            // Date-specific operations would need proper date/time handling
            case Type::SameDay:
            case Type::SameWeek:
            case Type::SameMonth:
            case Type::SameYear:
                return false; // Simplified - would need actual date comparison
            default:
                return false;
        }
    }
    
    std::string describe() const {
        switch (op_type) {
            case Type::Before: return "before(time)";
            case Type::After: return "after(time)";
            case Type::Between: return "between(start, end)";
            case Type::WithinLast: return "within_last(duration)";
            case Type::OlderThan: return "older_than(duration)";
            case Type::SameDay: return "same_day(time)";
            case Type::SameWeek: return "same_week(time)";
            case Type::SameMonth: return "same_month(time)";
            case Type::SameYear: return "same_year(time)";
            default: return "unknown";
        }
    }
};

// Convenience factory functions
template<typename TimeType>
TemporalOp<TimeType> before(const TimeType& time) {
    return TemporalOp<TimeType>(TemporalOp<TimeType>::Type::Before, time);
}

template<typename TimeType>
TemporalOp<TimeType> after(const TimeType& time) {
    return TemporalOp<TimeType>(TemporalOp<TimeType>::Type::After, time);
}

template<typename TimeType>
TemporalOp<TimeType> between(const TimeType& start, const TimeType& end) {
    return TemporalOp<TimeType>(TemporalOp<TimeType>::Type::Between, start, end);
}

template<typename TimeType>
TemporalOp<TimeType> within_last(std::chrono::duration<f64> duration) {
    return TemporalOp<TimeType>(TemporalOp<TimeType>::Type::WithinLast, duration);
}

template<typename TimeType>
TemporalOp<TimeType> older_than(std::chrono::duration<f64> duration) {
    return TemporalOp<TimeType>(TemporalOp<TimeType>::Type::OlderThan, duration);
}

} // namespace ecscope::ecs::query::operators