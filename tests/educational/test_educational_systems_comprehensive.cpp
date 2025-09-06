#include "../framework/ecscope_test_framework.hpp"
#include <ecscope/asset_education_system.hpp>
#include <ecscope/audio_education_system.hpp>
#include <ecscope/physics_education_tools.hpp>
#include <ecscope/debug_integration_system.hpp>
#include <ecscope/visual_ecs_inspector.hpp>
#include <ecscope/sparse_set_visualizer.hpp>
#include <ecscope/system_dependency_visualizer.hpp>

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>

namespace ECScope::Testing {

// =============================================================================
// Educational Systems Test Fixture
// =============================================================================

class EducationalSystemTest : public ECScopeTestFixture {
protected:
    void SetUp() override {
        ECScopeTestFixture::SetUp();
        
        // Initialize educational systems
        asset_education_ = std::make_unique<Education::AssetSystem>();
        audio_education_ = std::make_unique<Education::AudioSystem>();
        physics_education_ = std::make_unique<Education::PhysicsTools>();
        debug_integration_ = std::make_unique<Education::DebugIntegration>();
        ecs_inspector_ = std::make_unique<Education::ECSInspector>();
        sparse_set_viz_ = std::make_unique<Education::SparseSetVisualizer>();
        dependency_viz_ = std::make_unique<Education::SystemDependencyVisualizer>();
        
        // Set up educational content directory
        content_dir_ = "educational_content";
        std::filesystem::create_directories(content_dir_);
        
        // Initialize learning progress tracking
        setup_learning_modules();
        
        // Create test educational scenarios
        create_educational_scenarios();
    }

    void TearDown() override {
        dependency_viz_.reset();
        sparse_set_viz_.reset();
        ecs_inspector_.reset();
        debug_integration_.reset();
        physics_education_.reset();
        audio_education_.reset();
        asset_education_.reset();
        ECScopeTestFixture::TearDown();
    }

    void setup_learning_modules() {
        // Define learning modules and their progression
        learning_modules_ = {
            {
                "ecs_basics",
                {
                    "entities_and_components",
                    "systems_and_queries",
                    "archetype_management",
                    "performance_considerations"
                }
            },
            {
                "memory_management",
                {
                    "stack_vs_heap",
                    "memory_pools",
                    "cache_locality",
                    "numa_awareness"
                }
            },
            {
                "physics_simulation",
                {
                    "basic_kinematics",
                    "collision_detection",
                    "constraint_solving",
                    "soft_body_dynamics"
                }
            },
            {
                "audio_processing",
                {
                    "spatial_audio_basics",
                    "dsp_fundamentals",
                    "hrtf_processing",
                    "real_time_audio"
                }
            },
            {
                "networking_concepts",
                {
                    "client_server_architecture",
                    "state_synchronization",
                    "prediction_rollback",
                    "lag_compensation"
                }
            }
        };
    }

    void create_educational_scenarios() {
        // Create entities for educational demonstrations
        
        // Scenario 1: ECS Component lifecycle
        demo_entity_1_ = world_->create_entity();
        world_->add_component<TestPosition>(demo_entity_1_, 0, 0, 0);
        
        // Scenario 2: System dependencies
        demo_entity_2_ = world_->create_entity();
        world_->add_component<TestPosition>(demo_entity_2_, 5, 0, 0);
        world_->add_component<TestVelocity>(demo_entity_2_, 1, 0, 0);
        
        // Scenario 3: Memory layout demonstration
        for (int i = 0; i < 100; ++i) {
            auto entity = world_->create_entity();
            world_->add_component<TestPosition>(entity, 
                static_cast<float>(i % 10), static_cast<float>(i / 10), 0);
            
            if (i % 2 == 0) {
                world_->add_component<TestVelocity>(entity, 1, 1, 1);
            }
            if (i % 3 == 0) {
                world_->add_component<TestHealth>(entity, 100, 100);
            }
            
            demo_entities_.push_back(entity);
        }
    }

protected:
    std::unique_ptr<Education::AssetSystem> asset_education_;
    std::unique_ptr<Education::AudioSystem> audio_education_;
    std::unique_ptr<Education::PhysicsTools> physics_education_;
    std::unique_ptr<Education::DebugIntegration> debug_integration_;
    std::unique_ptr<Education::ECSInspector> ecs_inspector_;
    std::unique_ptr<Education::SparseSetVisualizer> sparse_set_viz_;
    std::unique_ptr<Education::SystemDependencyVisualizer> dependency_viz_;
    
