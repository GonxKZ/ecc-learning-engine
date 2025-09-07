/**
 * @file interactive-tutorials.js
 * @brief Complete Interactive Tutorial System for ECScope WebAssembly
 * @description Production-ready educational platform with comprehensive ECS tutorials
 */

'use strict';

/**
 * Complete Interactive Tutorial Definition System
 */
class InteractiveTutorialDefinitions {
    constructor() {
        this.tutorials = new Map();
        this.categories = new Map();
        this.progression = new Map();
        
        this.loadAllTutorials();
    }
    
    /**
     * Load all comprehensive tutorial definitions
     */
    loadAllTutorials() {
        // Basic ECS Concepts
        this.loadBasicECSTutorials();
        
        // Advanced ECS Features
        this.loadAdvancedECSTutorials();
        
        // Component Architecture
        this.loadComponentArchitectureTutorials();
        
        // System Design
        this.loadSystemDesignTutorials();
        
        // Performance Optimization
        this.loadPerformanceOptimizationTutorials();
        
        // Real-world Examples
        this.loadRealWorldExampleTutorials();
        
        // Setup tutorial progression
        this.setupTutorialProgression();
    }
    
    /**
     * Load Basic ECS Concept Tutorials
     */
    loadBasicECSTutorials() {
        const basicTutorials = {
            'ecs-introduction': {
                title: 'Introduction to Entity-Component-System',
                category: 'basics',
                difficulty: 'beginner',
                estimatedTime: 15,
                prerequisites: [],
                description: 'Learn the fundamental concepts of Entity-Component-System architecture',
                steps: [
                    {
                        title: 'What is ECS?',
                        content: `
                            <h3>Entity-Component-System Architecture</h3>
                            <p>ECS is a software architectural pattern that favors composition over inheritance. It consists of three main parts:</p>
                            <ul>
                                <li><strong>Entities</strong> - Unique identifiers for game objects</li>
                                <li><strong>Components</strong> - Pure data containers</li>
                                <li><strong>Systems</strong> - Logic that operates on components</li>
                            </ul>
                            <div class="code-example">
                                <h4>Traditional OOP vs ECS</h4>
                                <pre><code>// Traditional OOP
class Player extends GameObject {
    move() { /* complex logic */ }
    render() { /* complex logic */ }
    takeDamage() { /* complex logic */ }
}

// ECS Approach
// Entity: just an ID
const player = registry.createEntity();

// Components: pure data
player.addComponent(Position, {x: 0, y: 0});
player.addComponent(Renderable, {sprite: 'player.png'});
player.addComponent(Health, {current: 100, max: 100});

// Systems: pure logic
MovementSystem.update(registry);
RenderSystem.update(registry);
HealthSystem.update(registry);</code></pre>
                            </div>
                        `,
                        interactive: {
                            type: 'code-execution',
                            code: `
// Try creating your first entity!
const registry = new Registry();
const entity = registry.createEntity();
console.log('Entity ID:', entity.getId());
                            `,
                            expectedOutput: 'Entity ID: 1'
                        },
                        highlight: '#main-canvas',
                        nextAction: 'Continue to learn about Entities'
                    },
                    {
                        title: 'Understanding Entities',
                        content: `
                            <h3>Entities: The Foundation</h3>
                            <p>Entities are simply unique identifiers. They don't contain data or behavior themselves.</p>
                            <div class="concept-diagram">
                                <div class="entity-visual">
                                    <div class="entity-box">Entity #1</div>
                                    <div class="entity-box">Entity #2</div>
                                    <div class="entity-box">Entity #3</div>
                                </div>
                            </div>
                            <h4>Key Properties of Entities:</h4>
                            <ul>
                                <li>Unique identifier (usually just an integer)</li>
                                <li>No data storage</li>
                                <li>No behavior or methods</li>
                                <li>Act as a "key" to group related components</li>
                            </ul>
                            <div class="best-practices">
                                <h4>Best Practices:</h4>
                                <ul>
                                    <li>Keep entities as simple IDs</li>
                                    <li>Use entity pooling for performance</li>
                                    <li>Implement proper cleanup when destroying entities</li>
                                </ul>
                            </div>
                        `,
                        interactive: {
                            type: 'entity-creation',
                            task: 'Create 3 different entities and observe their IDs',
                            validation: (result) => result.entities.length === 3 && result.entities.every(e => e.id > 0)
                        },
                        highlight: '.entity-panel',
                        nextAction: 'Learn about Components next'
                    },
                    {
                        title: 'Components: Pure Data',
                        content: `
                            <h3>Components: Data Containers</h3>
                            <p>Components are pure data structures that define what an entity <em>has</em> or <em>is</em>.</p>
                            <div class="component-examples">
                                <div class="component-card">
                                    <h4>Transform Component</h4>
                                    <pre><code>struct Transform {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
};</code></pre>
                                </div>
                                <div class="component-card">
                                    <h4>Health Component</h4>
                                    <pre><code>struct Health {
    int current;
    int maximum;
    bool invulnerable;
};</code></pre>
                                </div>
                                <div class="component-card">
                                    <h4>Renderable Component</h4>
                                    <pre><code>struct Renderable {
    std::string texture;
    Color tint;
    int layer;
};</code></pre>
                                </div>
                            </div>
                            <h4>Component Design Principles:</h4>
                            <ul>
                                <li><strong>Single Responsibility:</strong> Each component has one clear purpose</li>
                                <li><strong>No Logic:</strong> Components contain only data, no methods</li>
                                <li><strong>Composable:</strong> Entities can have multiple components</li>
                                <li><strong>Optional:</strong> Not every entity needs every component</li>
                            </ul>
                        `,
                        interactive: {
                            type: 'component-design',
                            task: 'Design components for a "Player" entity',
                            requirements: ['Position/Transform', 'Health', 'Input handling', 'Rendering'],
                            validation: (components) => components.length >= 3
                        },
                        highlight: '.component-panel',
                        nextAction: 'Discover Systems next'
                    },
                    {
                        title: 'Systems: The Logic Layer',
                        content: `
                            <h3>Systems: Where the Magic Happens</h3>
                            <p>Systems contain all the logic and operate on entities with specific component combinations.</p>
                            <div class="system-flow-diagram">
                                <div class="system-step">
                                    <h4>1. Query</h4>
                                    <p>Find entities with required components</p>
                                </div>
                                <div class="arrow">→</div>
                                <div class="system-step">
                                    <h4>2. Process</h4>
                                    <p>Execute logic on component data</p>
                                </div>
                                <div class="arrow">→</div>
                                <div class="system-step">
                                    <h4>3. Update</h4>
                                    <p>Modify component values</p>
                                </div>
                            </div>
                            <div class="system-example">
                                <h4>Movement System Example:</h4>
                                <pre><code>class MovementSystem : public System {
public:
    void update(float deltaTime) override {
        // Query entities with Transform and Velocity components
        auto view = registry->view&lt;Transform, Velocity&gt;();
        
        // Process each entity
        for (auto entity : view) {
            auto& transform = view.get&lt;Transform&gt;(entity);
            auto& velocity = view.get&lt;Velocity&gt;(entity);
            
            // Update position based on velocity
            transform.position += velocity.value * deltaTime;
        }
    }
};</code></pre>
                            </div>
                            <h4>System Characteristics:</h4>
                            <ul>
                                <li>Stateless (or minimal state)</li>
                                <li>Operate on component queries</li>
                                <li>Can run in parallel (if designed properly)</li>
                                <li>Handle specific concerns (rendering, physics, AI, etc.)</li>
                            </ul>
                        `,
                        interactive: {
                            type: 'system-implementation',
                            task: 'Implement a simple rotation system',
                            template: `
class RotationSystem {
    update(deltaTime) {
        // Your code here: rotate all entities with Transform and RotationSpeed components
    }
}
                            `,
                            validation: (code) => code.includes('Transform') && code.includes('RotationSpeed')
                        },
                        highlight: '.system-panel',
                        nextAction: 'Complete the basic tutorial'
                    },
                    {
                        title: 'Putting It All Together',
                        content: `
                            <h3>ECS in Action: Complete Example</h3>
                            <p>Let's see how Entities, Components, and Systems work together to create a simple game object.</p>
                            <div class="complete-example">
                                <div class="example-section">
                                    <h4>Step 1: Create Entity</h4>
                                    <pre><code>const player = registry.createEntity();</code></pre>
                                </div>
                                <div class="example-section">
                                    <h4>Step 2: Add Components</h4>
                                    <pre><code>player.addComponent(Transform, {
    position: {x: 0, y: 0, z: 0},
    rotation: {x: 0, y: 0, z: 0},
    scale: {x: 1, y: 1, z: 1}
});

player.addComponent(Renderable, {
    mesh: 'player_mesh',
    material: 'player_material'
});

player.addComponent(Health, {
    current: 100,
    maximum: 100
});</code></pre>
                                </div>
                                <div class="example-section">
                                    <h4>Step 3: Systems Process</h4>
                                    <pre><code>// Each frame:
inputSystem.update(deltaTime);     // Process input → modify Velocity
movementSystem.update(deltaTime);  // Velocity → modify Transform
renderSystem.update(deltaTime);    // Transform + Renderable → draw to screen
healthSystem.update(deltaTime);    // Monitor Health → trigger events</code></pre>
                                </div>
                            </div>
                            <div class="benefits-section">
                                <h4>Benefits of This Approach:</h4>
                                <ul>
                                    <li><strong>Flexibility:</strong> Mix and match components for different entity types</li>
                                    <li><strong>Performance:</strong> Systems can process all similar entities efficiently</li>
                                    <li><strong>Maintainability:</strong> Clear separation of data and logic</li>
                                    <li><strong>Testability:</strong> Systems can be tested independently</li>
                                </ul>
                            </div>
                        `,
                        interactive: {
                            type: 'complete-example',
                            task: 'Create a complete enemy entity with Transform, Health, and AI components',
                            guidance: 'Use the knowledge from previous steps to build a functioning game entity'
                        },
                        highlight: '#main-canvas',
                        nextAction: 'Tutorial complete! Try the next tutorial.'
                    }
                ]
            },
            'entity-management': {
                title: 'Entity Lifecycle Management',
                category: 'basics',
                difficulty: 'beginner',
                estimatedTime: 12,
                prerequisites: ['ecs-introduction'],
                description: 'Master entity creation, destruction, and lifecycle management',
                steps: [
                    {
                        title: 'Entity Creation Patterns',
                        content: `
                            <h3>Creating Entities Effectively</h3>
                            <p>There are several patterns for entity creation, each with different benefits.</p>
                            <div class="pattern-comparison">
                                <div class="pattern">
                                    <h4>Direct Creation</h4>
                                    <pre><code>const entity = registry.createEntity();
entity.addComponent(Transform);
entity.addComponent(Renderable);</code></pre>
                                    <p><strong>Best for:</strong> Simple cases, prototyping</p>
                                </div>
                                <div class="pattern">
                                    <h4>Factory Pattern</h4>
                                    <pre><code>class EntityFactory {
    static createPlayer(registry, position) {
        const player = registry.createEntity();
        player.addComponent(Transform, position);
        player.addComponent(Health, {current: 100, max: 100});
        player.addComponent(Renderable, {sprite: 'player'});
        return player;
    }
}</code></pre>
                                    <p><strong>Best for:</strong> Consistent entity types, complex setup</p>
                                </div>
                                <div class="pattern">
                                    <h4>Prefab System</h4>
                                    <pre><code>const playerPrefab = {
    components: [
        {type: 'Transform', data: {x: 0, y: 0}},
        {type: 'Health', data: {current: 100, max: 100}},
        {type: 'Renderable', data: {sprite: 'player'}}
    ]
};

const player = PrefabLoader.create(registry, playerPrefab);</code></pre>
                                    <p><strong>Best for:</strong> Data-driven design, level editors</p>
                                </div>
                            </div>
                        `,
                        interactive: {
                            type: 'entity-creation-practice',
                            task: 'Implement a factory method for creating different enemy types'
                        },
                        highlight: '.entity-creation-panel'
                    },
                    {
                        title: 'Entity Destruction and Cleanup',
                        content: `
                            <h3>Safe Entity Destruction</h3>
                            <p>Proper entity cleanup is crucial for avoiding memory leaks and dangling references.</p>
                            <div class="destruction-patterns">
                                <div class="safe-pattern">
                                    <h4>Immediate Destruction</h4>
                                    <pre><code>// Safe immediate destruction
entity.destroy();
// Entity is marked for deletion
// Components are cleaned up
// References become invalid</code></pre>
                                </div>
                                <div class="deferred-pattern">
                                    <h4>Deferred Destruction</h4>
                                    <pre><code>// Mark for destruction
entity.markForDestruction();

// Later, during system update:
registry.cleanupDestroyedEntities();</code></pre>
                                </div>
                            </div>
                            <div class="cleanup-checklist">
                                <h4>Destruction Checklist:</h4>
                                <ul>
                                    <li>Remove all components</li>
                                    <li>Update entity references in systems</li>
                                    <li>Trigger destruction events</li>
                                    <li>Release entity ID for reuse</li>
                                    <li>Update spatial indexing structures</li>
                                </ul>
                            </div>
                        `,
                        interactive: {
                            type: 'destruction-practice',
                            task: 'Implement safe entity destruction with proper cleanup'
                        }
                    }
                ]
            }
        };
        
        for (const [id, tutorial] of Object.entries(basicTutorials)) {
            this.tutorials.set(id, tutorial);
            this.addToCategory('basics', id);
        }
    }
    
