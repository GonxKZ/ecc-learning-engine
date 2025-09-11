#pragma once

/**
 * @file ai.hpp
 * @brief Main AI/ML Framework Header - Production-Grade AI System for ECScope Engine
 * 
 * This is the main entry point for ECScope's comprehensive AI/ML framework.
 * It provides modern game AI, machine learning integration, procedural content
 * generation, and performance optimization systems designed for professional
 * game development.
 * 
 * Features:
 * - Behavior AI: FSM, Behavior Trees, GOAP, Utility AI
 * - Machine Learning: Neural Networks, RL, Genetic Algorithms
 * - Procedural Content: Noise, L-systems, WFC, Cellular Automata
 * - Performance Optimization: ML-based system tuning and prediction
 * - Deep ECS Integration: AI Components and Systems
 * - Multi-threading: Job-based AI processing
 * - Debugging: AI decision visualization and profiling
 */

#include "core/ai_types.hpp"
#include "core/ai_memory.hpp"
#include "core/ai_math.hpp"
#include "core/ai_components.hpp"
#include "core/ai_systems.hpp"
#include "core/blackboard.hpp"

// Behavior AI Systems
#include "behavior/finite_state_machine.hpp"
#include "behavior/behavior_tree.hpp"
#include "behavior/goap.hpp"
#include "behavior/utility_ai.hpp"
#include "behavior/flocking.hpp"
#include "behavior/steering.hpp"

// Machine Learning
#include "ml/neural_network.hpp"
#include "ml/reinforcement_learning.hpp"
#include "ml/genetic_algorithm.hpp"
#include "ml/decision_tree.hpp"
#include "ml/clustering.hpp"
#include "ml/feature_extraction.hpp"

// Procedural Content Generation
#include "pcg/noise.hpp"
#include "pcg/l_systems.hpp"
#include "pcg/wave_function_collapse.hpp"
#include "pcg/cellular_automata.hpp"
#include "pcg/grammar_generation.hpp"
#include "pcg/markov_chains.hpp"

// Performance Optimization
#include "optimization/ecs_optimizer.hpp"
#include "optimization/load_balancer.hpp"
#include "optimization/predictive_cache.hpp"
#include "optimization/parameter_tuner.hpp"
#include "optimization/anomaly_detector.hpp"
#include "optimization/memory_optimizer.hpp"

namespace ecscope::ai {

/**
 * @brief AI Framework Version Information
 */
struct Version {
    static constexpr u32 MAJOR = 1;
    static constexpr u32 MINOR = 0;
    static constexpr u32 PATCH = 0;
    
    static std::string to_string() {
        return std::to_string(MAJOR) + "." + 
               std::to_string(MINOR) + "." + 
               std::to_string(PATCH);
    }
};

/**
 * @brief Initialize the AI Framework
 * @param ecs_registry Reference to the ECS registry for integration
 * @param config Global AI configuration
 * @return True if initialization successful
 */
bool initialize(ecs::Registry& ecs_registry, const AIConfig& config = {});

/**
 * @brief Shutdown the AI Framework
 */
void shutdown();

/**
 * @brief Get AI Framework Status
 */
struct FrameworkStatus {
    bool initialized = false;
    usize active_agents = 0;
    usize active_behaviors = 0;
    usize active_ml_models = 0;
    usize memory_usage_bytes = 0;
    f64 average_frame_time_ms = 0.0;
};

FrameworkStatus get_status();

/**
 * @brief Update all AI systems (call once per frame)
 * @param delta_time Time since last update in seconds
 */
void update(f64 delta_time);

/**
 * @brief AI Framework Configuration
 */
struct AIConfig {
    // Threading Configuration
    u32 ai_thread_count = 4;
    bool enable_parallel_processing = true;
    bool enable_job_stealing = true;
    
    // Memory Configuration
    usize ai_memory_pool_size = 256 * 1024 * 1024; // 256MB
    bool enable_memory_tracking = true;
    bool enable_garbage_collection = true;
    
    // Behavior AI Configuration
    bool enable_fsm = true;
    bool enable_behavior_trees = true;
    bool enable_goap = true;
    bool enable_utility_ai = true;
    bool enable_flocking = true;
    
    // ML Configuration
    bool enable_neural_networks = true;
    bool enable_reinforcement_learning = true;
    bool enable_genetic_algorithms = true;
    bool enable_decision_trees = true;
    bool enable_clustering = true;
    
    // PCG Configuration
    bool enable_noise_generation = true;
    bool enable_l_systems = true;
    bool enable_wave_function_collapse = true;
    bool enable_cellular_automata = true;
    bool enable_grammar_generation = true;
    
    // Optimization Configuration
    bool enable_ecs_optimization = true;
    bool enable_load_balancing = true;
    bool enable_predictive_caching = true;
    bool enable_parameter_tuning = true;
    bool enable_anomaly_detection = true;
    
    // Debug Configuration
    bool enable_ai_visualization = true;
    bool enable_performance_profiling = true;
    bool enable_decision_logging = true;
    usize max_debug_history = 10000;
    
    // Integration Configuration
    bool deep_ecs_integration = true;
    bool enable_scripting_bridge = true;
    bool enable_asset_integration = true;
    bool enable_networking_sync = false;
};

/**
 * @brief AI Agent Manager - High-level interface for AI entities
 */
class AIAgentManager {
public:
    explicit AIAgentManager(ecs::Registry& registry);
    ~AIAgentManager() = default;
    
    // Agent Creation
    ecs::Entity create_agent(const std::string& name, const AgentConfig& config = {});
    void destroy_agent(ecs::Entity agent);
    