    std::string content_dir_;
    std::map<std::string, std::vector<std::string>> learning_modules_;
    
    Entity demo_entity_1_;
    Entity demo_entity_2_;
    std::vector<Entity> demo_entities_;
};

// =============================================================================
// Tutorial System Validation Tests
// =============================================================================

TEST_F(EducationalSystemTest, ECSBasicsTutorialValidation) {
    const std::string module = "ecs_basics";
    
    // Test tutorial progression for ECS basics
    auto tutorial_steps = asset_education_->get_tutorial_steps(module);
    EXPECT_GT(tutorial_steps.size(), 0) << "ECS basics tutorial should have steps";
    
    // Validate each tutorial step
    for (const auto& step : tutorial_steps) {
        // Check that step has required content
        EXPECT_FALSE(step.title.empty()) << "Tutorial step should have title";
        EXPECT_FALSE(step.description.empty()) << "Tutorial step should have description";
        EXPECT_FALSE(step.code_example.empty()) << "Tutorial step should have code example";
        
        // Validate code example syntax
        bool syntax_valid = asset_education_->validate_code_syntax(step.code_example);
        EXPECT_TRUE(syntax_valid) << "Code example should have valid syntax: " << step.title;
        
        // Check interactive elements
        if (!step.interactive_elements.empty()) {
            for (const auto& element : step.interactive_elements) {
                EXPECT_TRUE(asset_education_->validate_interactive_element(element))
                    << "Interactive element should be valid: " << element.type;
            }
        }
    }
    
    // Test tutorial completion tracking
    Education::LearningProgress progress;
    progress.module_id = module;
    progress.completed_steps = 0;
    progress.total_steps = tutorial_steps.size();
    
    // Simulate tutorial progression
    for (size_t i = 0; i < tutorial_steps.size(); ++i) {
        asset_education_->complete_tutorial_step(progress, i);
        EXPECT_EQ(progress.completed_steps, i + 1);
        
        float completion_percentage = asset_education_->get_completion_percentage(progress);
        float expected_percentage = static_cast<float>(i + 1) / tutorial_steps.size() * 100.0f;
        EXPECT_NEAR(completion_percentage, expected_percentage, 0.1f);
    }
    
    EXPECT_TRUE(asset_education_->is_module_completed(progress));
}

TEST_F(EducationalSystemTest, InteractiveTutorialValidation) {
    // Test interactive tutorial components
    
    // Create interactive ECS demonstration
    auto demo_data = ecs_inspector_->create_demo_scenario("component_lifecycle");
    EXPECT_FALSE(demo_data.entities.empty()) << "Demo should have entities";
    EXPECT_FALSE(demo_data.systems.empty()) << "Demo should have systems";
    
    // Test step-by-step execution
    Education::InteractiveTutorial tutorial;
    tutorial.scenario_id = "component_lifecycle";
    tutorial.current_step = 0;
    
    std::vector<std::string> expected_steps = {
        "create_entity",
        "add_component",
        "query_components",
        "modify_component",
        "remove_component",
        "destroy_entity"
    };
    
    for (const auto& step : expected_steps) {
        bool step_valid = asset_education_->execute_tutorial_step(tutorial, step);
        EXPECT_TRUE(step_valid) << "Tutorial step should execute successfully: " << step;
        
        // Verify system state after step
        auto state = ecs_inspector_->capture_system_state(*world_);
        EXPECT_TRUE(asset_education_->validate_system_state(state, step))
            << "System state should be valid after step: " << step;
        
        tutorial.current_step++;
    }
}

TEST_F(EducationalSystemTest, CodeExampleValidation) {
    // Test that all code examples in tutorials are correct and compilable
    
    for (const auto& [module_name, lessons] : learning_modules_) {
        auto code_examples = asset_education_->get_code_examples(module_name);
        
        for (const auto& example : code_examples) {
            // Test syntax validation
            bool syntax_valid = asset_education_->validate_syntax(example.code);
            EXPECT_TRUE(syntax_valid) 
                << "Code example should have valid syntax in module: " << module_name
                << ", example: " << example.title;
            
            // Test compilation (if supported)
            if (asset_education_->supports_compilation()) {
                auto compile_result = asset_education_->test_compile(example.code);
                EXPECT_TRUE(compile_result.success)
                    << "Code should compile: " << compile_result.error_message;
            }
            
            // Test expected output
            if (!example.expected_output.empty()) {
                auto execution_result = asset_education_->execute_example(example.code);
                EXPECT_EQ(execution_result.output, example.expected_output)
                    << "Code output should match expected output";
            }
            
            // Validate educational annotations
            for (const auto& annotation : example.annotations) {
                EXPECT_FALSE(annotation.text.empty()) 
                    << "Annotation should have text";
                EXPECT_GE(annotation.line_number, 1) 
                    << "Annotation should reference valid line";
                EXPECT_LE(annotation.line_number, count_lines(example.code))
                    << "Annotation line should exist in code";
            }
        }
    }
}

// =============================================================================
// Learning Progress Tracking Tests
// =============================================================================

TEST_F(EducationalSystemTest, LearningProgressTracking) {
    // Test comprehensive learning progress tracking
    
    Education::StudentProfile profile;
    profile.student_id = "test_student_123";
    profile.skill_level = Education::SkillLevel::Beginner;
    profile.learning_style = Education::LearningStyle::Visual;
    
    // Initialize progress tracking
    asset_education_->initialize_student_progress(profile);
    
    // Test module progression
    for (const auto& [module_name, lessons] : learning_modules_) {
        auto module_progress = asset_education_->start_module(profile.student_id, module_name);
        
        EXPECT_EQ(module_progress.module_id, module_name);
        EXPECT_EQ(module_progress.completed_steps, 0);
        EXPECT_FALSE(module_progress.is_completed);
        
        // Simulate lesson completion
        for (size_t i = 0; i < lessons.size(); ++i) {
            const std::string& lesson = lessons[i];
            
            // Record lesson start time
            auto start_time = std::chrono::steady_clock::now();
            
            // Simulate lesson completion
            bool lesson_completed = asset_education_->complete_lesson(
                profile.student_id, module_name, lesson);
            EXPECT_TRUE(lesson_completed);
            
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time);
            
            // Record lesson metrics
            Education::LessonMetrics metrics;
            metrics.lesson_id = lesson;
            metrics.completion_time_minutes = duration.count();
            metrics.attempts = 1;
            metrics.hints_used = 0;
            
            asset_education_->record_lesson_metrics(profile.student_id, metrics);
            
            // Verify progress update
            auto updated_progress = asset_education_->get_module_progress(
                profile.student_id, module_name);
            EXPECT_EQ(updated_progress.completed_steps, i + 1);
        }
        
        // Verify module completion
        auto final_progress = asset_education_->get_module_progress(
            profile.student_id, module_name);
        EXPECT_TRUE(final_progress.is_completed);
        EXPECT_EQ(final_progress.completed_steps, lessons.size());
    }
    
