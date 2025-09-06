# ECScope Interactive Documentation System

**Comprehensive interactive documentation framework with live code examples, educational tutorials, and automated API generation**

## System Overview

The ECScope Interactive Documentation System transforms traditional static documentation into a living, breathing educational platform that adapts to user needs and provides hands-on learning experiences.

### Core Components

```
Interactive Documentation System
├── Live Code Examples Engine
├── Interactive Tutorial Framework  
├── Automated API Documentation Generator
├── Educational Visualization Suite
├── Performance Analysis Integration
├── Search & Navigation System
└── Multi-Format Export Pipeline
```

## Architecture

### Documentation Engine Core

```typescript
interface DocumentationEngine {
    // Core functionality
    initialize(): Promise<void>;
    loadModule(modulePath: string): Promise<DocumentationModule>;
    registerCodeExample(example: CodeExample): void;
    
    // Interactive features
    executeCodeExample(id: string, parameters?: any): Promise<ExecutionResult>;
    visualizeSystem(systemName: string, config?: VisualizationConfig): void;
    
    // Navigation and search
    search(query: string): Promise<SearchResult[]>;
    navigate(path: string): void;
    
    // Educational features
    startTutorial(tutorialId: string): Promise<TutorialSession>;
    assessUnderstanding(answers: Answer[]): AssessmentResult;
}
```

### Live Code Examples

```cpp
// Interactive code examples with real-time execution
class LiveCodeExample {
private:
    std::string example_id_;
    std::string source_code_;
    std::unordered_map<std::string, std::string> parameters_;
    std::function<void()> execution_callback_;
    
public:
    LiveCodeExample(std::string id, std::string code);
    
    // Interactive execution
    void set_parameter(const std::string& name, const std::string& value);
    ExecutionResult execute();
    void visualize_execution();
    
    // Educational features
    std::vector<std::string> get_learning_objectives() const;
    std::string explain_concept(const std::string& concept) const;
    std::vector<CodeExample> get_related_examples() const;
};
```

## Implementation Plan

### Phase 1: Documentation Infrastructure

#### 1.1 Core Documentation Engine
```javascript
// docs/interactive/core/documentation-engine.js
class DocumentationEngine {
    constructor() {
        this.modules = new Map();
        this.codeExamples = new Map();
        this.tutorials = new Map();
        this.searchIndex = new SearchIndex();
    }
    
    async initialize() {
        await this.loadCoreModules();
        await this.buildSearchIndex();
        await this.initializeCodeExecutor();
    }
    
    async executeCode(exampleId, parameters = {}) {
        const example = this.codeExamples.get(exampleId);
        if (!example) throw new Error(`Example ${exampleId} not found`);
        
        // Prepare execution environment
        const executor = new CodeExecutor({
            type: example.language,
            safetyLevel: 'high',
            timeoutMs: 30000
        });
        
        // Execute with visualization
        const result = await executor.run(example.code, parameters);
        
        // Generate educational insights
        const insights = this.generateInsights(result, example);
        
        return {
            output: result,
            insights: insights,
            visualizations: this.generateVisualizations(result)
        };
    }
}
```

#### 1.2 Interactive Tutorial Framework
```javascript
// docs/interactive/tutorials/tutorial-framework.js
class InteractiveTutorial {
    constructor(tutorialConfig) {
        this.steps = tutorialConfig.steps;
        this.currentStep = 0;
        this.userProgress = new Map();
        this.adaptiveEngine = new AdaptiveLearningEngine();
    }
    
    async startStep(stepIndex) {
        const step = this.steps[stepIndex];
        
        // Load step resources
        await this.loadStepResources(step);
        
        // Setup interactive environment
        const environment = new InteractiveEnvironment({
            codeEditor: step.hasCodeEditor,
            visualization: step.visualization,
            assessment: step.assessment
        });
        
        // Track learning analytics
        this.trackStepStart(stepIndex);
        
        return environment;
    }
    
    async completeStep(stepIndex, userResponse) {
        const assessment = await this.assessResponse(stepIndex, userResponse);
        this.userProgress.set(stepIndex, assessment);
        
        // Adaptive recommendations
        const recommendations = this.adaptiveEngine.getRecommendations(
            this.userProgress,
            this.currentStep
        );
        
        return {
            assessment: assessment,
            nextStep: this.getNextStep(assessment),
            recommendations: recommendations
        };
    }
}
```

