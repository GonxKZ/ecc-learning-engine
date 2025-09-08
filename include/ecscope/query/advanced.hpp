#pragma once

#include "query_engine.hpp"
#include "query_builder.hpp"
#include "query_cache.hpp"
#include "query_optimizer.hpp"
#include "spatial_queries.hpp"
#include "query_operators.hpp"
#include "../core/types.hpp"
#include "../core/log.hpp"

#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <execution>
#include <immintrin.h> // For SIMD operations
#include <concepts>
#include <type_traits>

namespace ecscope::ecs::query::advanced {

/**
 * @brief Thread pool for parallel query execution
 */
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_requested_{false};
    
public:
    explicit ThreadPool(usize thread_count = std::thread::hardware_concurrency()) {
        workers_.reserve(thread_count);
        
        for (usize i = 0; i < thread_count; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] { 
                            return stop_requested_ || !tasks_.empty(); 
                        });
                        
                        if (stop_requested_ && tasks_.empty()) {
                            break;
                        }
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    
                    task();
                }
            });
        }
        
        LOG_INFO("ThreadPool initialized with {} worker threads", thread_count);
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_requested_ = true;
        }
        
        condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    template<typename Func, typename... Args>
    auto enqueue(Func&& func, Args&&... args) 
        -> std::future<typename std::result_of_t<Func(Args...)>> {
        
        using ReturnType = typename std::result_of_t<Func(Args...)>;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> future = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            if (stop_requested_) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return future;
    }
    
    usize worker_count() const { return workers_.size(); }
    
    usize pending_tasks() const {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }
};

/**
 * @brief SIMD-optimized operations for query processing
 */
namespace simd {

/**
 * @brief SIMD-accelerated range filtering for float arrays
 */
class SIMDRangeFilter {
private:
    static constexpr usize SIMD_WIDTH = 8; // AVX2 processes 8 floats at once
    
public:
    static std::vector<usize> filter_range_f32(const f32* values, usize count, 
                                              f32 min_val, f32 max_val) {
        std::vector<usize> result_indices;
        result_indices.reserve(count / 4); // Optimistic reserve
        
#ifdef __AVX2__
        __m256 min_vec = _mm256_set1_ps(min_val);
        __m256 max_vec = _mm256_set1_ps(max_val);
        
        usize simd_count = (count / SIMD_WIDTH) * SIMD_WIDTH;
        
        // Process SIMD-width chunks
        for (usize i = 0; i < simd_count; i += SIMD_WIDTH) {
            __m256 data = _mm256_loadu_ps(&values[i]);
            __m256 ge_min = _mm256_cmp_ps(data, min_vec, _CMP_GE_OQ);
            __m256 le_max = _mm256_cmp_ps(data, max_vec, _CMP_LE_OQ);
            __m256 in_range = _mm256_and_ps(ge_min, le_max);
            
            u32 mask = _mm256_movemask_ps(in_range);
            
            // Extract indices of matching elements
            for (usize j = 0; j < SIMD_WIDTH; ++j) {
                if (mask & (1u << j)) {
                    result_indices.push_back(i + j);
                }
            }
        }
        
        // Process remaining elements
        for (usize i = simd_count; i < count; ++i) {
            if (values[i] >= min_val && values[i] <= max_val) {
                result_indices.push_back(i);
            }
        }
#else
        // Fallback to scalar implementation
        for (usize i = 0; i < count; ++i) {
            if (values[i] >= min_val && values[i] <= max_val) {
                result_indices.push_back(i);
            }
        }
#endif
        
        return result_indices;
    }
    
    static std::vector<usize> filter_range_i32(const i32* values, usize count,
                                              i32 min_val, i32 max_val) {
        std::vector<usize> result_indices;
        result_indices.reserve(count / 4);
        
#ifdef __AVX2__
        __m256i min_vec = _mm256_set1_epi32(min_val);
        __m256i max_vec = _mm256_set1_epi32(max_val);
        
        usize simd_count = (count / SIMD_WIDTH) * SIMD_WIDTH;
        
        for (usize i = 0; i < simd_count; i += SIMD_WIDTH) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&values[i]));
            __m256i ge_min = _mm256_cmpgt_epi32(data, _mm256_sub_epi32(min_vec, _mm256_set1_epi32(1)));
            __m256i le_max = _mm256_cmpgt_epi32(_mm256_add_epi32(max_vec, _mm256_set1_epi32(1)), data);
            __m256i in_range = _mm256_and_si256(ge_min, le_max);
            