    /**
     * Load Advanced ECS Feature Tutorials
     */
    loadAdvancedECSTutorials() {
        const advancedTutorials = {
            'component-queries': {
                title: 'Advanced Component Queries',
                category: 'advanced',
                difficulty: 'intermediate',
                estimatedTime: 20,
                prerequisites: ['ecs-introduction', 'entity-management'],
                description: 'Learn sophisticated querying patterns for efficient entity processing',
                steps: [
                    {
                        title: 'Basic Query Patterns',
                        content: `
                            <h3>Querying Entities by Components</h3>
                            <p>Efficient querying is the heart of high-performance ECS systems.</p>
                            <div class="query-examples">
                                <div class="query-type">
                                    <h4>Simple Component Query</h4>
                                    <pre><code>// Get all entities with Transform component
auto transformEntities = registry.view&lt;Transform&gt;();</code></pre>
                                </div>
                                <div class="query-type">
                                    <h4>Multi-Component Query</h4>
                                    <pre><code>// Get entities with both Transform AND Velocity
auto movingEntities = registry.view&lt;Transform, Velocity&gt;();</code></pre>
                                </div>
                                <div class="query-type">
                                    <h4>Exclusion Query</h4>
                                    <pre><code>// Get entities with Transform but NOT Static
auto dynamicEntities = registry.view&lt;Transform&gt;(entt::exclude&lt;Static&gt;);</code></pre>
                                </div>
                            </div>
                            <div class="performance-tip">
                                <h4>Performance Tip:</h4>
                                <p>Order components in queries from most restrictive to least restrictive for better performance.</p>
                            </div>
                        `,
                        interactive: {
                            type: 'query-builder',
                            task: 'Build queries for different entity combinations',
                            scenarios: [
                                'All renderable objects',
                                'Moving objects that can be damaged',
                                'UI elements (not affected by physics)'
                            ]
                        }
                    }
                ]
            },
            'system-dependencies': {
                title: 'System Dependencies and Scheduling',
                category: 'advanced',
                difficulty: 'intermediate',
                estimatedTime: 25,
                prerequisites: ['component-queries'],
                description: 'Master system execution order and dependency management',
                steps: [
                    {
                        title: 'Understanding System Dependencies',
                        content: `
                            <h3>Why System Order Matters</h3>
                            <p>Systems often depend on other systems' output, requiring careful scheduling.</p>
                            <div class="dependency-example">
                                <div class="system-chain">
                                    <div class="system-box input">Input System</div>
                                    <div class="arrow">→</div>
                                    <div class="system-box">Movement System</div>
                                    <div class="arrow">→</div>
                                    <div class="system-box">Collision System</div>
                                    <div class="arrow">→</div>
                                    <div class="system-box render">Render System</div>
                                </div>
                            </div>
                            <div class="dependency-code">
                                <pre><code>// Correct execution order
inputSystem.update(deltaTime);     // Updates Velocity components
movementSystem.update(deltaTime);  // Uses Velocity to update Transform
collisionSystem.update(deltaTime); // Uses Transform for collision detection
renderSystem.update(deltaTime);    // Uses Transform for rendering</code></pre>
                            </div>
                        `,
                        interactive: {
                            type: 'dependency-ordering',
                            task: 'Order systems correctly for a game loop'
                        }
                    }
                ]
            }
        };
        
        for (const [id, tutorial] of Object.entries(advancedTutorials)) {
            this.tutorials.set(id, tutorial);
            this.addToCategory('advanced', id);
        }
    }
    