    // Test overall progress calculation
    auto overall_progress = asset_education_->get_overall_progress(profile.student_id);
    EXPECT_EQ(overall_progress.completed_modules, learning_modules_.size());
    EXPECT_NEAR(overall_progress.completion_percentage, 100.0f, 0.1f);
}

TEST_F(EducationalSystemTest, AdaptiveLearningPath) {
    // Test adaptive learning path generation based on student performance
    
    Education::StudentProfile profile;
    profile.student_id = "adaptive_test_student";
    profile.skill_level = Education::SkillLevel::Intermediate;
    profile.learning_style = Education::LearningStyle::Kinesthetic;
    
    asset_education_->initialize_student_progress(profile);
    
    // Simulate varied performance across different topics
    std::map<std::string, Education::PerformanceMetrics> topic_performance = {
        {"ecs_basics", {85.0f, 12, 2}},        // High score, fast, few attempts
        {"memory_management", {45.0f, 35, 8}},  // Low score, slow, many attempts
        {"physics_simulation", {70.0f, 20, 4}}, // Medium performance
        {"audio_processing", {92.0f, 10, 1}},   // Excellent performance
        {"networking_concepts", {30.0f, 45, 12}} // Poor performance
    };
    
    for (const auto& [topic, performance] : topic_performance) {
        asset_education_->record_topic_performance(profile.student_id, topic, performance);
    }
    
    // Generate adaptive learning path
    auto adaptive_path = asset_education_->generate_adaptive_path(profile.student_id);
    
    EXPECT_FALSE(adaptive_path.recommended_modules.empty()) 
        << "Should recommend modules for improvement";
    
    // Should prioritize struggling areas
    bool found_memory_focus = std::find(adaptive_path.recommended_modules.begin(),
                                       adaptive_path.recommended_modules.end(),
                                       "memory_management") != adaptive_path.recommended_modules.end();
    bool found_networking_focus = std::find(adaptive_path.recommended_modules.begin(),
                                           adaptive_path.recommended_modules.end(),
                                           "networking_concepts") != adaptive_path.recommended_modules.end();
                                           
    EXPECT_TRUE(found_memory_focus || found_networking_focus) 
        << "Should recommend struggling topics for review";
    
    // Should suggest appropriate difficulty level
    EXPECT_NE(adaptive_path.suggested_difficulty, Education::Difficulty::Advanced)
        << "Should not suggest advanced difficulty for struggling student";
        
    // Should recommend learning resources
    EXPECT_FALSE(adaptive_path.recommended_resources.empty())
        << "Should recommend learning resources";
}