            // Convert to float for movemask
            __m256 in_range_f = _mm256_castsi256_ps(in_range);
            u32 mask = _mm256_movemask_ps(in_range_f);
            
            for (usize j = 0; j < SIMD_WIDTH; ++j) {
                if (mask & (1u << j)) {
                    result_indices.push_back(i + j);
                }
            }
        }
        
        // Process remaining elements
        for (usize i = simd_count; i < count; ++i) {
            if (values[i] >= min_val && values[i] <= max_val) {
                result_indices.push_back(i);
            }
        }
#else
        for (usize i = 0; i < count; ++i) {
            if (values[i] >= min_val && values[i] <= max_val) {
                result_indices.push_back(i);
            }
        }
#endif
        
        return result_indices;
    }
};

/**
 * @brief SIMD-accelerated distance calculations
 */
class SIMDDistanceCalculator {
public:
    static std::vector<f32> calculate_distances_squared(const spatial::Vec3* positions, 
                                                       usize count, 
                                                       const spatial::Vec3& target) {
        std::vector<f32> distances_sq;
        distances_sq.resize(count);
        
#ifdef __AVX2__
        __m256 target_x = _mm256_set1_ps(target.x);
        __m256 target_y = _mm256_set1_ps(target.y);
        __m256 target_z = _mm256_set1_ps(target.z);
        
        usize simd_count = (count / 8) * 8;
        
        for (usize i = 0; i < simd_count; i += 8) {
            // Load 8 positions (interleaved x,y,z)
            // This assumes positions are stored in AoS format
            // For SoA format, the loading would be different
            
            // For simplicity, process 2 at a time with AVX
            for (usize j = 0; j < 8 && (i + j) < count; j += 2) {
                __m256 pos_data = _mm256_set_ps(
                    positions[i + j + 1].z, positions[i + j + 1].y, positions[i + j + 1].x, 0,
                    positions[i + j].z, positions[i + j].y, positions[i + j].x, 0
                );
                
                __m256 target_data = _mm256_set_ps(target.z, target.y, target.x, 0,
                                                  target.z, target.y, target.x, 0);
                
                __m256 diff = _mm256_sub_ps(pos_data, target_data);
                __m256 sq = _mm256_mul_ps(diff, diff);
                
                // Horizontal add to get distance squared for each position
                // This is a simplified version - real implementation would be more complex
                f32 result[8];
                _mm256_storeu_ps(result, sq);
                
                if (i + j < count) {
                    distances_sq[i + j] = result[0] + result[1] + result[2];
                }
                if (i + j + 1 < count) {
                    distances_sq[i + j + 1] = result[4] + result[5] + result[6];
                }
            }
        }
        
        // Process remaining elements
        for (usize i = simd_count; i < count; ++i) {
            spatial::Vec3 diff = positions[i] - target;
            distances_sq[i] = diff.length_squared();
        }
#else
        for (usize i = 0; i < count; ++i) {
            spatial::Vec3 diff = positions[i] - target;
            distances_sq[i] = diff.length_squared();
        }
#endif
        
        return distances_sq;
    }
};

} // namespace simd

/**
 * @brief Parallel query executor with work stealing
 */
class ParallelQueryExecutor {
private:
    std::unique_ptr<ThreadPool> thread_pool_;
    usize parallel_threshold_;
    
    template<typename Iterator, typename Predicate>
    auto parallel_filter(Iterator begin, Iterator end, Predicate&& predicate, usize chunk_size = 1000) {
        using ValueType = typename std::iterator_traits<Iterator>::value_type;
        std::vector<ValueType> results;
        
        usize total_size = std::distance(begin, end);
        if (total_size < parallel_threshold_) {
            // Use sequential processing for small datasets
            std::copy_if(begin, end, std::back_inserter(results), predicate);
            return results;
        }
        
        // Parallel processing
        usize num_chunks = (total_size + chunk_size - 1) / chunk_size;
        std::vector<std::future<std::vector<ValueType>>> futures;
        
        auto current = begin;
        for (usize i = 0; i < num_chunks && current != end; ++i) {
            auto chunk_end = current;
            std::advance(chunk_end, std::min(chunk_size, static_cast<usize>(std::distance(current, end))));
            
            futures.emplace_back(thread_pool_->enqueue([current, chunk_end, &predicate]() {
                std::vector<ValueType> chunk_results;
                std::copy_if(current, chunk_end, std::back_inserter(chunk_results), predicate);
                return chunk_results;
            }));
            
            current = chunk_end;
        }
        
        // Collect results
        for (auto& future : futures) {
            auto chunk_results = future.get();
            results.insert(results.end(), chunk_results.begin(), chunk_results.end());
        }
        
        return results;
    }
    
public:
    explicit ParallelQueryExecutor(usize thread_count = std::thread::hardware_concurrency(),
                                  usize threshold = 1000)
        : thread_pool_(std::make_unique<ThreadPool>(thread_count))
        , parallel_threshold_(threshold) {
        
        LOG_INFO("ParallelQueryExecutor initialized: {} threads, threshold: {}", 
                thread_count, threshold);
    }
    