### Phase 2: Live Code Execution

#### 2.1 Safe Code Executor
```javascript
// docs/interactive/execution/code-executor.js
class SafeCodeExecutor {
    constructor(config) {
        this.sandbox = new WebAssemblyVirtualMachine(config);
        this.resourceLimits = config.resourceLimits;
        this.allowedAPIs = config.allowedAPIs;
    }
    
    async execute(code, language, parameters = {}) {
        // Validate code safety
        const validation = await this.validateCode(code, language);
        if (!validation.safe) {
            throw new SecurityError(validation.issues);
        }
        
        // Prepare execution context
        const context = this.createExecutionContext(parameters);
        
        // Execute with monitoring
        const monitor = new ExecutionMonitor({
            memoryLimit: this.resourceLimits.memory,
            timeLimit: this.resourceLimits.time,
            cpuLimit: this.resourceLimits.cpu
        });
        
        try {
            const result = await this.sandbox.run(code, context, monitor);
            
            // Generate educational output
            return {
                success: true,
                output: result.output,
                performance: result.performance,
                memoryUsage: result.memoryUsage,
                insights: this.generateInsights(result),
                visualizations: this.createVisualizations(result)
            };
        } catch (error) {
            return {
                success: false,
                error: this.formatError(error),
                suggestions: this.generateSuggestions(error)
            };
        }
    }
}
```

#### 2.2 Performance Integration
```cpp
// Integration with ECScope performance system
class DocumentationPerformanceIntegrator {
private:
    std::shared_ptr<performance::PerformanceLab> performance_lab_;
    std::unique_ptr<memory::MemoryTracker> memory_tracker_;
    
public:
    DocumentationPerformanceIntegrator(
        std::shared_ptr<performance::PerformanceLab> lab) 
        : performance_lab_(lab) {
        memory_tracker_ = std::make_unique<memory::MemoryTracker>();
    }
    
    // Execute example with performance tracking
    ExecutionResult execute_example_with_tracking(
        const std::string& example_id,
        const std::unordered_map<std::string, std::string>& parameters) {
        
        PROFILE_SCOPE("Documentation Example Execution");
        
        memory_tracker_->start_tracking_scope(example_id);
        
        // Execute the example
        auto result = execute_example(example_id, parameters);
        
        // Gather performance metrics
        auto memory_snapshot = memory_tracker_->end_tracking_scope();
        auto performance_metrics = performance_lab_->get_current_metrics();
        
        result.performance_data = {
            .execution_time = performance_metrics.last_frame_time,
            .memory_used = memory_snapshot.peak_usage,
            .cache_efficiency = performance_metrics.cache_hit_ratio,
            .educational_insights = generate_performance_insights(
                performance_metrics, memory_snapshot)
        };
        
        return result;
    }
};
```

### Phase 3: Automated API Documentation

#### 3.1 Source Code Analysis Engine
```cpp
// tools/doc-generator/api-analyzer.hpp
class APIDocumentationGenerator {
private:
    std::unique_ptr<clang::ASTConsumer> ast_consumer_;
    std::vector<APIDocumentation> documented_apis_;
    
public:
    struct APIDocumentation {
        std::string name;
        std::string description;
        std::vector<Parameter> parameters;
        std::string return_type;
        std::vector<CodeExample> examples;
        std::vector<std::string> related_concepts;
        PerformanceCharacteristics performance;
        std::string educational_notes;
    };
    
    // Generate comprehensive API documentation
    std::vector<APIDocumentation> analyze_codebase(
        const std::vector<std::string>& header_files) {
        
        for (const auto& file : header_files) {
            analyze_header_file(file);
        }
        
        // Cross-reference with examples
        link_code_examples();
        
        // Generate performance data
        extract_performance_characteristics();
        
        // Add educational annotations
        generate_educational_content();
        
        return documented_apis_;
    }
    
private:
    void analyze_header_file(const std::string& filename) {
        // Parse with Clang AST
        // Extract classes, functions, enums
        // Generate documentation metadata
    }
    
    void link_code_examples() {
        // Find relevant examples in examples/ directory
        // Associate with API functions
        // Create interactive demonstrations
    }
};
```