// =============================================================================
// Interactive Visualization Tests
// =============================================================================

TEST_F(EducationalSystemTest, ECSVisualizationAccuracy) {
    // Test ECS inspector visualization accuracy
    
    // Create known ECS state
    auto visualization_entity = world_->create_entity();
    world_->add_component<TestPosition>(visualization_entity, 10, 20, 30);
    world_->add_component<TestVelocity>(visualization_entity, 1, 2, 3);
    
    // Capture visualization data
    auto visualization_data = ecs_inspector_->generate_visualization_data(*world_);
    
    // Verify entity representation
    bool found_entity = false;
    for (const auto& entity_viz : visualization_data.entities) {
        if (entity_viz.entity_id == visualization_entity) {
            found_entity = true;
            
            // Verify components are represented
            bool has_position = false;
            bool has_velocity = false;
            
            for (const auto& component : entity_viz.components) {
                if (component.type_name == "TestPosition") {
                    has_position = true;
                    // Verify component data
                    EXPECT_NE(component.data.find("x: 10"), std::string::npos);
                    EXPECT_NE(component.data.find("y: 20"), std::string::npos);
                    EXPECT_NE(component.data.find("z: 30"), std::string::npos);
                }
                if (component.type_name == "TestVelocity") {
                    has_velocity = true;
                    EXPECT_NE(component.data.find("vx: 1"), std::string::npos);
                    EXPECT_NE(component.data.find("vy: 2"), std::string::npos);
                    EXPECT_NE(component.data.find("vz: 3"), std::string::npos);
                }
            }
            
            EXPECT_TRUE(has_position) << "Should visualize position component";
            EXPECT_TRUE(has_velocity) << "Should visualize velocity component";
            break;
        }
    }
    
    EXPECT_TRUE(found_entity) << "Should find visualization entity";
    
    // Test archetype visualization
    auto archetype_data = ecs_inspector_->generate_archetype_visualization(*world_);
    EXPECT_GT(archetype_data.archetypes.size(), 0) << "Should have archetype data";
    
    // Verify archetype contains our entity
    bool found_archetype = false;
    for (const auto& archetype : archetype_data.archetypes) {
        if (archetype.component_types.count("TestPosition") > 0 &&
            archetype.component_types.count("TestVelocity") > 0) {
            found_archetype = true;
            EXPECT_GT(archetype.entity_count, 0) << "Archetype should have entities";
            break;
        }
    }
    
    EXPECT_TRUE(found_archetype) << "Should find matching archetype";
}