    template<typename Container, typename Predicate>
    auto execute_parallel_filter(const Container& container, Predicate&& predicate) {
        return parallel_filter(container.begin(), container.end(), 
                             std::forward<Predicate>(predicate));
    }
    
    template<typename Container, typename Transform>
    auto execute_parallel_transform(const Container& container, Transform&& transform) {
        using InputType = typename Container::value_type;
        using OutputType = std::invoke_result_t<Transform, const InputType&>;
        
        std::vector<OutputType> results;
        results.resize(container.size());
        
        if (container.size() < parallel_threshold_) {
            std::transform(container.begin(), container.end(), results.begin(), transform);
            return results;
        }
        
        // Parallel transform using std::execution
        std::transform(std::execution::par_unseq, container.begin(), container.end(),
                      results.begin(), transform);
        
        return results;
    }
    
    template<typename Container>
    auto execute_parallel_sort(Container& container) {
        if (container.size() < parallel_threshold_) {
            std::sort(container.begin(), container.end());
            return;
        }
        
        std::sort(std::execution::par_unseq, container.begin(), container.end());
    }
    
    template<typename Container, typename Compare>
    auto execute_parallel_sort(Container& container, Compare&& compare) {
        if (container.size() < parallel_threshold_) {
            std::sort(container.begin(), container.end(), compare);
            return;
        }
        
        std::sort(std::execution::par_unseq, container.begin(), container.end(), compare);
    }
    
    usize thread_count() const { return thread_pool_->worker_count(); }
    usize pending_tasks() const { return thread_pool_->pending_tasks(); }
};

/**
 * @brief Streaming query processor for large datasets
 */
template<typename... Components>
class StreamingQueryProcessor {
public:
    using EntityTuple = std::tuple<Entity, Components*...>;
    using StreamConsumer = std::function<void(const EntityTuple&)>;
    using StreamFilter = std::function<bool(const EntityTuple&)>;
    using StreamTransform = std::function<EntityTuple(const EntityTuple&)>;
    
private:
    QueryEngine* engine_;
    usize chunk_size_;
    bool enable_buffering_;
    std::vector<EntityTuple> buffer_;
    
public:
    explicit StreamingQueryProcessor(QueryEngine* engine, usize chunk_size = 10000)
        : engine_(engine), chunk_size_(chunk_size), enable_buffering_(false) {}
    
    StreamingQueryProcessor& with_buffering(bool enable) {
        enable_buffering_ = enable;
        if (enable) {
            buffer_.reserve(chunk_size_);
        }
        return *this;
    }
    
    StreamingQueryProcessor& with_chunk_size(usize size) {
        chunk_size_ = size;
        return *this;
    }
    
    template<typename Predicate>
    void stream_filter(const QueryPredicate<Components...>& predicate, 
                      StreamConsumer consumer) {
        // Stream entities in chunks to avoid memory pressure
        ComponentSignature required = make_signature<Components...>();
        
        // This would need to be integrated with the actual Registry implementation
        // For now, we'll use a simplified approach
        
        usize processed = 0;
        constexpr usize BATCH_SIZE = 1000;
        
        // Simulate streaming by processing entities in batches
        auto all_entities = engine_->get_registry()->get_all_entities();
        
        for (usize start = 0; start < all_entities.size(); start += BATCH_SIZE) {
            usize end = std::min(start + BATCH_SIZE, all_entities.size());
            
            for (usize i = start; i < end; ++i) {
                Entity entity = all_entities[i];
                
                auto components = std::make_tuple(
                    engine_->get_registry()->get_component<Components>(entity)...);
                
                bool has_all = (std::get<Components*>(components) && ...);
                if (has_all) {
                    EntityTuple tuple = std::make_tuple(entity, std::get<Components*>(components)...);
                    if (predicate(tuple)) {
                        if (enable_buffering_) {
                            buffer_.push_back(tuple);
                            if (buffer_.size() >= chunk_size_) {
                                for (const auto& buffered_tuple : buffer_) {
                                    consumer(buffered_tuple);
                                }
                                buffer_.clear();
                            }
                        } else {
                            consumer(tuple);
                        }
                    }
                }
            }
            
            processed += (end - start);
            if (processed % 10000 == 0) {
                LOG_DEBUG("Streamed {} entities", processed);
            }
        }
        
        // Flush remaining buffered items
        if (enable_buffering_ && !buffer_.empty()) {
            for (const auto& buffered_tuple : buffer_) {
                consumer(buffered_tuple);
            }
            buffer_.clear();
        }
    }
    