#### 3.2 Interactive API Explorer
```javascript
// docs/interactive/api/api-explorer.js
class InteractiveAPIExplorer {
    constructor(apiDatabase) {
        this.apis = apiDatabase;
        this.codeGenerator = new CodeGenerator();
        this.visualizer = new APIVisualizer();
    }
    
    async exploreAPI(apiName) {
        const api = this.apis.get(apiName);
        if (!api) throw new Error(`API ${apiName} not found`);
        
        return {
            // Basic information
            documentation: api.documentation,
            signature: api.signature,
            parameters: api.parameters,
            
            // Interactive elements
            interactiveExample: await this.generateInteractiveExample(api),
            parameterEditor: this.createParameterEditor(api),
            visualizations: await this.visualizer.createAPIVisualizations(api),
            
            // Educational content
            conceptExplanations: api.educationalNotes,
            relatedConcepts: this.findRelatedConcepts(api),
            learningPath: this.generateLearningPath(api),
            
            // Performance insights
            performanceCharacteristics: api.performance,
            optimizationTips: this.generateOptimizationTips(api)
        };
    }
    
    async generateInteractiveExample(api) {
        const example = await this.codeGenerator.generateExample(api);
        return new LiveCodeExample({
            id: `api_${api.name}_example`,
            code: example.code,
            explanation: example.explanation,
            interactiveParameters: example.parameters,
            expectedOutput: example.expectedOutput
        });
    }
}
```

### Phase 4: Educational Visualization Suite

#### 4.1 System Architecture Visualizer
```javascript
// docs/interactive/visualization/system-visualizer.js
class SystemArchitectureVisualizer {
    constructor(canvas) {
        this.canvas = canvas;
        this.renderer = new Three.WebGLRenderer({ canvas });
        this.scene = new Three.Scene();
        this.animationEngine = new AnimationEngine();
    }
    
    visualizeECSArchitecture(ecsData) {
        // Create 3D visualization of ECS components
        const visualization = {
            entities: this.createEntityVisualization(ecsData.entities),
            components: this.createComponentVisualization(ecsData.components),
            systems: this.createSystemVisualization(ecsData.systems),
            dataFlow: this.createDataFlowVisualization(ecsData.dataFlow)
        };
        
        // Add interactive elements
        this.addInteractiveControls(visualization);
        
        // Start animation
        this.animationEngine.start();
        
        return visualization;
    }
    
    visualizeMemoryLayout(memoryData) {
        return {
            // Memory structure visualization
            arenas: this.visualizeArenaAllocators(memoryData.arenas),
            pools: this.visualizePoolAllocators(memoryData.pools),
            cacheLines: this.visualizeCacheLines(memoryData.cacheAccess),
            
            // Performance overlays
            hotspots: this.visualizePerformanceHotspots(memoryData.hotspots),
            accessPatterns: this.visualizeAccessPatterns(memoryData.access)
        };
    }
    
    visualizePhysicsSimulation(physicsData) {
        return {
            // Physics visualization
            rigidBodies: this.visualizeRigidBodies(physicsData.bodies),
            collisions: this.visualizeCollisions(physicsData.collisions),
            constraints: this.visualizeConstraints(physicsData.constraints),
            
            // Algorithm steps
            broadPhase: this.visualizeBroadPhase(physicsData.broadPhase),
            narrowPhase: this.visualizeNarrowPhase(physicsData.narrowPhase),
            integration: this.visualizeIntegration(physicsData.integration)
        };
    }
}
```