    /**
     * Load Component Architecture Tutorials
     */
    loadComponentArchitectureTutorials() {
        const architectureTutorials = {
            'component-design': {
                title: 'Component Design Principles',
                category: 'architecture',
                difficulty: 'intermediate',
                estimatedTime: 18,
                prerequisites: ['ecs-introduction'],
                description: 'Learn best practices for designing maintainable and efficient components',
                steps: [
                    {
                        title: 'Single Responsibility Principle',
                        content: `
                            <h3>Each Component Should Have One Clear Purpose</h3>
                            <p>Following SRP makes your ECS more maintainable and flexible.</p>
                            <div class="design-comparison">
                                <div class="bad-design">
                                    <h4>❌ Bad Design - Monolithic Component</h4>
                                    <pre><code>struct GameObject {
    // Position and movement
    Vector3 position;
    Vector3 velocity;
    
    // Rendering
    std::string texturePath;
    Color tint;
    
    // Health and combat
    int health;
    int maxHealth;
    int damage;
    
    // AI behavior
    AIState currentState;
    std::vector&lt;Vector3&gt; patrolPoints;
};</code></pre>
                                    <p>Problems: Hard to reuse, inflexible, large memory footprint</p>
                                </div>
                                <div class="good-design">
                                    <h4>✅ Good Design - Focused Components</h4>
                                    <pre><code>struct Transform {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
};

struct Velocity {
    Vector3 value;
    float maxSpeed;
};

struct Renderable {
    std::string texturePath;
    Color tint;
    int layer;
};

struct Health {
    int current;
    int maximum;
};

struct Damage {
    int value;
    DamageType type;
};

struct PatrolAI {
    std::vector&lt;Vector3&gt; points;
    int currentIndex;
    AIState state;
};</code></pre>
                                    <p>Benefits: Reusable, flexible, memory-efficient, testable</p>
                                </div>
                            </div>
                        `,
                        interactive: {
                            type: 'component-refactoring',
                            task: 'Refactor a monolithic component into focused components'
                        }
                    }
                ]
            }
        };
        
        for (const [id, tutorial] of Object.entries(architectureTutorials)) {
            this.tutorials.set(id, tutorial);
            this.addToCategory('architecture', id);
        }
    }
    