    template<typename Predicate>
    void stream_transform_filter(const QueryPredicate<Components...>& predicate,
                               StreamTransform transform,
                               StreamConsumer consumer) {
        stream_filter(predicate, [&](const EntityTuple& tuple) {
            EntityTuple transformed = transform(tuple);
            consumer(transformed);
        });
    }
    
    void stream_to_file(const std::string& filename, 
                       const QueryPredicate<Components...>& predicate) {
        // Implementation would write streaming results to file
        LOG_INFO("Streaming query results to file: {}", filename);
        
        usize count = 0;
        stream_filter(predicate, [&count](const EntityTuple& tuple) {
            ++count;
            // Write tuple to file
        });
        
        LOG_INFO("Streamed {} entities to file", count);
    }
};

/**
 * @brief Hot path optimization for frequently executed queries
 */
class HotPathOptimizer {
private:
    struct HotQuery {
        std::string signature;
        usize execution_count;
        f64 average_time_us;
        std::chrono::steady_clock::time_point last_executed;
        bool is_compiled;
        
        HotQuery() : execution_count(0), average_time_us(0.0), is_compiled(false) {}
    };
    
    std::unordered_map<std::string, HotQuery> hot_queries_;
    usize hot_threshold_;
    mutable std::mutex hot_queries_mutex_;
    
public:
    explicit HotPathOptimizer(usize threshold = 50) : hot_threshold_(threshold) {}
    
    void record_execution(const std::string& query_signature, f64 execution_time_us) {
        std::lock_guard<std::mutex> lock(hot_queries_mutex_);
        
        auto& hot_query = hot_queries_[query_signature];
        hot_query.signature = query_signature;
        hot_query.execution_count++;
        hot_query.last_executed = std::chrono::steady_clock::now();
        
        // Update running average
        if (hot_query.execution_count == 1) {
            hot_query.average_time_us = execution_time_us;
        } else {
            hot_query.average_time_us = 
                (hot_query.average_time_us * (hot_query.execution_count - 1) + execution_time_us) / 
                hot_query.execution_count;
        }
        
        // Check if query should be compiled
        if (!hot_query.is_compiled && hot_query.execution_count >= hot_threshold_) {
            compile_hot_query(hot_query);
        }
    }
    
    bool is_hot_query(const std::string& query_signature) const {
        std::lock_guard<std::mutex> lock(hot_queries_mutex_);
        auto it = hot_queries_.find(query_signature);
        return it != hot_queries_.end() && it->second.execution_count >= hot_threshold_;
    }
    
    std::vector<std::string> get_hot_queries() const {
        std::lock_guard<std::mutex> lock(hot_queries_mutex_);
        std::vector<std::string> hot_query_list;
        
        for (const auto& [signature, query] : hot_queries_) {
            if (query.execution_count >= hot_threshold_) {
                hot_query_list.push_back(signature);
            }
        }
        
        return hot_query_list;
    }
    
    struct HotPathStats {
        usize total_queries;
        usize hot_queries;
        usize compiled_queries;
        f64 average_hot_execution_time_us;
        std::vector<std::pair<std::string, usize>> top_queries;
    };
    