#### 4.2 Performance Analysis Visualizer
```javascript
// docs/interactive/visualization/performance-visualizer.js
class PerformanceAnalysisVisualizer {
    constructor() {
        this.chartEngine = new ChartEngine();
        this.dataProcessor = new DataProcessor();
    }
    
    createPerformanceCharts(performanceData) {
        return {
            // Timing charts
            frameTimes: this.chartEngine.createTimeSeriesChart({
                data: performanceData.frameTimes,
                title: "Frame Time Analysis",
                yAxis: "Time (ms)",
                annotations: this.identifyPerformanceSpikes(performanceData.frameTimes)
            }),
            
            // Memory charts
            memoryUsage: this.chartEngine.createAreaChart({
                data: performanceData.memoryUsage,
                title: "Memory Usage Over Time",
                yAxis: "Memory (MB)",
                breakdown: performanceData.memoryBreakdown
            }),
            
            // Cache analysis
            cacheEfficiency: this.chartEngine.createHeatmap({
                data: performanceData.cacheHits,
                title: "Cache Efficiency Heatmap",
                interactive: true,
                tooltips: this.generateCacheTooltips(performanceData.cacheHits)
            }),
            
            // Scaling analysis
            scalingCurves: this.chartEngine.createScatterPlot({
                data: performanceData.scalingData,
                title: "Performance Scaling Analysis",
                xAxis: "Entity Count",
                yAxis: "Operations/Second",
                trendLines: true,
                annotations: this.identifyScalingBottlenecks(performanceData.scalingData)
            })
        };
    }
}
```

### Phase 5: Advanced Search and Navigation

#### 5.1 Semantic Search Engine
```javascript
// docs/interactive/search/semantic-search.js
class SemanticSearchEngine {
    constructor() {
        this.index = new SearchIndex();
        this.vectorizer = new TextVectorizer();
        this.conceptGraph = new ConceptGraph();
    }
    
    async buildIndex(documentationSources) {
        // Process all documentation
        for (const source of documentationSources) {
            const processed = await this.processDocument(source);
            this.index.addDocument(processed);
        }
        
        // Build concept relationships
        await this.conceptGraph.buildFromDocuments(documentationSources);
        
        // Create vector embeddings for semantic search
        await this.createVectorEmbeddings();
    }
    
    async search(query, options = {}) {
        // Parse query intent
        const queryIntent = await this.parseQueryIntent(query);
        
        // Multiple search strategies
        const results = await Promise.all([
            this.textualSearch(query, options),
            this.semanticSearch(queryIntent, options),
            this.conceptualSearch(queryIntent, options),
            this.exampleSearch(query, options)
        ]);
        
        // Merge and rank results
        const mergedResults = this.mergeSearchResults(results);
        
        // Add educational context
        const contextualResults = await this.addEducationalContext(mergedResults);
        
        return {
            results: contextualResults,
            suggestions: this.generateSearchSuggestions(query),
            relatedConcepts: this.findRelatedConcepts(queryIntent),
            learningPath: this.generateLearningPath(contextualResults)
        };
    }
}
```

#### 5.2 Intelligent Navigation System
```javascript
// docs/interactive/navigation/smart-navigation.js
class SmartNavigationSystem {
    constructor() {
        this.userModel = new UserLearningModel();
        this.contentGraph = new ContentGraph();
        this.pathfinder = new LearningPathfinder();
    }
    
    generateLearningPath(userId, targetConcept, currentKnowledge = {}) {
        // Assess current knowledge level
        const assessment = this.userModel.assessKnowledge(userId, currentKnowledge);
        
        // Find optimal learning path
        const path = this.pathfinder.findOptimalPath({
            start: assessment.knowledgeState,
            target: targetConcept,
            userPreferences: this.userModel.getPreferences(userId),
            difficulty: assessment.skillLevel
        });
        
        return {
            steps: path.steps.map(step => ({
                concept: step.concept,
                resources: step.resources,
                exercises: step.exercises,
                assessments: step.assessments,
                estimatedTime: step.estimatedTime,
                prerequisites: step.prerequisites,
                learningObjectives: step.objectives
            })),
            totalEstimatedTime: path.totalTime,
            difficultyProgression: path.difficultyProgression,
            checkpoints: path.checkpoints
        };
    }
    
    adaptToUserProgress(userId, completedStep, performance) {
        // Update user model
        this.userModel.updateProgress(userId, completedStep, performance);
        
        // Adapt future path
        const updatedPath = this.pathfinder.adaptPath(
            userId,
            performance,
            this.userModel.getPredictions(userId)
        );
        
        return updatedPath;
    }
}
```