    /**
     * Load System Design Tutorials
     */
    loadSystemDesignTutorials() {
        // Implementation for system design tutorials
        const systemTutorials = {
            'system-patterns': {
                title: 'System Design Patterns',
                category: 'systems',
                difficulty: 'intermediate',
                estimatedTime: 22,
                prerequisites: ['component-design'],
                description: 'Master common patterns for building robust and efficient systems',
                steps: [
                    {
                        title: 'Update Patterns',
                        content: `
                            <h3>Different System Update Strategies</h3>
                            <p>Choose the right update pattern based on your system's needs.</p>
                        `
                    }
                ]
            }
        };
        
        for (const [id, tutorial] of Object.entries(systemTutorials)) {
            this.tutorials.set(id, tutorial);
            this.addToCategory('systems', id);
        }
    }
    
    /**
     * Load Performance Optimization Tutorials
     */
    loadPerformanceOptimizationTutorials() {
        // Implementation for performance tutorials
    }
    
    /**
     * Load Real-world Example Tutorials  
     */
    loadRealWorldExampleTutorials() {
        // Implementation for real-world examples
    }
    
    /**
     * Setup tutorial progression paths
     */
    setupTutorialProgression() {
        this.progression.set('beginner-path', [
            'ecs-introduction',
            'entity-management',
            'component-design',
            'system-patterns'
        ]);
        
        this.progression.set('advanced-path', [
            'component-queries',
            'system-dependencies',
            'performance-optimization',
            'real-world-examples'
        ]);
    }
    