TEST_F(EducationalSystemTest, SparseSetVisualization) {
    // Test sparse set visualization for educational purposes
    
    // Create sparse scenario (many entities, few with specific component)
    constexpr size_t total_entities = 1000;
    constexpr size_t tagged_entities = 50;
    
    std::vector<Entity> all_entities;
    
    // Create many entities
    for (size_t i = 0; i < total_entities; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<TestPosition>(entity, static_cast<float>(i), 0, 0);
        all_entities.push_back(entity);
    }
    
    // Add special tag to only some entities (sparse)
    for (size_t i = 0; i < tagged_entities; ++i) {
        size_t entity_index = i * (total_entities / tagged_entities);
        world_->add_component<TestTag>(all_entities[entity_index], "special");
    }
    
    // Generate sparse set visualization
    auto sparse_viz = sparse_set_viz_->generate_sparse_set_visualization<TestTag>(*world_);
    
    EXPECT_EQ(sparse_viz.total_entities, total_entities);
    EXPECT_EQ(sparse_viz.component_entities, tagged_entities);
    EXPECT_NEAR(sparse_viz.density, static_cast<float>(tagged_entities) / total_entities, 0.01f);
    
    // Verify visualization shows sparse nature
    EXPECT_LT(sparse_viz.density, 0.1f) << "Should demonstrate sparse set (< 10% density)";
    
    // Test dense visualization for comparison
    auto dense_viz = sparse_set_viz_->generate_sparse_set_visualization<TestPosition>(*world_);
    EXPECT_NEAR(dense_viz.density, 1.0f, 0.01f) << "Position component should be dense (100% density)";
    
    // Clean up
    for (auto entity : all_entities) {
        world_->destroy_entity(entity);
    }
}

TEST_F(EducationalSystemTest, SystemDependencyVisualization) {
    // Test system dependency visualization
    
    // Create mock systems with dependencies
    Education::SystemInfo movement_system;
    movement_system.name = "MovementSystem";
    movement_system.required_components = {"TestPosition", "TestVelocity"};
    movement_system.dependencies = {};
    
    Education::SystemInfo rendering_system;
    rendering_system.name = "RenderingSystem";
    rendering_system.required_components = {"TestPosition"};
    rendering_system.dependencies = {"MovementSystem"}; // Depends on movement for updated positions
    
    Education::SystemInfo collision_system;
    collision_system.name = "CollisionSystem";
    collision_system.required_components = {"TestPosition"};
    collision_system.dependencies = {"MovementSystem"};
    
    Education::SystemInfo audio_system;
    audio_system.name = "AudioSystem";
    audio_system.required_components = {"TestPosition"};
    audio_system.dependencies = {"MovementSystem", "CollisionSystem"}; // Needs position and collision info
    
    std::vector<Education::SystemInfo> systems = {
        movement_system, rendering_system, collision_system, audio_system
    };
    
    // Generate dependency graph
    auto dependency_graph = dependency_viz_->generate_dependency_graph(systems);
    
    EXPECT_EQ(dependency_graph.nodes.size(), 4) << "Should have 4 system nodes";
    EXPECT_GT(dependency_graph.edges.size(), 0) << "Should have dependency edges";
    
    // Verify dependency relationships
    bool movement_to_rendering = false;
    bool movement_to_collision = false;
    bool collision_to_audio = false;
    
    for (const auto& edge : dependency_graph.edges) {
        if (edge.from == "MovementSystem" && edge.to == "RenderingSystem") {
            movement_to_rendering = true;
        }
        if (edge.from == "MovementSystem" && edge.to == "CollisionSystem") {
            movement_to_collision = true;
        }
        if (edge.from == "CollisionSystem" && edge.to == "AudioSystem") {
            collision_to_audio = true;
        }
    }
    
    EXPECT_TRUE(movement_to_rendering) << "Should show movement->rendering dependency";
    EXPECT_TRUE(movement_to_collision) << "Should show movement->collision dependency";
    EXPECT_TRUE(collision_to_audio) << "Should show collision->audio dependency";
    
    // Test cycle detection
    auto cycles = dependency_viz_->detect_dependency_cycles(dependency_graph);
    EXPECT_EQ(cycles.size(), 0) << "Should not detect cycles in valid dependency graph";
    
    // Test execution order generation
    auto execution_order = dependency_viz_->generate_execution_order(systems);
    EXPECT_EQ(execution_order.size(), 4) << "Should generate execution order for all systems";
    
    // MovementSystem should be first (no dependencies)
    EXPECT_EQ(execution_order[0], "MovementSystem") << "Movement system should execute first";
}