## Integration with ECScope Systems

### Performance Lab Integration
```cpp
// Integration with existing performance systems
class DocumentationPerformanceIntegration {
private:
    std::shared_ptr<performance::PerformanceLab> perf_lab_;
    std::shared_ptr<performance::ecs::ECSPerformanceBenchmarker> ecs_benchmarker_;
    
public:
    // Generate live performance documentation
    struct LivePerformanceDoc {
        std::string system_name;
        std::vector<performance::BenchmarkResult> current_metrics;
        std::vector<std::string> optimization_suggestions;
        visualization::PerformanceCharts charts;
    };
    
    LivePerformanceDoc generate_live_performance_doc(const std::string& system) {
        LivePerformanceDoc doc;
        doc.system_name = system;
        
        // Run real-time benchmarks
        auto benchmark_config = performance::ecs::ECSBenchmarkConfig::create_quick();
        auto results = ecs_benchmarker_->run_specific_benchmarks({system});
        doc.current_metrics = results;
        
        // Generate suggestions
        doc.optimization_suggestions = ecs_benchmarker_->suggest_optimizations(results[0]);
        
        // Create visualizations
        doc.charts = visualization::create_performance_charts(results);
        
        return doc;
    }
};
```

### Memory System Integration
```cpp
// Integration with memory tracking
class DocumentationMemoryIntegration {
private:
    std::unique_ptr<memory::MemoryTracker> memory_tracker_;
    std::unique_ptr<memory::MemoryLaboratory> memory_lab_;
    
public:
    // Generate memory usage documentation with live data
    struct LiveMemoryDoc {
        memory::MemorySnapshot current_state;
        std::vector<memory::AllocationInfo> recent_allocations;
        memory::MemoryLaboratory::ExperimentResult latest_experiment;
        std::vector<std::string> educational_insights;
    };
    
    LiveMemoryDoc generate_live_memory_doc() {
        LiveMemoryDoc doc;
        
        // Capture current state
        doc.current_state = memory_tracker_->capture_snapshot();
        
        // Get recent allocations for analysis
        doc.recent_allocations = memory_tracker_->get_recent_allocations(100);
        
        // Run memory experiment
        auto experiment_config = memory::MemoryLaboratory::ExperimentConfig{
            .type = memory::ExperimentType::AllocationPattern,
            .entity_count = 1000,
            .enable_visualization = true
        };
        doc.latest_experiment = memory_lab_->run_experiment(experiment_config);
        
        // Generate insights
        doc.educational_insights = generate_memory_insights(doc);
        
        return doc;
    }
};
```

## User Experience Features