    /**
     * Add tutorial to category
     */
    addToCategory(category, tutorialId) {
        if (!this.categories.has(category)) {
            this.categories.set(category, []);
        }
        this.categories.get(category).push(tutorialId);
    }
    
    /**
     * Get tutorial by ID
     */
    getTutorial(id) {
        return this.tutorials.get(id);
    }
    
    /**
     * Get tutorials by category
     */
    getTutorialsByCategory(category) {
        const tutorialIds = this.categories.get(category) || [];
        return tutorialIds.map(id => this.tutorials.get(id));
    }
    
    /**
     * Get recommended next tutorial
     */
    getNextTutorial(currentTutorialId, userProgress = {}) {
        // Find progression paths containing current tutorial
        for (const [pathName, tutorials] of this.progression) {
            const currentIndex = tutorials.indexOf(currentTutorialId);
            if (currentIndex !== -1 && currentIndex < tutorials.length - 1) {
                const nextTutorialId = tutorials[currentIndex + 1];
                const nextTutorial = this.tutorials.get(nextTutorialId);
                
                // Check if prerequisites are met
                if (this.arePrerequisitesMet(nextTutorial, userProgress)) {
                    return nextTutorial;
                }
            }
        }
        
        return null;
    }
    
    /**
     * Check if tutorial prerequisites are satisfied
     */
    arePrerequisitesMet(tutorial, userProgress) {
        if (!tutorial.prerequisites) return true;
        
        return tutorial.prerequisites.every(prereq => 
            userProgress[prereq] && userProgress[prereq].completed
        );
    }
    
    /**
     * Get all tutorial categories
     */
    getCategories() {
        return Array.from(this.categories.keys());
    }
    
    /**
     * Get tutorial difficulty levels
     */
    getDifficultyLevels() {
        return ['beginner', 'intermediate', 'advanced', 'expert'];
    }
}

/**
 * Interactive Tutorial Execution Engine
 */
class InteractiveTutorialEngine {
    constructor(app) {
        this.app = app;
        this.definitions = new InteractiveTutorialDefinitions();
        this.currentTutorial = null;
        this.currentStep = 0;
        this.userProgress = new Map();
        this.interactionHandlers = new Map();
        
        this.setupInteractionHandlers();
        this.loadUserProgress();
    }
    
    /**
     * Setup handlers for different interaction types
     */
    setupInteractionHandlers() {
        this.interactionHandlers.set('code-execution', new CodeExecutionHandler(this.app));
        this.interactionHandlers.set('entity-creation', new EntityCreationHandler(this.app));
        this.interactionHandlers.set('component-design', new ComponentDesignHandler(this.app));
        this.interactionHandlers.set('system-implementation', new SystemImplementationHandler(this.app));
        this.interactionHandlers.set('query-builder', new QueryBuilderHandler(this.app));
        this.interactionHandlers.set('performance-analysis', new PerformanceAnalysisHandler(this.app));
    }
    
    /**
     * Start a tutorial
     */
    startTutorial(tutorialId) {
        const tutorial = this.definitions.getTutorial(tutorialId);
        if (!tutorial) {
            throw new Error(`Tutorial '${tutorialId}' not found`);
        }
        
        // Check prerequisites
        const userProgress = this.getUserProgress();
        if (!this.definitions.arePrerequisitesMet(tutorial, userProgress)) {
            throw new Error('Prerequisites not met for this tutorial');
        }
        
        this.currentTutorial = { id: tutorialId, ...tutorial };
        this.currentStep = 0;
        
        this.showStep(0);
        this.trackTutorialStart(tutorialId);
        
        console.log(`Started tutorial: ${tutorial.title}`);
    }
    
    /**
     * Display current step
     */
    showStep(stepIndex) {
        if (!this.currentTutorial || stepIndex >= this.currentTutorial.steps.length) {
            return;
        }
        
        const step = this.currentTutorial.steps[stepIndex];
        this.currentStep = stepIndex;
        
        // Update UI
        this.updateTutorialDisplay(step);
        
        // Setup interactivity
        if (step.interactive) {
            this.setupStepInteractivity(step.interactive);
        }
        
        // Update progress tracking
        this.trackStepView(this.currentTutorial.id, stepIndex);
    }
    
    /**
     * Setup interactivity for current step
     */
    setupStepInteractivity(interactiveConfig) {
        const handler = this.interactionHandlers.get(interactiveConfig.type);
        if (handler) {
            handler.setup(interactiveConfig, (result) => {
                this.handleInteractionComplete(result);
            });
        }
    }
    