// =============================================================================
// Educational Content Correctness Tests
// =============================================================================

TEST_F(EducationalSystemTest, PhysicsEducationToolsValidation) {
    // Test physics education tools for correctness
    
    // Test physics concept explanations
    auto kinematic_explanation = physics_education_->get_concept_explanation("kinematics");
    EXPECT_FALSE(kinematic_explanation.title.empty()) << "Should have physics concept explanation";
    EXPECT_FALSE(kinematic_explanation.description.empty()) << "Should have concept description";
    EXPECT_GT(kinematic_explanation.formulas.size(), 0) << "Should have relevant formulas";
    
    // Test physics simulation validation
    Education::PhysicsScenario scenario;
    scenario.name = "projectile_motion";
    scenario.initial_velocity = {10.0f, 15.0f, 0.0f};
    scenario.gravity = {0.0f, -9.81f, 0.0f};
    scenario.simulation_time = 2.0f;
    
    auto simulation_result = physics_education_->run_educational_simulation(scenario);
    
    // Verify physics correctness
    EXPECT_GT(simulation_result.trajectory_points.size(), 0) << "Should generate trajectory points";
    
    // Check trajectory follows physics laws
    // At peak, vertical velocity should be approximately zero
    float peak_time = scenario.initial_velocity.y / std::abs(scenario.gravity.y);
    float peak_height = scenario.initial_velocity.y * peak_time + 0.5f * scenario.gravity.y * peak_time * peak_time;
    
    bool found_peak = false;
    for (const auto& point : simulation_result.trajectory_points) {
        if (std::abs(point.time - peak_time) < 0.1f) { // Within 0.1s of expected peak
            EXPECT_NEAR(point.velocity.y, 0.0f, 1.0f) << "Velocity should be near zero at peak";
            EXPECT_NEAR(point.position.y, peak_height, 0.5f) << "Height should match calculated peak";
            found_peak = true;
            break;
        }
    }
    
    EXPECT_TRUE(found_peak) << "Should find trajectory peak";
    
    // Test educational annotations
    auto annotations = physics_education_->generate_trajectory_annotations(simulation_result);
    EXPECT_GT(annotations.size(), 0) << "Should generate educational annotations";
    
    // Should have annotation for peak
    bool has_peak_annotation = std::any_of(annotations.begin(), annotations.end(),
        [](const Education::Annotation& ann) {
            return ann.type == Education::AnnotationType::Peak;
        });
    EXPECT_TRUE(has_peak_annotation) << "Should annotate trajectory peak";
}