### Interactive Learning Dashboard
```html
<!-- docs/interactive/dashboard/learning-dashboard.html -->
<!DOCTYPE html>
<html>
<head>
    <title>ECScope Interactive Learning Dashboard</title>
    <link rel="stylesheet" href="styles/dashboard.css">
</head>
<body>
    <div class="dashboard-container">
        <!-- Learning Progress Sidebar -->
        <aside class="learning-sidebar">
            <div class="progress-overview">
                <h3>Learning Progress</h3>
                <div class="skill-meters">
                    <div class="skill-meter">
                        <label>ECS Architecture</label>
                        <progress value="75" max="100"></progress>
                    </div>
                    <div class="skill-meter">
                        <label>Memory Management</label>
                        <progress value="60" max="100"></progress>
                    </div>
                    <div class="skill-meter">
                        <label>Performance Optimization</label>
                        <progress value="40" max="100"></progress>
                    </div>
                </div>
            </div>
            
            <div class="current-path">
                <h3>Current Learning Path</h3>
                <ol class="learning-steps">
                    <li class="completed">ECS Basics</li>
                    <li class="completed">Component Design</li>
                    <li class="current">Memory Layout Optimization</li>
                    <li class="upcoming">Advanced Queries</li>
                    <li class="upcoming">System Dependencies</li>
                </ol>
            </div>
        </aside>
        
        <!-- Main Content Area -->
        <main class="main-content">
            <!-- Interactive Code Editor -->
            <section class="code-playground">
                <div class="editor-header">
                    <h2>Interactive Code Playground</h2>
                    <div class="editor-controls">
                        <button onclick="runCode()">Run Code</button>
                        <button onclick="resetCode()">Reset</button>
                        <button onclick="explainCode()">Explain</button>
                    </div>
                </div>
                
                <div class="editor-container">
                    <div class="code-editor" id="codeEditor">
                        <!-- Monaco Editor will be initialized here -->
                    </div>
                    
                    <div class="output-panel">
                        <div class="tabs">
                            <button class="tab active" data-tab="output">Output</button>
                            <button class="tab" data-tab="visualization">Visualization</button>
                            <button class="tab" data-tab="performance">Performance</button>
                            <button class="tab" data-tab="insights">Insights</button>
                        </div>
                        
                        <div class="tab-content">
                            <div id="output-content" class="tab-pane active">
                                <!-- Code execution output -->
                            </div>
                            <div id="visualization-content" class="tab-pane">
                                <!-- Interactive visualizations -->
                                <canvas id="visualizationCanvas"></canvas>
                            </div>
                            <div id="performance-content" class="tab-pane">
                                <!-- Performance metrics and charts -->
                            </div>
                            <div id="insights-content" class="tab-pane">
                                <!-- Educational insights and explanations -->
                            </div>
                        </div>
                    </div>
                </div>
            </section>
            
            <!-- Documentation Viewer -->
            <section class="documentation-viewer">
                <div class="doc-header">
                    <input type="text" placeholder="Search documentation..." class="search-bar">
                    <div class="view-controls">
                        <button class="view-btn active" data-view="interactive">Interactive</button>
                        <button class="view-btn" data-view="reference">Reference</button>
                        <button class="view-btn" data-view="tutorial">Tutorial</button>
                    </div>
                </div>
                
                <div class="doc-content" id="documentationContent">
                    <!-- Dynamic documentation content -->
                </div>
            </section>
        </main>
        
        <!-- Performance Monitor Sidebar -->
        <aside class="performance-sidebar">
            <div class="live-metrics">
                <h3>Live Performance Metrics</h3>
                <div class="metric-cards">
                    <div class="metric-card">
                        <div class="metric-value" id="frameTime">16.7ms</div>
                        <div class="metric-label">Frame Time</div>
                    </div>
                    <div class="metric-card">
                        <div class="metric-value" id="memoryUsage">42MB</div>
                        <div class="metric-label">Memory Usage</div>
                    </div>
                    <div class="metric-card">
                        <div class="metric-value" id="cacheHitRatio">89%</div>
                        <div class="metric-label">Cache Hit Ratio</div>
                    </div>
                </div>
                
                <div class="performance-chart">
                    <canvas id="livePerformanceChart"></canvas>
                </div>
            </div>
            
            <div class="optimization-tips">
                <h3>Optimization Suggestions</h3>
                <ul class="tips-list" id="optimizationTips">
                    <li class="tip">
                        <span class="tip-icon">💡</span>
                        Consider using SoA layout for better cache performance
                    </li>
                    <li class="tip">
                        <span class="tip-icon">⚡</span>
                        Batch similar operations to reduce overhead
                    </li>
                </ul>
            </div>
        </aside>
    </div>
    
    <!-- JavaScript initialization -->
    <script src="js/monaco-editor.min.js"></script>
    <script src="js/chart.js"></script>
    <script src="js/three.min.js"></script>
    <script src="js/dashboard.js"></script>
</body>
</html>
```