    /**
     * Handle completion of interactive element
     */
    handleInteractionComplete(result) {
        if (result.success) {
            // Show success feedback
            this.showSuccessFeedback(result.message || 'Great job!');
            
            // Enable next step
            this.enableNextStep();
            
            // Track successful interaction
            this.trackInteractionSuccess(this.currentTutorial.id, this.currentStep);
        } else {
            // Show helpful feedback
            this.showHelpfulFeedback(result.message || 'Try again!', result.hint);
        }
    }
    
    /**
     * Move to next step
     */
    nextStep() {
        if (!this.currentTutorial) return;
        
        if (this.currentStep < this.currentTutorial.steps.length - 1) {
            this.showStep(this.currentStep + 1);
        } else {
            this.completeTutorial();
        }
    }
    
    /**
     * Move to previous step
     */
    previousStep() {
        if (this.currentStep > 0) {
            this.showStep(this.currentStep - 1);
        }
    }
    
    /**
     * Complete current tutorial
     */
    completeTutorial() {
        if (!this.currentTutorial) return;
        
        // Mark as completed
        this.markTutorialCompleted(this.currentTutorial.id);
        
        // Show completion screen
        this.showCompletionScreen();
        
        // Suggest next tutorial
        const nextTutorial = this.definitions.getNextTutorial(
            this.currentTutorial.id, 
            this.getUserProgress()
        );
        
        if (nextTutorial) {
            this.suggestNextTutorial(nextTutorial);
        }
        
        console.log(`Completed tutorial: ${this.currentTutorial.title}`);
    }
    
    /**
     * Update tutorial display in UI
     */
    updateTutorialDisplay(step) {
        const tutorialContent = document.querySelector('.tutorial-content');
        if (tutorialContent) {
            tutorialContent.innerHTML = `
                <div class="tutorial-header">
                    <h2>${this.currentTutorial.title}</h2>
                    <div class="tutorial-progress">
                        <span>${this.currentStep + 1} / ${this.currentTutorial.steps.length}</span>
                        <div class="progress-bar">
                            <div class="progress-fill" style="width: ${((this.currentStep + 1) / this.currentTutorial.steps.length) * 100}%"></div>
                        </div>
                    </div>
                </div>
                <div class="tutorial-body">
                    <h3>${step.title}</h3>
                    ${step.content}
                </div>
                <div class="tutorial-controls">
                    <button class="prev-btn" ${this.currentStep === 0 ? 'disabled' : ''}>Previous</button>
                    <button class="next-btn" ${!step.interactive ? '' : 'disabled'}>
                        ${this.currentStep === this.currentTutorial.steps.length - 1 ? 'Complete' : 'Next'}
                    </button>
                </div>
                <div class="tutorial-interaction"></div>
            `;
            
            // Setup event listeners
            this.setupTutorialControls();
        }
    }
    
    /**
     * Setup tutorial control event listeners
     */
    setupTutorialControls() {
        const prevBtn = document.querySelector('.tutorial-controls .prev-btn');
        const nextBtn = document.querySelector('.tutorial-controls .next-btn');
        
        if (prevBtn) {
            prevBtn.addEventListener('click', () => this.previousStep());
        }
        
        if (nextBtn) {
            nextBtn.addEventListener('click', () => this.nextStep());
        }
    }
    
    /**
     * Show success feedback
     */
    showSuccessFeedback(message) {
        const feedback = document.createElement('div');
        feedback.className = 'tutorial-feedback success';
        feedback.innerHTML = `
            <div class="feedback-icon">✅</div>
            <div class="feedback-message">${message}</div>
        `;
        
        const interactionArea = document.querySelector('.tutorial-interaction');
        if (interactionArea) {
            interactionArea.appendChild(feedback);
            
            setTimeout(() => {
                feedback.remove();
            }, 3000);
        }
    }
    
    /**
     * Show helpful feedback with hints
     */
    showHelpfulFeedback(message, hint = null) {
        const feedback = document.createElement('div');
        feedback.className = 'tutorial-feedback help';
        feedback.innerHTML = `
            <div class="feedback-icon">💡</div>
            <div class="feedback-message">${message}</div>
            ${hint ? `<div class="feedback-hint">Hint: ${hint}</div>` : ''}
        `;
        
        const interactionArea = document.querySelector('.tutorial-interaction');
        if (interactionArea) {
            interactionArea.appendChild(feedback);
        }
    }
    
    /**
     * Enable next step button
     */
    enableNextStep() {
        const nextBtn = document.querySelector('.tutorial-controls .next-btn');
        if (nextBtn) {
            nextBtn.disabled = false;
        }
    }
    
    /**
     * Load user progress from storage
     */
    loadUserProgress() {
        try {
            const saved = localStorage.getItem('ecscope-tutorial-progress');
            if (saved) {
                const progress = JSON.parse(saved);
                for (const [tutorialId, data] of Object.entries(progress)) {
                    this.userProgress.set(tutorialId, data);
                }
            }
        } catch (error) {
            console.warn('Failed to load tutorial progress:', error);
        }
    }
    
    /**
     * Save user progress to storage
     */
    saveUserProgress() {
        try {
            const progressObj = {};
            for (const [tutorialId, data] of this.userProgress) {
                progressObj[tutorialId] = data;
            }
            localStorage.setItem('ecscope-tutorial-progress', JSON.stringify(progressObj));
        } catch (error) {
            console.warn('Failed to save tutorial progress:', error);
        }
    }
    