TEST_F(EducationalSystemTest, AudioEducationSystemValidation) {
    // Test audio education system correctness
    
    // Test basic audio concept explanations
    auto wave_explanation = audio_education_->get_concept_explanation("sound_waves");
    EXPECT_FALSE(wave_explanation.description.empty()) << "Should explain sound waves";
    EXPECT_GT(wave_explanation.interactive_demos.size(), 0) << "Should have interactive demos";
    
    // Test audio visualization generation
    std::vector<float> test_signal;
    float frequency = 440.0f; // A4
    float sample_rate = 48000.0f;
    float duration = 1.0f;
    
    // Generate test sine wave
    size_t sample_count = static_cast<size_t>(sample_rate * duration);
    for (size_t i = 0; i < sample_count; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        test_signal.push_back(std::sin(2.0f * M_PI * frequency * t));
    }
    
    // Generate waveform visualization
    auto waveform_viz = audio_education_->generate_waveform_visualization(test_signal, sample_rate);
    EXPECT_GT(waveform_viz.data_points.size(), 0) << "Should generate waveform visualization";
    EXPECT_NEAR(waveform_viz.peak_amplitude, 1.0f, 0.1f) << "Peak amplitude should be ~1.0";
    
    // Generate spectrum visualization
    auto spectrum_viz = audio_education_->generate_spectrum_visualization(test_signal, sample_rate);
    EXPECT_GT(spectrum_viz.frequency_bins.size(), 0) << "Should generate spectrum visualization";
    
    // Find peak frequency in spectrum
    float peak_freq = 0.0f;
    float peak_magnitude = 0.0f;
    for (size_t i = 0; i < spectrum_viz.frequency_bins.size(); ++i) {
        if (spectrum_viz.magnitude[i] > peak_magnitude) {
            peak_magnitude = spectrum_viz.magnitude[i];
            peak_freq = spectrum_viz.frequency_bins[i];
        }
    }
    
    EXPECT_NEAR(peak_freq, frequency, 20.0f) << "Should detect correct peak frequency";
    
    // Test 3D audio education tools
    Education::SpatialAudioScenario spatial_scenario;
    spatial_scenario.listener_position = {0, 0, 0};
    spatial_scenario.source_position = {5, 0, 0}; // 5 units to the right
    spatial_scenario.frequency = 1000.0f;
    
    auto spatial_result = audio_education_->demonstrate_spatial_audio(spatial_scenario);
    
    // Right ear should be louder due to proximity
    EXPECT_GT(spatial_result.right_ear_amplitude, spatial_result.left_ear_amplitude)
        << "Right ear should be louder for right-side source";
        
    // Should have time delay between ears
    EXPECT_GT(spatial_result.interaural_time_difference, 0.0f)
        << "Should have positive ITD for right-side source";
}

// =============================================================================
// Educational Assessment Tests
// =============================================================================

TEST_F(EducationalSystemTest, KnowledgeAssessmentValidation) {
    // Test educational assessment system
    
    Education::Assessment ecs_assessment;
    ecs_assessment.assessment_id = "ecs_fundamentals";
    ecs_assessment.topic = "ecs_basics";
    ecs_assessment.difficulty = Education::Difficulty::Beginner;
    
    // Add test questions
    Education::Question q1;
    q1.question_text = "What is an Entity in an ECS architecture?";
    q1.type = Education::QuestionType::MultipleChoice;
    q1.correct_answers = {"A unique identifier that represents a game object"};
    q1.options = {
        "A unique identifier that represents a game object",
        "A collection of data components",
        "A function that processes entities",
        "A storage container for components"
    };
    
    Education::Question q2;
    q2.question_text = "Implement a basic Position component with x, y coordinates.";
    q2.type = Education::QuestionType::CodeWriting;
    q2.correct_answers = {"struct Position { float x, y; };"};
    q2.validation_criteria = {
        "Contains struct or class definition",
        "Has x and y members",
        "Uses appropriate numeric type"
    };
    
    ecs_assessment.questions = {q1, q2};
    
    // Test assessment validation
    bool assessment_valid = asset_education_->validate_assessment(ecs_assessment);
    EXPECT_TRUE(assessment_valid) << "Assessment should be valid";
    
    // Test student response evaluation
    Education::StudentResponse response1;
    response1.question_id = 0;
    response1.answer = "A unique identifier that represents a game object";
    
    Education::StudentResponse response2;
    response2.question_id = 1;
    response2.answer = "struct Position { float x; float y; };";
    
    std::vector<Education::StudentResponse> responses = {response1, response2};
    
    auto assessment_result = asset_education_->evaluate_assessment(ecs_assessment, responses);
    
    EXPECT_EQ(assessment_result.total_questions, 2);
    EXPECT_EQ(assessment_result.correct_answers, 2) << "Both answers should be correct";
    EXPECT_NEAR(assessment_result.score_percentage, 100.0f, 0.1f);
    
    // Test partial credit for code questions
    Education::StudentResponse partial_response;
    partial_response.question_id = 1;
    partial_response.answer = "struct Position { float x; }"; // Missing y coordinate
    
    std::vector<Education::StudentResponse> partial_responses = {response1, partial_response};
    auto partial_result = asset_education_->evaluate_assessment(ecs_assessment, partial_responses);
    
    EXPECT_LT(partial_result.score_percentage, 100.0f) << "Should receive partial credit";
    EXPECT_GT(partial_result.score_percentage, 50.0f) << "Should get more than 50% for partial answer";
}