    // Behavior Assignment
    void assign_fsm(ecs::Entity agent, std::shared_ptr<FSM> fsm);
    void assign_behavior_tree(ecs::Entity agent, std::shared_ptr<BehaviorTree> bt);
    void assign_goap(ecs::Entity agent, std::shared_ptr<GOAPPlanner> planner);
    void assign_utility_ai(ecs::Entity agent, std::shared_ptr<UtilityAI> utility);
    
    // Group Management
    void create_group(const std::string& group_name, const std::vector<ecs::Entity>& agents);
    void assign_flocking_behavior(const std::string& group_name, const FlockingConfig& config);
    
    // Blackboard Access
    std::shared_ptr<Blackboard> get_agent_blackboard(ecs::Entity agent);
    std::shared_ptr<Blackboard> get_global_blackboard();
    
    // Statistics
    AgentStats get_agent_stats(ecs::Entity agent);
    std::vector<AgentStats> get_all_agent_stats();
    
private:
    ecs::Registry* registry_;
    std::unordered_map<ecs::Entity, std::shared_ptr<Blackboard>> agent_blackboards_;
    std::shared_ptr<Blackboard> global_blackboard_;
    std::unordered_map<std::string, std::vector<ecs::Entity>> groups_;
};

/**
 * @brief ML Model Manager - High-level interface for ML models
 */
class MLModelManager {
public:
    MLModelManager() = default;
    ~MLModelManager() = default;
    
    // Neural Networks
    u32 create_neural_network(const NeuralNetworkConfig& config);
    bool train_neural_network(u32 model_id, const TrainingData& data);
    std::vector<f32> predict(u32 model_id, const std::vector<f32>& input);
    
    // Reinforcement Learning
    u32 create_rl_agent(const RLConfig& config);
    void train_rl_agent(u32 agent_id, const Environment& env, u32 episodes);
    Action get_rl_action(u32 agent_id, const State& state);
    
    // Genetic Algorithms
    u32 create_genetic_algorithm(const GAConfig& config);
    void run_genetic_algorithm(u32 ga_id, u32 generations);
    std::vector<f32> get_best_solution(u32 ga_id);
    
    // Model Persistence
    bool save_model(u32 model_id, const std::string& filepath);
    u32 load_model(const std::string& filepath);
    
    // Model Statistics
    ModelStats get_model_stats(u32 model_id);
    
private:
    std::unordered_map<u32, std::unique_ptr<NeuralNetwork>> neural_networks_;
    std::unordered_map<u32, std::unique_ptr<RLAgent>> rl_agents_;
    std::unordered_map<u32, std::unique_ptr<GeneticAlgorithm>> genetic_algorithms_;
    u32 next_model_id_ = 1;
};

/**
 * @brief PCG Manager - High-level interface for procedural generation
 */
class PCGManager {
public:
    PCGManager() = default;
    ~PCGManager() = default;
    
    // Noise Generation
    NoiseGenerator create_noise_generator(NoiseType type, u64 seed = 0);
    f32 sample_noise_2d(const NoiseGenerator& gen, f32 x, f32 y);
    f32 sample_noise_3d(const NoiseGenerator& gen, f32 x, f32 y, f32 z);
    
    // L-Systems
    LSystemGenerator create_l_system(const LSystemRules& rules);
    std::string generate_l_system(const LSystemGenerator& gen, u32 iterations);
    
    // Wave Function Collapse
    WFCGenerator create_wfc(const TileSet& tileset, const WFCConstraints& constraints);
    Grid2D<u32> generate_wfc_2d(const WFCGenerator& gen, u32 width, u32 height);
    
    // Cellular Automata
    CellularAutomaton create_cellular_automaton(const CARule& rule);
    Grid2D<bool> generate_cellular_automata(const CellularAutomaton& ca, 
                                          const Grid2D<bool>& initial, u32 steps);
    
    // Grammar Generation
    GrammarGenerator create_grammar(const GrammarRules& rules);
    std::string generate_text(const GrammarGenerator& gen, u32 max_length);
    
private:
    std::vector<NoiseGenerator> noise_generators_;
    std::vector<LSystemGenerator> l_system_generators_;
    std::vector<WFCGenerator> wfc_generators_;
};

/**
 * @brief Performance Optimizer - ML-based system optimization
 */
class PerformanceOptimizer {
public:
    explicit PerformanceOptimizer(ecs::Registry& registry);
    ~PerformanceOptimizer() = default;
    
    // ECS Optimization
    void optimize_ecs_layout();
    void optimize_system_scheduling();
    void optimize_query_caching();
    
    // Load Balancing
    void enable_dynamic_load_balancing();
    void set_load_balancing_strategy(LoadBalancingStrategy strategy);
    
    // Predictive Caching
    void enable_predictive_asset_caching();
    void predict_and_cache_assets();
    
    // Parameter Tuning
    void auto_tune_physics_parameters();
    void auto_tune_rendering_parameters();
    void auto_tune_ai_parameters();
    
    // Anomaly Detection
    void enable_performance_anomaly_detection();
    std::vector<PerformanceAnomaly> get_detected_anomalies();
    
    // Memory Optimization
    void optimize_memory_layout();
    void enable_garbage_collection_prediction();
    
private:
    ecs::Registry* registry_;
    std::unique_ptr<ECSOptimizer> ecs_optimizer_;
    std::unique_ptr<LoadBalancer> load_balancer_;
    std::unique_ptr<PredictiveCache> predictive_cache_;
    std::unique_ptr<ParameterTuner> parameter_tuner_;
    std::unique_ptr<AnomalyDetector> anomaly_detector_;
    std::unique_ptr<MemoryOptimizer> memory_optimizer_;
};

} // namespace ecscope::ai