#pragma once

#include "plugin_interface.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <thread>

namespace ecscope::plugins {

    /**
     * @brief Message priority levels
     */
    enum class MessagePriority {
        Critical = 0,  // System-critical messages
        High = 1,      // Important messages
        Normal = 2,    // Regular messages
        Low = 3        // Low-priority messages
    };
    
    /**
     * @brief Message delivery modes
     */
    enum class DeliveryMode {
        Synchronous,    // Block until message is processed
        Asynchronous,   // Queue message and return immediately
        Broadcast,      // Send to all subscribed plugins
        Reliable        // Ensure delivery with retries
    };
    
    /**
     * @brief Plugin message structure
     */
    struct PluginMessage {
        std::string id;
        std::string sender;
        std::string recipient;
        std::string type;
        std::string content;
        std::map<std::string, std::string> parameters;
        MessagePriority priority{MessagePriority::Normal};
        DeliveryMode delivery_mode{DeliveryMode::Asynchronous};
        uint64_t timestamp{0};
        uint32_t retry_count{0};
        uint32_t max_retries{3};
        
        PluginMessage() = default;
        PluginMessage(const std::string& sender_name, const std::string& recipient_name,
                     const std::string& message_type, const std::string& message_content)
            : sender(sender_name), recipient(recipient_name), type(message_type), content(message_content) {
            generate_id();
        }
        
        void generate_id();
        std::string serialize() const;
        bool deserialize(const std::string& data);
    };
    
    /**
     * @brief Message handler function signature
     */
    using MessageHandler = std::function<std::string(const PluginMessage&)>;
    
    /**
     * @brief Event subscription information
     */
    struct EventSubscription {
        std::string plugin_name;
        std::string event_pattern;
        std::function<void(const std::string&, const std::map<std::string, std::string>&)> callback;
        MessagePriority min_priority{MessagePriority::Low};
        bool active{true};
        uint64_t subscription_id{0};
    };
    
    /**
     * @brief Plugin event structure
     */
    struct PluginEvent {
        std::string id;
        std::string name;
        std::string source;
        std::map<std::string, std::string> data;
        MessagePriority priority{MessagePriority::Normal};
        uint64_t timestamp{0};
        std::vector<std::string> targets; // Empty for broadcast
        
        PluginEvent() = default;
        PluginEvent(const std::string& event_name, const std::string& source_plugin)
            : name(event_name), source(source_plugin) {
            generate_id();
        }
        
        void generate_id();
    };
    
    /**
     * @brief Message router for inter-plugin communication
     */
    class MessageRouter {
    public:
        MessageRouter();
        ~MessageRouter();
        
        // Lifecycle
        bool initialize();
        void shutdown();
        bool is_running() const { return running_; }
        
        // Plugin registration
        void register_plugin(const std::string& plugin_name);
        void unregister_plugin(const std::string& plugin_name);
        bool is_plugin_registered(const std::string& plugin_name) const;
        std::vector<std::string> get_registered_plugins() const;
        
        // Message handlers
        void set_message_handler(const std::string& plugin_name, const std::string& message_type, MessageHandler handler);
        void remove_message_handler(const std::string& plugin_name, const std::string& message_type);
        bool has_message_handler(const std::string& plugin_name, const std::string& message_type) const;
        
        // Message sending
        bool send_message(const PluginMessage& message);
        bool send_message(const std::string& sender, const std::string& recipient, 
                         const std::string& type, const std::string& content,
                         const std::map<std::string, std::string>& params = {},
                         MessagePriority priority = MessagePriority::Normal,
                         DeliveryMode mode = DeliveryMode::Asynchronous);
        
        // Broadcast messaging
        void broadcast_message(const std::string& sender, const std::string& type, 
                              const std::string& content, 
                              const std::map<std::string, std::string>& params = {},
                              MessagePriority priority = MessagePriority::Normal);
        
        // Synchronous messaging
        std::string send_sync_message(const std::string& sender, const std::string& recipient,
                                     const std::string& type, const std::string& content,
                                     const std::map<std::string, std::string>& params = {},
                                     uint32_t timeout_ms = 5000);
        
        // Message querying
        std::vector<PluginMessage> get_pending_messages(const std::string& plugin_name) const;
        size_t get_message_queue_size(const std::string& plugin_name) const;
        void clear_message_queue(const std::string& plugin_name);
        
        // Configuration
        void set_max_queue_size(size_t max_size) { max_queue_size_ = max_size; }
        void set_message_timeout(uint32_t timeout_ms) { message_timeout_ms_ = timeout_ms; }
        void set_max_retries(uint32_t retries) { max_retries_ = retries; }
        void set_thread_count(size_t count);
        
        // Statistics
        struct Statistics {
            uint64_t messages_sent{0};
            uint64_t messages_delivered{0};
            uint64_t messages_failed{0};
            uint64_t broadcasts_sent{0};
            uint64_t sync_messages{0};
            uint64_t total_queue_size{0};
            uint64_t average_processing_time_us{0};
        };
        
        Statistics get_statistics() const;
        void reset_statistics();
        
    private:
        // Message processing
        void message_processing_thread();
        bool process_message(const PluginMessage& message);
        void retry_failed_message(const PluginMessage& message);
        
        // Message storage
        std::unordered_map<std::string, std::queue<PluginMessage>> message_queues_;
        std::unordered_map<std::string, std::unordered_map<std::string, MessageHandler>> message_handlers_;
        std::unordered_set<std::string> registered_plugins_;
        
        // Synchronization
        mutable std::shared_mutex router_mutex_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        
        // Threading
        std::vector<std::thread> processing_threads_;
        std::atomic<bool> running_{false};
        size_t thread_count_{2};
        