// =============================================================================
// Performance Visualization Tests
// =============================================================================

TEST_F(EducationalSystemTest, PerformanceVisualizationAccuracy) {
    // Test performance visualization tools for educational accuracy
    
    // Create scenario with known performance characteristics
    constexpr size_t entity_count = 10000;
    
    // Scenario 1: Cache-friendly layout (all entities have same components)
    std::vector<Entity> cache_friendly_entities;
    for (size_t i = 0; i < entity_count; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<TestPosition>(entity, static_cast<float>(i), 0, 0);
        world_->add_component<TestVelocity>(entity, 1, 1, 1);
        cache_friendly_entities.push_back(entity);
    }
    
    // Measure cache-friendly performance
    auto start_time = std::chrono::high_resolution_clock::now();
    world_->each<TestPosition, TestVelocity>([](Entity, TestPosition& pos, TestVelocity& vel) {
        pos.x += vel.vx * 0.016f;
        pos.y += vel.vy * 0.016f;
        pos.z += vel.vz * 0.016f;
    });
    auto cache_friendly_time = std::chrono::high_resolution_clock::now() - start_time;
    
    // Clean up
    for (auto entity : cache_friendly_entities) {
        world_->destroy_entity(entity);
    }
    
    // Scenario 2: Cache-unfriendly layout (mixed archetypes)
    std::vector<Entity> cache_unfriendly_entities;
    for (size_t i = 0; i < entity_count; ++i) {
        auto entity = world_->create_entity();
        world_->add_component<TestPosition>(entity, static_cast<float>(i), 0, 0);
        
        if (i % 2 == 0) {
            world_->add_component<TestVelocity>(entity, 1, 1, 1);
        }
        if (i % 3 == 0) {
            world_->add_component<TestHealth>(entity, 100, 100);
        }
        if (i % 5 == 0) {
            world_->add_component<TestTag>(entity, "mixed");
        }
        
        cache_unfriendly_entities.push_back(entity);
    }
    
    // Measure cache-unfriendly performance
    start_time = std::chrono::high_resolution_clock::now();
    world_->each<TestPosition, TestVelocity>([](Entity, TestPosition& pos, TestVelocity& vel) {
        pos.x += vel.vx * 0.016f;
        pos.y += vel.vy * 0.016f;
        pos.z += vel.vz * 0.016f;
    });
    auto cache_unfriendly_time = std::chrono::high_resolution_clock::now() - start_time;
    
    // Generate performance visualization
    Education::PerformanceComparison comparison;
    comparison.scenario1_name = "Cache-friendly (homogeneous archetypes)";
    comparison.scenario1_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(cache_friendly_time).count();
    comparison.scenario2_name = "Cache-unfriendly (mixed archetypes)";
    comparison.scenario2_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(cache_unfriendly_time).count();
    
    auto performance_viz = debug_integration_->generate_performance_comparison(comparison);
    
    EXPECT_GT(performance_viz.speedup_factor, 1.0f) 
        << "Cache-friendly should be faster than cache-unfriendly";
    
    EXPECT_FALSE(performance_viz.explanation.empty())
        << "Should provide educational explanation of performance difference";
        
    EXPECT_GT(performance_viz.memory_access_patterns.size(), 0)
        << "Should visualize memory access patterns";
    
    // Clean up
    for (auto entity : cache_unfriendly_entities) {
        world_->destroy_entity(entity);
    }
}

} // namespace ECScope::Testing