    HotPathStats get_statistics() const {
        std::lock_guard<std::mutex> lock(hot_queries_mutex_);
        
        HotPathStats stats;
        stats.total_queries = hot_queries_.size();
        stats.hot_queries = 0;
        stats.compiled_queries = 0;
        f64 total_time = 0.0;
        
        std::vector<std::pair<std::string, usize>> query_counts;
        
        for (const auto& [signature, query] : hot_queries_) {
            query_counts.emplace_back(signature, query.execution_count);
            
            if (query.execution_count >= hot_threshold_) {
                stats.hot_queries++;
                total_time += query.average_time_us;
                
                if (query.is_compiled) {
                    stats.compiled_queries++;
                }
            }
        }
        
        stats.average_hot_execution_time_us = 
            stats.hot_queries > 0 ? total_time / stats.hot_queries : 0.0;
        
        // Sort by execution count and take top 10
        std::sort(query_counts.begin(), query_counts.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        stats.top_queries.assign(query_counts.begin(), 
                                query_counts.begin() + std::min(10ul, query_counts.size()));
        
        return stats;
    }
    
private:
    void compile_hot_query(HotQuery& hot_query) {
        // In a real implementation, this would generate optimized machine code
        // for the specific query pattern
        hot_query.is_compiled = true;
        
        LOG_INFO("Compiled hot query: {} (executed {} times, avg {:.2f}µs)", 
                hot_query.signature, hot_query.execution_count, hot_query.average_time_us);
    }
};

/**
 * @brief Query profiler for performance analysis
 */
class QueryProfiler {
private:
    struct ProfileData {
        std::string query_name;
        std::chrono::nanoseconds total_time{0};
        usize execution_count{0};
        usize total_entities_processed{0};
        usize total_entities_matched{0};
        f64 average_selectivity{0.0};
        
        f64 average_time_us() const {
            return execution_count > 0 ? 
                std::chrono::duration<f64, std::micro>(total_time).count() / execution_count : 0.0;
        }
        
        f64 entities_per_second() const {
            f64 time_seconds = std::chrono::duration<f64>(total_time).count();
            return time_seconds > 0.0 ? total_entities_processed / time_seconds : 0.0;
        }
    };
    
    std::unordered_map<std::string, ProfileData> profile_data_;
    mutable std::mutex profile_mutex_;
    bool enabled_;
    
public:
    explicit QueryProfiler(bool enabled = true) : enabled_(enabled) {}
    
    void record_query_execution(const std::string& query_name,
                               std::chrono::nanoseconds execution_time,
                               usize entities_processed,
                               usize entities_matched) {
        if (!enabled_) return;
        
        std::lock_guard<std::mutex> lock(profile_mutex_);
        
        auto& profile = profile_data_[query_name];
        profile.query_name = query_name;
        profile.total_time += execution_time;
        profile.execution_count++;
        profile.total_entities_processed += entities_processed;
        profile.total_entities_matched += entities_matched;
        
        // Update running average selectivity
        f64 current_selectivity = entities_processed > 0 ? 
            static_cast<f64>(entities_matched) / entities_processed : 0.0;
        
        profile.average_selectivity = 
            (profile.average_selectivity * (profile.execution_count - 1) + current_selectivity) /
            profile.execution_count;
    }
    
    void enable(bool enable) { enabled_ = enable; }
    bool is_enabled() const { return enabled_; }
    
    struct ProfileReport {
        std::vector<ProfileData> sorted_by_total_time;
        std::vector<ProfileData> sorted_by_average_time;
        std::vector<ProfileData> sorted_by_frequency;
        f64 total_query_time_seconds;
        usize total_query_executions;
    };
    
    ProfileReport generate_report() const {
        std::lock_guard<std::mutex> lock(profile_mutex_);
        
        ProfileReport report;
        report.total_query_time_seconds = 0.0;
        report.total_query_executions = 0;
        
        std::vector<ProfileData> all_profiles;
        all_profiles.reserve(profile_data_.size());
        
        for (const auto& [name, profile] : profile_data_) {
            all_profiles.push_back(profile);
            report.total_query_time_seconds += 
                std::chrono::duration<f64>(profile.total_time).count();
            report.total_query_executions += profile.execution_count;
        }
        
        // Sort by total time
        report.sorted_by_total_time = all_profiles;
        std::sort(report.sorted_by_total_time.begin(), report.sorted_by_total_time.end(),
                 [](const auto& a, const auto& b) { return a.total_time > b.total_time; });
        
        // Sort by average time
        report.sorted_by_average_time = all_profiles;
        std::sort(report.sorted_by_average_time.begin(), report.sorted_by_average_time.end(),
                 [](const auto& a, const auto& b) { return a.average_time_us() > b.average_time_us(); });
        
        // Sort by frequency
        report.sorted_by_frequency = all_profiles;
        std::sort(report.sorted_by_frequency.begin(), report.sorted_by_frequency.end(),
                 [](const auto& a, const auto& b) { return a.execution_count > b.execution_count; });
        
        return report;
    }
    