### Responsive Design System
```css
/* docs/interactive/styles/responsive-design.css */
:root {
    --primary-color: #2563eb;
    --secondary-color: #64748b;
    --success-color: #10b981;
    --warning-color: #f59e0b;
    --danger-color: #ef4444;
    --background: #ffffff;
    --surface: #f8fafc;
    --text-primary: #1e293b;
    --text-secondary: #64748b;
    --border: #e2e8f0;
}

[data-theme="dark"] {
    --background: #0f172a;
    --surface: #1e293b;
    --text-primary: #f1f5f9;
    --text-secondary: #94a3b8;
    --border: #334155;
}

.dashboard-container {
    display: grid;
    grid-template-columns: 280px 1fr 320px;
    grid-template-rows: 100vh;
    gap: 1rem;
    padding: 1rem;
    background: var(--background);
    color: var(--text-primary);
}

@media (max-width: 1200px) {
    .dashboard-container {
        grid-template-columns: 1fr;
        grid-template-rows: auto auto 1fr;
    }
    
    .learning-sidebar,
    .performance-sidebar {
        order: -1;
    }
}

.code-playground {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 8px;
    overflow: hidden;
}

.editor-container {
    display: grid;
    grid-template-columns: 1fr 1fr;
    height: 600px;
}

@media (max-width: 768px) {
    .editor-container {
        grid-template-columns: 1fr;
        grid-template-rows: 1fr 1fr;
    }
}

.interactive-element {
    padding: 1rem;
    margin: 0.5rem 0;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: var(--surface);
    transition: all 0.2s ease;
}

.interactive-element:hover {
    border-color: var(--primary-color);
    box-shadow: 0 4px 12px rgba(37, 99, 235, 0.15);
}

.code-example {
    position: relative;
    background: #1e293b;
    color: #f1f5f9;
    padding: 1rem;
    border-radius: 6px;
    font-family: 'Fira Code', monospace;
}

.code-example::before {
    content: 'Live Example';
    position: absolute;
    top: -10px;
    left: 1rem;
    background: var(--primary-color);
    color: white;
    padding: 0.25rem 0.5rem;
    border-radius: 4px;
    font-size: 0.75rem;
    font-weight: 500;
}
```

## Configuration and Setup

### Documentation Configuration
```yaml
# docs/config/documentation.yml
documentation:
  title: "ECScope Interactive Documentation"
  version: "1.0.0"
  
  # Core settings
  core:
    language: "en"
    theme: "auto" # light, dark, auto
    search_provider: "semantic"
    analytics_enabled: true
    
  # Interactive features
  interactive:
    code_execution: true
    max_execution_time: 30000 # milliseconds
    memory_limit: "128MB"
    allowed_languages: ["cpp", "javascript", "glsl"]
    
  # Learning features
  learning:
    adaptive_difficulty: true
    progress_tracking: true
    assessment_enabled: true
    personalization: true
    
  # Performance integration
  performance:
    live_metrics: true
    benchmark_integration: true
    memory_tracking: true
    optimization_suggestions: true
    
  # Content organization
  content:
    auto_generate_api: true
    include_examples: true
    cross_reference: true
    version_control: true
    
  # Export options
  export:
    formats: ["html", "pdf", "epub", "json"]
    include_interactive: false # for static exports
    bundle_assets: true
    
  # Customization
  customization:
    logo: "assets/logo.svg"
    favicon: "assets/favicon.ico"
    custom_css: "styles/custom.css"
    custom_js: "js/custom.js"
```