        // Configuration
        size_t max_queue_size_{1000};
        uint32_t message_timeout_ms_{5000};
        uint32_t max_retries_{3};
        
        // Statistics
        mutable Statistics stats_;
        mutable std::mutex stats_mutex_;
        
        // Retry mechanism
        std::queue<PluginMessage> retry_queue_;
        std::mutex retry_mutex_;
        std::thread retry_thread_;
        void retry_processing_thread();
    };
    
    /**
     * @brief Event system for plugin notifications
     */
    class EventSystem {
    public:
        EventSystem();
        ~EventSystem();
        
        // Lifecycle
        bool initialize();
        void shutdown();
        bool is_running() const { return running_; }
        
        // Event subscription
        uint64_t subscribe(const std::string& plugin_name, const std::string& event_pattern,
                          std::function<void(const std::string&, const std::map<std::string, std::string>&)> callback,
                          MessagePriority min_priority = MessagePriority::Low);
        void unsubscribe(uint64_t subscription_id);
        void unsubscribe_plugin(const std::string& plugin_name);
        size_t get_subscription_count(const std::string& plugin_name = "") const;
        
        // Event emission
        void emit_event(const PluginEvent& event);
        void emit_event(const std::string& event_name, const std::string& source_plugin,
                       const std::map<std::string, std::string>& data = {},
                       MessagePriority priority = MessagePriority::Normal,
                       const std::vector<std::string>& targets = {});
        
        // Event querying
        std::vector<PluginEvent> get_recent_events(size_t count = 100) const;
        std::vector<PluginEvent> get_events_for_plugin(const std::string& plugin_name, size_t count = 100) const;
        void clear_event_history();
        
        // Event patterns
        bool matches_pattern(const std::string& event_name, const std::string& pattern) const;
        std::vector<std::string> get_matching_subscriptions(const std::string& event_name) const;
        
        // Configuration
        void set_max_history_size(size_t size) { max_history_size_ = size; }
        void set_processing_threads(size_t count);
        void set_event_timeout(uint32_t timeout_ms) { event_timeout_ms_ = timeout_ms; }
        
        // Statistics
        struct Statistics {
            uint64_t events_emitted{0};
            uint64_t events_delivered{0};
            uint64_t subscriptions_active{0};
            uint64_t pattern_matches{0};
            uint64_t processing_time_total_us{0};
            uint64_t average_processing_time_us{0};
        };
        
        Statistics get_statistics() const;
        void reset_statistics();
        
    private:
        // Event processing
        void event_processing_thread();
        void process_event(const PluginEvent& event);
        void deliver_to_subscribers(const PluginEvent& event);
        
        // Subscription management
        std::unordered_map<uint64_t, EventSubscription> subscriptions_;
        std::unordered_map<std::string, std::vector<uint64_t>> plugin_subscriptions_;
        std::atomic<uint64_t> next_subscription_id_{1};
        
        // Event storage
        std::queue<PluginEvent> event_queue_;
        std::vector<PluginEvent> event_history_;
        size_t max_history_size_{1000};
        
        // Synchronization
        mutable std::shared_mutex event_mutex_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        
        // Threading
        std::vector<std::thread> processing_threads_;
        std::atomic<bool> running_{false};
        size_t thread_count_{1};
        
        // Configuration
        uint32_t event_timeout_ms_{1000};
        
        // Statistics
        mutable Statistics stats_;
        mutable std::mutex stats_mutex_;
        
        // Pattern matching helpers
        bool is_glob_pattern(const std::string& pattern) const;
        bool match_glob(const std::string& text, const std::string& pattern) const;
        bool is_regex_pattern(const std::string& pattern) const;
        bool match_regex(const std::string& text, const std::string& pattern) const;
    };
    
    /**
     * @brief Main plugin messaging system
     */
    class PluginMessaging {
    public:
        PluginMessaging();
        ~PluginMessaging();
        
        // Lifecycle
        bool initialize();
        void shutdown();
        bool is_running() const;
        
        // Plugin registration
        void register_plugin(const std::string& plugin_name);
        void unregister_plugin(const std::string& plugin_name);
        bool is_plugin_registered(const std::string& plugin_name) const;
        
        // Message routing
        MessageRouter* get_message_router() { return message_router_.get(); }
        const MessageRouter* get_message_router() const { return message_router_.get(); }
        
        // Event system
        EventSystem* get_event_system() { return event_system_.get(); }
        const EventSystem* get_event_system() const { return event_system_.get(); }
        
        // High-level messaging API
        bool send_message(const std::string& sender, const std::string& recipient,
                         const std::string& message, const std::map<std::string, std::string>& params = {});
        void broadcast_message(const std::string& sender, const std::string& message,
                              const std::map<std::string, std::string>& params = {});
        void emit_event(const std::string& event_name, const std::string& source,
                       const std::map<std::string, std::string>& data = {});
        
        // Plugin context integration
        void setup_plugin_messaging(const std::string& plugin_name, PluginContext* context);
        void cleanup_plugin_messaging(const std::string& plugin_name);
        
        // Security integration
        void set_security_check(std::function<bool(const std::string&, const std::string&)> check);
        bool check_communication_allowed(const std::string& sender, const std::string& recipient) const;
        
        // Statistics and monitoring
        struct CombinedStatistics {
            MessageRouter::Statistics routing;
            EventSystem::Statistics events;
            uint64_t total_plugins{0};
            uint64_t active_connections{0};
        };
        
        CombinedStatistics get_statistics() const;
        std::string generate_report() const;
        
    private:
        std::unique_ptr<MessageRouter> message_router_;
        std::unique_ptr<EventSystem> event_system_;
        std::function<bool(const std::string&, const std::string&)> security_check_;
        
        bool initialized_{false};
    };
    
} // namespace ecscope::plugins