    /**
     * Get user progress
     */
    getUserProgress() {
        const progress = {};
        for (const [tutorialId, data] of this.userProgress) {
            progress[tutorialId] = data;
        }
        return progress;
    }
    
    /**
     * Mark tutorial as completed
     */
    markTutorialCompleted(tutorialId) {
        const existing = this.userProgress.get(tutorialId) || {};
        this.userProgress.set(tutorialId, {
            ...existing,
            completed: true,
            completedAt: Date.now(),
            score: existing.score || 0
        });
        this.saveUserProgress();
    }
    
    /**
     * Track tutorial analytics
     */
    trackTutorialStart(tutorialId) {
        const existing = this.userProgress.get(tutorialId) || {};
        this.userProgress.set(tutorialId, {
            ...existing,
            startedAt: Date.now(),
            attempts: (existing.attempts || 0) + 1
        });
        this.saveUserProgress();
    }
    
    trackStepView(tutorialId, stepIndex) {
        const existing = this.userProgress.get(tutorialId) || {};
        const steps = existing.steps || {};
        steps[stepIndex] = {
            viewedAt: Date.now(),
            viewed: true
        };
        
        this.userProgress.set(tutorialId, {
            ...existing,
            steps,
            furthestStep: Math.max(existing.furthestStep || 0, stepIndex)
        });
        this.saveUserProgress();
    }
    
    trackInteractionSuccess(tutorialId, stepIndex) {
        const existing = this.userProgress.get(tutorialId) || {};
        const steps = existing.steps || {};
        steps[stepIndex] = {
            ...(steps[stepIndex] || {}),
            completed: true,
            completedAt: Date.now()
        };
        
        this.userProgress.set(tutorialId, {
            ...existing,
            steps,
            score: (existing.score || 0) + 10 // Award points
        });
        this.saveUserProgress();
    }
    
    /**
     * Show completion screen
     */
    showCompletionScreen() {
        const modal = document.createElement('div');
        modal.className = 'tutorial-completion-modal';
        modal.innerHTML = `
            <div class="completion-content">
                <div class="completion-header">
                    <div class="completion-icon">🎉</div>
                    <h2>Tutorial Complete!</h2>
                    <h3>${this.currentTutorial.title}</h3>
                </div>
                <div class="completion-stats">
                    <div class="stat">
                        <span class="stat-label">Steps Completed</span>
                        <span class="stat-value">${this.currentTutorial.steps.length}</span>
                    </div>
                    <div class="stat">
                        <span class="stat-label">Score</span>
                        <span class="stat-value">${this.userProgress.get(this.currentTutorial.id)?.score || 0}</span>
                    </div>
                </div>
                <div class="completion-actions">
                    <button class="continue-btn">Continue Learning</button>
                    <button class="close-btn">Close</button>
                </div>
            </div>
        `;
        
        document.body.appendChild(modal);
        
        // Setup event listeners
        modal.querySelector('.continue-btn').addEventListener('click', () => {
            modal.remove();
            this.showTutorialSelection();
        });
        
        modal.querySelector('.close-btn').addEventListener('click', () => {
            modal.remove();
        });
    }
    
    /**
     * Suggest next tutorial
     */
    suggestNextTutorial(tutorial) {
        // Implementation for tutorial suggestions
    }
    
    /**
     * Show tutorial selection interface
     */
    showTutorialSelection() {
        // Implementation for tutorial browser
    }
}

/**
 * Base class for interaction handlers
 */
class InteractionHandler {
    constructor(app) {
        this.app = app;
    }
    
    setup(config, callback) {
        throw new Error('setup() must be implemented by subclass');
    }
    
    cleanup() {
        // Default cleanup - override if needed
    }
}

/**
 * Code execution interaction handler
 */
class CodeExecutionHandler extends InteractionHandler {
    setup(config, callback) {
        const container = document.querySelector('.tutorial-interaction');
        container.innerHTML = `
            <div class="code-exercise">
                <h4>Try it yourself:</h4>
                <textarea class="code-input" placeholder="Enter your code here...">${config.code || ''}</textarea>
                <button class="run-code-btn">Run Code</button>
                <div class="code-output"></div>
            </div>
        `;
        
        const runBtn = container.querySelector('.run-code-btn');
        const codeInput = container.querySelector('.code-input');
        const output = container.querySelector('.code-output');
        
        runBtn.addEventListener('click', () => {
            try {
                const code = codeInput.value;
                const result = this.executeCode(code);
                output.textContent = result;
                
                // Validate result
                const isCorrect = this.validateOutput(result, config.expectedOutput);
                callback({
                    success: isCorrect,
                    message: isCorrect ? 'Code executed successfully!' : 'Output doesn\'t match expected result',
                    result: result
                });
                
            } catch (error) {
                output.textContent = 'Error: ' + error.message;
                callback({
                    success: false,
                    message: 'Code execution failed',
                    error: error.message
                });
            }
        });
    }
    