### Build System Integration
```cmake
# CMake integration for documentation generation
# Add to main CMakeLists.txt

option(ECSCOPE_BUILD_DOCS "Build ECScope documentation" ON)
option(ECSCOPE_DOCS_INTERACTIVE "Include interactive features in documentation" ON)

if(ECSCOPE_BUILD_DOCS)
    find_program(NODE_JS node REQUIRED)
    find_program(NPM npm REQUIRED)
    
    # Setup documentation build target
    add_custom_target(docs_build
        COMMAND ${CMAKE_COMMAND} -E echo "Building interactive documentation..."
        COMMAND ${NPM} install
        COMMAND ${NPM} run build
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/interactive
        COMMENT "Building ECScope interactive documentation"
    )
    
    # Setup documentation server for development
    add_custom_target(docs_serve
        COMMAND ${NPM} run serve
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/interactive
        COMMENT "Starting ECScope documentation server"
    )
    
    # API documentation generation
    if(ECSCOPE_DOCS_INTERACTIVE)
        find_program(CLANG_TOOL clang++ REQUIRED)
        
        add_custom_target(docs_generate_api
            COMMAND ${CMAKE_SOURCE_DIR}/tools/generate_api_docs.py
                --source-dir ${CMAKE_SOURCE_DIR}/include
                --output-dir ${CMAKE_SOURCE_DIR}/docs/interactive/generated
                --examples-dir ${CMAKE_SOURCE_DIR}/examples
            COMMENT "Generating API documentation from source code"
        )
        
        add_dependencies(docs_build docs_generate_api)
    endif()
    
    # Install documentation
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/docs/interactive/dist/
            DESTINATION share/ecscope/docs
            COMPONENT docs)
endif()
```

## Educational Content Structure

### Learning Modules
```
docs/interactive/content/
├── fundamentals/
│   ├── 01-introduction-to-ecs/
│   │   ├── concept.md
│   │   ├── interactive-demo.js
│   │   ├── exercises.json
│   │   └── assessment.json
│   ├── 02-components-and-entities/
│   ├── 03-systems-and-queries/
│   └── 04-architecture-benefits/
├── memory-management/
│   ├── 01-allocation-strategies/
│   ├── 02-cache-optimization/
│   ├── 03-memory-layouts/
│   └── 04-performance-analysis/
├── advanced-systems/
│   ├── 01-physics-integration/
│   ├── 02-rendering-pipeline/
│   ├── 03-multi-threading/
│   └── 04-optimization-techniques/
└── research-topics/
    ├── 01-novel-ecs-patterns/
    ├── 02-memory-research/
    └── 03-performance-studies/
```

### Interactive Tutorials Structure
```json
{
  "tutorial_id": "memory-optimization-basics",
  "title": "Memory Layout Optimization in ECS",
  "description": "Learn how different memory layouts affect performance",
  "estimated_duration": "45 minutes",
  "difficulty": "intermediate",
  "prerequisites": ["ecs-basics", "cpp-fundamentals"],
  "learning_objectives": [
    "Understand SoA vs AoS memory layouts",
    "Measure cache performance impact",
    "Implement cache-friendly data structures",
    "Analyze memory access patterns"
  ],
  "steps": [
    {
      "step_id": "intro-memory-layouts",
      "title": "Introduction to Memory Layouts",
      "type": "concept",
      "content": {
        "explanation": "...",
        "visualizations": ["memory-layout-comparison"],
        "interactive_demo": "memory-layout-demo"
      }
    },
    {
      "step_id": "implement-soa",
      "title": "Implementing Structure of Arrays",
      "type": "coding_exercise",
      "content": {
        "starter_code": "...",
        "instructions": "...",
        "hints": ["..."],
        "solution": "...",
        "tests": ["..."]
      }
    },
    {
      "step_id": "performance-measurement",
      "title": "Measuring Performance Impact",
      "type": "experiment",
      "content": {
        "experiment_config": {...},
        "expected_results": {...},
        "analysis_questions": ["..."]
      }
    },
    {
      "step_id": "assessment",
      "title": "Knowledge Check",
      "type": "assessment",
      "content": {
        "questions": [...],
        "interactive_challenges": [...]
      }
    }
  ]
}
```

This comprehensive interactive documentation system transforms ECScope from a static learning resource into a dynamic, adaptive educational platform. Users can experiment with real code, see immediate performance impacts, and follow personalized learning paths that adapt to their understanding and progress.

The system leverages ECScope's existing performance and memory tracking infrastructure to provide real-time insights and educational value, making complex systems engineering concepts tangible and understandable through interactive experimentation.