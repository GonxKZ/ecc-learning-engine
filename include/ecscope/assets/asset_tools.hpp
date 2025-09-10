#pragma once

#include "asset.hpp"
#include "processors/asset_processor.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace ecscope::assets::tools {

    // Asset build configuration
    struct BuildConfiguration {
        std::string name = "default";
        std::string output_directory = "build/assets/";
        std::string cache_directory = "build/cache/";
        
        // Platform settings
        std::string target_platform = "pc";
        QualityLevel default_quality = QualityLevel::HIGH;
        bool enable_compression = true;
        bool enable_optimization = true;
        bool generate_debug_info = false;
        
        // Processing settings
        std::unordered_map<AssetType, processors::ProcessingOptions> type_specific_options;
        std::vector<std::string> excluded_patterns;
        std::vector<std::string> force_rebuild_patterns;
        
        // Packaging settings
        bool create_asset_bundles = true;
        size_t max_bundle_size_mb = 100;
        bool compress_bundles = true;
        
        // Validation settings
        bool strict_validation = true;
        bool fail_on_warnings = false;
    };

    // Asset build result
    struct BuildResult {
        bool success = false;
        std::string error_message;
        
        size_t assets_processed = 0;
        size_t assets_succeeded = 0;
        size_t assets_failed = 0;
        size_t assets_skipped = 0;
        
        size_t total_input_bytes = 0;
        size_t total_output_bytes = 0;
        std::chrono::milliseconds total_build_time{0};
        
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        std::unordered_map<std::string, std::string> file_mappings; // source -> output
        
        float get_compression_ratio() const {
            return total_input_bytes > 0 ? 
                   static_cast<float>(total_output_bytes) / total_input_bytes : 1.0f;
        }
        
        float get_success_rate() const {
            return assets_processed > 0 ? 
                   static_cast<float>(assets_succeeded) / assets_processed : 0.0f;
        }
    };

    // Asset builder - builds assets for deployment
    class AssetBuilder {
    public:
        AssetBuilder();
        ~AssetBuilder();
        
        // Configuration
        void set_build_configuration(const BuildConfiguration& config);
        const BuildConfiguration& get_build_configuration() const { return m_config; }
        
        void add_source_directory(const std::string& directory, bool recursive = true);
        void remove_source_directory(const std::string& directory);
        void clear_source_directories();
        
        // Asset discovery
        std::vector<std::string> discover_assets() const;
        std::vector<std::string> filter_assets(const std::vector<std::string>& assets) const;
        bool should_rebuild_asset(const std::string& source_path) const;
        
        // Building
        BuildResult build_all();
        BuildResult build_incremental();
        BuildResult build_assets(const std::vector<std::string>& asset_paths);
        BuildResult build_asset(const std::string& asset_path);
        
        // Cleaning
        void clean_build_directory();
        void clean_cache_directory();
        void clean_all();
        
        // Progress callbacks
        using ProgressCallback = std::function<void(const std::string&, float)>;
        void set_progress_callback(ProgressCallback callback) { m_progress_callback = callback; }
        
        using LogCallback = std::function<void(const std::string&, int)>; // message, level
        void set_log_callback(LogCallback callback) { m_log_callback = callback; }
        
        // Statistics
        BuildResult get_last_build_result() const { return m_last_result; }
        void reset_statistics();
        
        // Dependency tracking
        void enable_dependency_tracking(bool enable) { m_dependency_tracking_enabled = enable; }
        bool is_dependency_tracking_enabled() const { return m_dependency_tracking_enabled; }
        
        std::vector<std::string> get_asset_dependencies(const std::string& asset_path) const;
        void invalidate_dependents(const std::string& asset_path);
        
    private:
        BuildConfiguration m_config;
        std::vector<std::string> m_source_directories;
        std::unique_ptr<processors::ProcessingPipeline> m_processing_pipeline;
        
        // Callbacks
        ProgressCallback m_progress_callback;
        LogCallback m_log_callback;
        
        // Build state
        BuildResult m_last_result;
        std::unordered_map<std::string, std::chrono::system_clock::time_point> m_file_timestamps;
        std::unordered_map<std::string, std::vector<std::string>> m_dependencies;
        bool m_dependency_tracking_enabled = true;
        
        // Internal methods
        BuildResult build_assets_internal(const std::vector<std::string>& asset_paths);
        bool process_asset(const std::string& source_path, const std::string& output_path);
        std::string get_output_path(const std::string& source_path) const;
        std::string get_cache_path(const std::string& source_path) const;
        
        void report_progress(const std::string& message, float progress);
        void log_message(const std::string& message, int level);
        
        bool is_file_newer(const std::string& source, const std::string& target) const;
        void update_file_timestamp(const std::string& file_path);
        void load_dependency_cache();
        void save_dependency_cache();
    };

    // Asset packer - creates asset bundles
    class AssetPacker {
    public:
        AssetPacker();
        ~AssetPacker();
        
        // Bundle configuration
        struct BundleConfig {
            std::string name;
            std::vector<std::string> asset_patterns;
            size_t max_size_mb = 100;
            bool compress = true;
            int compression_level = 6; // 0-9 for zlib
            std::string output_path;
        };
        
        void add_bundle_config(const BundleConfig& config);
        void remove_bundle_config(const std::string& name);
        void clear_bundle_configs();
        
        // Packing operations
        struct PackResult {
            bool success = false;
            std::string error_message;
            std::vector<std::string> created_bundles;
            size_t total_assets_packed = 0;
            size_t total_bundles_created = 0;
            size_t uncompressed_size = 0;
            size_t compressed_size = 0;
            std::chrono::milliseconds pack_time{0};
        };
        
        PackResult pack_all();
        PackResult pack_bundle(const std::string& bundle_name);
        PackResult pack_assets(const std::vector<std::string>& asset_paths, 
                              const std::string& output_bundle);
        
        // Bundle management
        std::vector<std::string> get_bundle_names() const;
        BundleConfig get_bundle_config(const std::string& name) const;
        
        // Validation
        bool validate_bundle(const std::string& bundle_path) const;
        std::vector<std::string> list_bundle_contents(const std::string& bundle_path) const;
        
    private:
        std::unordered_map<std::string, BundleConfig> m_bundle_configs;
        
        PackResult pack_bundle_internal(const BundleConfig& config);
        std::vector<std::string> resolve_asset_patterns(const std::vector<std::string>& patterns) const;
    };

    // Asset validator - validates asset integrity and compatibility
    class AssetValidator {
    public:
        AssetValidator();
        ~AssetValidator();
        
        // Validation result
        struct ValidationResult {
            enum class Severity {
                INFO,
                WARNING,
                ERROR,
                CRITICAL
            };
            
            struct Issue {
                std::string file_path;
                std::string message;
                Severity severity;
                std::string category;
                int line = -1;
                int column = -1;
            };
            
            bool passed = true;
            std::vector<Issue> issues;
            size_t files_validated = 0;
            std::chrono::milliseconds validation_time{0};
            
            size_t get_error_count() const;
            size_t get_warning_count() const;
            bool has_critical_errors() const;
        };
        
        // Validation operations
        ValidationResult validate_asset(const std::string& asset_path) const;
        ValidationResult validate_assets(const std::vector<std::string>& asset_paths) const;
        ValidationResult validate_directory(const std::string& directory, bool recursive = true) const;
        ValidationResult validate_bundle(const std::string& bundle_path) const;
        
        // Validation rules
        void enable_rule(const std::string& rule_name, bool enable = true);
        void disable_rule(const std::string& rule_name);
        bool is_rule_enabled(const std::string& rule_name) const;
        std::vector<std::string> get_available_rules() const;
        
        // Custom validators
        using CustomValidator = std::function<ValidationResult(const std::string&)>;
        void register_custom_validator(const std::string& name, CustomValidator validator);
        void unregister_custom_validator(const std::string& name);
        
        // Configuration
        void set_strict_mode(bool strict) { m_strict_mode = strict; }
        bool is_strict_mode() const { return m_strict_mode; }
        
        void set_max_file_size_mb(size_t max_size) { m_max_file_size_bytes = max_size * 1024 * 1024; }
        size_t get_max_file_size_mb() const { return m_max_file_size_bytes / 1024 / 1024; }
        
    private:
        std::unordered_map<std::string, bool> m_enabled_rules;
        std::unordered_map<std::string, CustomValidator> m_custom_validators;
        bool m_strict_mode = false;
        size_t m_max_file_size_bytes = 100 * 1024 * 1024; // 100MB
        
        // Built-in validation rules
        ValidationResult validate_file_size(const std::string& asset_path) const;
        ValidationResult validate_file_format(const std::string& asset_path) const;
        ValidationResult validate_asset_metadata(const std::string& asset_path) const;
        ValidationResult validate_dependencies(const std::string& asset_path) const;
        ValidationResult validate_naming_convention(const std::string& asset_path) const;
        ValidationResult validate_texture_properties(const std::string& asset_path) const;
        ValidationResult validate_audio_properties(const std::string& asset_path) const;
        ValidationResult validate_mesh_properties(const std::string& asset_path) const;
        
        void add_issue(ValidationResult& result, const std::string& file_path,
                      const std::string& message, ValidationResult::Severity severity,
                      const std::string& category) const;
    };

    // Asset optimizer - optimizes assets for performance and size
    class AssetOptimizer {
    public:
        AssetOptimizer();
        ~AssetOptimizer();
        
        // Optimization result
        struct OptimizationResult {
            bool success = false;
            std::string error_message;
            
            size_t original_size = 0;
            size_t optimized_size = 0;
            std::chrono::milliseconds optimization_time{0};
            
            std::unordered_map<std::string, std::string> optimizations_applied;
            std::vector<std::string> warnings;
            
            float get_size_reduction() const {
                return original_size > 0 ? 
                       1.0f - (static_cast<float>(optimized_size) / original_size) : 0.0f;
            }
        };
        
        // Optimization operations
        OptimizationResult optimize_asset(const std::string& input_path, 
                                        const std::string& output_path) const;
        OptimizationResult optimize_assets(const std::vector<std::string>& asset_paths,
                                         const std::string& output_directory) const;
        
        // Optimization settings
        void set_aggressive_optimization(bool aggressive) { m_aggressive_optimization = aggressive; }
        bool is_aggressive_optimization() const { return m_aggressive_optimization; }
        
        void set_preserve_quality(bool preserve) { m_preserve_quality = preserve; }
        bool is_preserve_quality() const { return m_preserve_quality; }
        
        void set_target_platform(const std::string& platform) { m_target_platform = platform; }
        const std::string& get_target_platform() const { return m_target_platform; }
        
        // Optimization profiles
        struct OptimizationProfile {
            std::string name;
            std::unordered_map<AssetType, processors::ProcessingOptions> type_options;
            bool aggressive = false;
            bool preserve_quality = true;
        };
        
        void add_optimization_profile(const OptimizationProfile& profile);
        void remove_optimization_profile(const std::string& name);
        void set_active_profile(const std::string& name);
        std::string get_active_profile() const { return m_active_profile; }
        
    private:
        bool m_aggressive_optimization = false;
        bool m_preserve_quality = true;
        std::string m_target_platform = "pc";
        std::string m_active_profile = "default";
        std::unordered_map<std::string, OptimizationProfile> m_profiles;
        
        OptimizationResult optimize_texture(const std::string& input_path,
                                          const std::string& output_path) const;
        OptimizationResult optimize_mesh(const std::string& input_path,
                                       const std::string& output_path) const;
        OptimizationResult optimize_audio(const std::string& input_path,
                                        const std::string& output_path) const;
        OptimizationResult optimize_shader(const std::string& input_path,
                                         const std::string& output_path) const;
    };

    // Asset database tool - manages asset database operations
    class AssetDatabaseTool {
    public:
        AssetDatabaseTool();
        ~AssetDatabaseTool();
        
        // Database operations
        bool create_database(const std::string& db_path);
        bool open_database(const std::string& db_path);
        void close_database();
        bool is_database_open() const;
        
        // Asset indexing
        bool index_directory(const std::string& directory, bool recursive = true);
        bool index_asset(const std::string& asset_path);
        bool remove_asset_from_index(const std::string& asset_path);
        
        // Database queries
        std::vector<std::string> find_assets_by_type(AssetType type) const;
        std::vector<std::string> find_assets_by_pattern(const std::string& pattern) const;
        std::vector<std::string> find_assets_by_tag(const std::string& tag) const;
        std::vector<std::string> find_duplicate_assets() const;
        std::vector<std::string> find_orphaned_assets() const;
        
        // Database maintenance
        bool vacuum_database();
        bool optimize_database();
        bool backup_database(const std::string& backup_path);
        bool restore_database(const std::string& backup_path);
        bool verify_database_integrity();
        
        // Statistics
        struct DatabaseStatistics {
            size_t total_assets = 0;
            size_t total_size_bytes = 0;
            std::unordered_map<AssetType, size_t> assets_by_type;
            std::chrono::system_clock::time_point last_update;
        };
        
        DatabaseStatistics get_database_statistics() const;
        
    private:
        class DatabaseImpl;
        std::unique_ptr<DatabaseImpl> m_impl;
    };

    // Command-line tool interface
    class AssetToolCLI {
    public:
        AssetToolCLI();
        ~AssetToolCLI();
        
        // Command registration
        using CommandHandler = std::function<int(const std::vector<std::string>&)>;
        void register_command(const std::string& name, const std::string& description,
                            CommandHandler handler);
        
        // Built-in commands
        void register_built_in_commands();
        
        // Execution
        int execute(int argc, char* argv[]);
        int execute_command(const std::string& command, const std::vector<std::string>& args);
        
        // Help system
        void print_help() const;
        void print_command_help(const std::string& command) const;
        
    private:
        struct Command {
            std::string name;
            std::string description;
            CommandHandler handler;
        };
        
        std::unordered_map<std::string, Command> m_commands;
        
        // Built-in command implementations
        int cmd_build(const std::vector<std::string>& args);
        int cmd_pack(const std::vector<std::string>& args);
        int cmd_validate(const std::vector<std::string>& args);
        int cmd_optimize(const std::vector<std::string>& args);
        int cmd_info(const std::vector<std::string>& args);
        int cmd_convert(const std::vector<std::string>& args);
        int cmd_database(const std::vector<std::string>& args);
        
        // Utility methods
        std::vector<std::string> parse_arguments(const std::string& arg_string) const;
        void print_error(const std::string& message) const;
        void print_warning(const std::string& message) const;
        void print_info(const std::string& message) const;
    };

} // namespace ecscope::assets::tools