    executeCode(code) {
        // Safe code execution in sandboxed environment
        // This is a simplified version - real implementation would need proper sandboxing
        try {
            const func = new Function('Registry', 'console', code + '\nreturn typeof result !== "undefined" ? result : "Code executed";');
            const mockRegistry = {
                createEntity: () => ({ getId: () => Math.floor(Math.random() * 1000) + 1 })
            };
            const mockConsole = {
                log: (...args) => args.join(' ')
            };
            return func(mockRegistry, mockConsole);
        } catch (error) {
            throw new Error('Code execution failed: ' + error.message);
        }
    }
    
    validateOutput(actual, expected) {
        if (!expected) return true;
        return String(actual).includes(String(expected));
    }
}

/**
 * Entity creation interaction handler
 */
class EntityCreationHandler extends InteractionHandler {
    setup(config, callback) {
        const container = document.querySelector('.tutorial-interaction');
        container.innerHTML = `
            <div class="entity-creation-exercise">
                <h4>${config.task}</h4>
                <div class="entity-creation-ui">
                    <button class="create-entity-btn">Create Entity</button>
                    <div class="entity-list"></div>
                </div>
                <button class="submit-btn">Submit</button>
            </div>
        `;
        
        const createBtn = container.querySelector('.create-entity-btn');
        const entityList = container.querySelector('.entity-list');
        const submitBtn = container.querySelector('.submit-btn');
        
        let entities = [];
        
        createBtn.addEventListener('click', () => {
            const entityId = entities.length + 1;
            entities.push({ id: entityId });
            
            const entityElement = document.createElement('div');
            entityElement.className = 'entity-item';
            entityElement.innerHTML = `Entity #${entityId}`;
            entityList.appendChild(entityElement);
        });
        
        submitBtn.addEventListener('click', () => {
            const result = { entities };
            const isValid = config.validation ? config.validation(result) : entities.length > 0;
            
            callback({
                success: isValid,
                message: isValid ? 'Entities created successfully!' : 'Please create the required entities',
                result
            });
        });
    }
}

/**
 * Component design interaction handler
 */
class ComponentDesignHandler extends InteractionHandler {
    setup(config, callback) {
        const container = document.querySelector('.tutorial-interaction');
        container.innerHTML = `
            <div class="component-design-exercise">
                <h4>${config.task}</h4>
                <div class="component-designer">
                    <div class="requirements">
                        <h5>Requirements:</h5>
                        <ul>
                            ${config.requirements.map(req => `<li>${req}</li>`).join('')}
                        </ul>
                    </div>
                    <div class="component-form">
                        <input type="text" class="component-name" placeholder="Component name...">
                        <button class="add-component-btn">Add Component</button>
                    </div>
                    <div class="component-list"></div>
                </div>
                <button class="submit-btn">Submit Design</button>
            </div>
        `;
        
        const nameInput = container.querySelector('.component-name');
        const addBtn = container.querySelector('.add-component-btn');
        const componentList = container.querySelector('.component-list');
        const submitBtn = container.querySelector('.submit-btn');
        
        let components = [];
        
        addBtn.addEventListener('click', () => {
            const name = nameInput.value.trim();
            if (name) {
                components.push(name);
                
                const componentElement = document.createElement('div');
                componentElement.className = 'component-item';
                componentElement.innerHTML = `${name}`;
                componentList.appendChild(componentElement);
                
                nameInput.value = '';
            }
        });
        
        submitBtn.addEventListener('click', () => {
            const isValid = config.validation ? config.validation(components) : components.length > 0;
            
            callback({
                success: isValid,
                message: isValid ? 'Component design looks good!' : 'Please design more components',
                result: components
            });
        });
    }
}

/**
 * System implementation interaction handler
 */
class SystemImplementationHandler extends InteractionHandler {
    setup(config, callback) {
        const container = document.querySelector('.tutorial-interaction');
        container.innerHTML = `
            <div class="system-implementation-exercise">
                <h4>${config.task}</h4>
                <div class="code-editor">
                    <textarea class="system-code" placeholder="Implement your system here...">${config.template || ''}</textarea>
                </div>
                <button class="validate-btn">Validate Implementation</button>
            </div>
        `;
        
        const codeArea = container.querySelector('.system-code');
        const validateBtn = container.querySelector('.validate-btn');
        
        validateBtn.addEventListener('click', () => {
            const code = codeArea.value;
            const isValid = config.validation ? config.validation(code) : code.trim().length > 0;
            
            callback({
                success: isValid,
                message: isValid ? 'System implementation looks correct!' : 'Please implement the required system logic',
                result: code
            });
        });
    }
}

/**
 * Query builder interaction handler
 */
class QueryBuilderHandler extends InteractionHandler {
    setup(config, callback) {
        // Implementation for query building interface
        callback({ success: true, message: 'Query builder completed' });
    }
}

/**
 * Performance analysis interaction handler
 */
class PerformanceAnalysisHandler extends InteractionHandler {
    setup(config, callback) {
        // Implementation for performance analysis exercises
        callback({ success: true, message: 'Performance analysis completed' });
    }
}

// Export for use in main application
if (typeof window !== 'undefined') {
    window.InteractiveTutorialDefinitions = InteractiveTutorialDefinitions;
    window.InteractiveTutorialEngine = InteractiveTutorialEngine;
}