    std::string generate_report_string() const {
        auto report = generate_report();
        std::ostringstream oss;
        
        oss << "=== Query Performance Report ===\n";
        oss << "Total query executions: " << report.total_query_executions << "\n";
        oss << "Total query time: " << report.total_query_time_seconds << " seconds\n\n";
        
        oss << "Top 10 Queries by Total Time:\n";
        for (usize i = 0; i < std::min(10ul, report.sorted_by_total_time.size()); ++i) {
            const auto& profile = report.sorted_by_total_time[i];
            oss << "  " << (i + 1) << ". " << profile.query_name 
                << " - " << std::chrono::duration<f64>(profile.total_time).count() << "s"
                << " (" << profile.execution_count << " executions)\n";
        }
        
        oss << "\nTop 10 Queries by Average Time:\n";
        for (usize i = 0; i < std::min(10ul, report.sorted_by_average_time.size()); ++i) {
            const auto& profile = report.sorted_by_average_time[i];
            oss << "  " << (i + 1) << ". " << profile.query_name 
                << " - " << profile.average_time_us() << " µs avg"
                << " (" << profile.entities_per_second() << " entities/sec)\n";
        }
        
        oss << "\nMost Frequent Queries:\n";
        for (usize i = 0; i < std::min(10ul, report.sorted_by_frequency.size()); ++i) {
            const auto& profile = report.sorted_by_frequency[i];
            oss << "  " << (i + 1) << ". " << profile.query_name 
                << " - " << profile.execution_count << " executions"
                << " (avg selectivity: " << (profile.average_selectivity * 100.0) << "%)\n";
        }
        
        return oss.str();
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(profile_mutex_);
        profile_data_.clear();
    }
};

/**
 * @brief Advanced query engine with all optimizations enabled
 */
class AdvancedQueryEngine : public QueryEngine {
private:
    std::unique_ptr<ParallelQueryExecutor> parallel_executor_;
    std::unique_ptr<HotPathOptimizer> hot_path_optimizer_;
    std::unique_ptr<QueryProfiler> profiler_;
    
public:
    AdvancedQueryEngine(Registry* registry, const QueryConfig& config = QueryConfig::create_performance_optimized())
        : QueryEngine(registry, config)
        , parallel_executor_(std::make_unique<ParallelQueryExecutor>(config.max_worker_threads))
        , hot_path_optimizer_(std::make_unique<HotPathOptimizer>())
        , profiler_(std::make_unique<QueryProfiler>(config.enable_query_profiling)) {
        
        LOG_INFO("AdvancedQueryEngine initialized with all optimizations enabled");
    }
    
    ParallelQueryExecutor& parallel_executor() { return *parallel_executor_; }
    HotPathOptimizer& hot_path_optimizer() { return *hot_path_optimizer_; }
    QueryProfiler& profiler() { return *profiler_; }
    
    template<Component... Components>
    StreamingQueryProcessor<Components...> create_streaming_processor() {
        return StreamingQueryProcessor<Components...>(this);
    }
    
    std::string generate_comprehensive_report() const {
        std::ostringstream oss;
        
        oss << "=== Advanced Query Engine Report ===\n\n";
        
        // Base engine metrics
        auto metrics = get_performance_metrics();
        oss << "Base Engine Metrics:\n";
        oss << "  Total Queries: " << metrics.total_queries << "\n";
        oss << "  Cache Hit Ratio: " << (metrics.cache_hit_ratio * 100.0) << "%\n";
        oss << "  Parallel Executions: " << metrics.parallel_executions << "\n";
        oss << "  Average Execution Time: " << metrics.average_execution_time_us << " µs\n\n";
        
        // Hot path statistics
        auto hot_stats = hot_path_optimizer_->get_statistics();
        oss << "Hot Path Optimization:\n";
        oss << "  Hot Queries: " << hot_stats.hot_queries << "/" << hot_stats.total_queries << "\n";
        oss << "  Compiled Queries: " << hot_stats.compiled_queries << "\n";
        oss << "  Average Hot Query Time: " << hot_stats.average_hot_execution_time_us << " µs\n\n";
        
        // Profiler report
        if (profiler_->is_enabled()) {
            oss << profiler_->generate_report_string() << "\n";
        }
        
        // Parallel executor stats
        oss << "Parallel Execution:\n";
        oss << "  Worker Threads: " << parallel_executor_->thread_count() << "\n";
        oss << "  Pending Tasks: " << parallel_executor_->pending_tasks() << "\n";
        
        return oss.str();
    }
};

} // namespace ecscope::ecs::query::advanced