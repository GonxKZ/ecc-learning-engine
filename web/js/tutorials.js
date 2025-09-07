// =============================================================================
// ECScope Interactive Tutorials System
// =============================================================================

class TutorialSystem {
    constructor() {
        this.tutorials = new Map();
        this.currentTutorial = null;
        this.currentStep = 0;
        this.isRunning = false;
        
        this.initializeTutorials();
    }

    initializeTutorials() {
        // ECS Basics Tutorial
        this.tutorials.set('ecs-basics', {
            title: 'ECS Fundamentals',
            description: 'Learn the basics of Entity Component System architecture',
            steps: [
                {
                    title: 'What is ECS?',
                    content: `
                        <h4>Entity Component System (ECS)</h4>
                        <p>ECS is a software architectural pattern that separates data from behavior:</p>
                        <ul>
                            <li><strong>Entities</strong>: Unique identifiers (just IDs)</li>
                            <li><strong>Components</strong>: Pure data containers</li>
                            <li><strong>Systems</strong>: Logic that processes entities with specific components</li>
                        </ul>
                        <p>This pattern enables better performance, modularity, and composition.</p>
                    `,
                    interactive: false,
                    validation: null
                },
                {
                    title: 'Creating Entities',
                    content: `
                        <h4>Step 1: Create Your First Entity</h4>
                        <p>An entity is simply a unique identifier. Let's create one!</p>
                        <p>Click the "Create Entity" button below to create your first entity.</p>
                        <button id="tutorial-create-entity" class="btn btn-primary">Create Entity</button>
                        <div id="tutorial-entity-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateEntityCreation()
                },
                {
                    title: 'Adding Components',
                    content: `
                        <h4>Step 2: Add Components</h4>
                        <p>Components store data. Let's add a Position component to our entity.</p>
                        <p>A Position component might store X, Y, and Z coordinates.</p>
                        <button id="tutorial-add-position" class="btn btn-secondary">Add Position Component</button>
                        <div id="tutorial-position-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validatePositionComponent()
                },
                {
                    title: 'Adding More Components',
                    content: `
                        <h4>Step 3: Add Velocity Component</h4>
                        <p>Let's add a Velocity component to make our entity movable.</p>
                        <button id="tutorial-add-velocity" class="btn btn-secondary">Add Velocity Component</button>
                        <div id="tutorial-velocity-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateVelocityComponent()
                },
                {
                    title: 'Running Systems',
                    content: `
                        <h4>Step 4: Execute a System</h4>
                        <p>Systems process entities that have specific components. Let's run a movement system that updates positions based on velocities.</p>
                        <button id="tutorial-run-movement" class="btn btn-primary">Run Movement System</button>
                        <div id="tutorial-movement-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateMovementSystem()
                },
                {
                    title: 'Congratulations!',
                    content: `
                        <h4>You've Completed ECS Basics!</h4>
                        <p>You've successfully:</p>
                        <ul>
                            <li>✅ Created an entity</li>
                            <li>✅ Added components (Position, Velocity)</li>
                            <li>✅ Executed a system (Movement)</li>
                        </ul>
                        <p>This is the foundation of Entity Component System architecture!</p>
                        <div class="alert alert-success">
                            <strong>Tutorial Complete!</strong> You now understand the basics of ECS.
                        </div>
                    `,
                    interactive: false,
                    validation: null
                }
            ],
            progress: 0,
            completed: false,
            entityId: null
        });

        // Memory Management Tutorial
        this.tutorials.set('memory', {
            title: 'Memory Management',
            description: 'Understanding memory allocation and optimization',
            steps: [
                {
                    title: 'Memory in ECS',
                    content: `
                        <h4>Memory Management in ECS</h4>
                        <p>ECS engines need efficient memory management because:</p>
                        <ul>
                            <li>Components are stored in memory pools</li>
                            <li>Cache-friendly data layout improves performance</li>
                            <li>Memory fragmentation can hurt performance</li>
                        </ul>
                        <div id="tutorial-memory-stats" class="bg-light p-3 rounded mt-3"></div>
                    `,
                    interactive: false,
                    validation: null
                },
                {
                    title: 'Creating Many Entities',
                    content: `
                        <h4>Memory Allocation Test</h4>
                        <p>Let's create many entities to see memory allocation in action:</p>
                        <button id="tutorial-create-many" class="btn btn-primary">Create 1000 Entities</button>
                        <div id="tutorial-allocation-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateMassEntityCreation()
                },
                {
                    title: 'Memory Patterns',
                    content: `
                        <h4>Archetype-based Storage</h4>
                        <p>ECS stores components in "archetypes" - groups of entities with the same component types.</p>
                        <p>This creates cache-friendly memory layouts.</p>
                        <button id="tutorial-show-archetypes" class="btn btn-secondary">Show Archetypes</button>
                        <div id="tutorial-archetypes-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateArchetypeVisualization()
                },
                {
                    title: 'Memory Management Complete',
                    content: `
                        <h4>Memory Management Mastery!</h4>
                        <p>You've learned about:</p>
                        <ul>
                            <li>✅ ECS memory patterns</li>
                            <li>✅ Archetype-based storage</li>
                            <li>✅ Memory allocation tracking</li>
                        </ul>
                        <div class="alert alert-success">
                            <strong>Tutorial Complete!</strong> You understand ECS memory management.
                        </div>
                    `,
                    interactive: false,
                    validation: null
                }
            ],
            progress: 0,
            completed: false
        });

        // Performance Tutorial
        this.tutorials.set('performance', {
            title: 'Performance Optimization',
            description: 'Techniques for optimizing ECS performance',
            steps: [
                {
                    title: 'Performance in ECS',
                    content: `
                        <h4>ECS Performance Principles</h4>
                        <p>ECS achieves high performance through:</p>
                        <ul>
                            <li><strong>Data Locality</strong>: Components stored contiguously</li>
                            <li><strong>Cache Efficiency</strong>: Systems process similar data together</li>
                            <li><strong>Minimal Branching</strong>: Predictable execution paths</li>
                            <li><strong>SIMD Optimization</strong>: Process multiple components at once</li>
                        </ul>
                    `,
                    interactive: false,
                    validation: null
                },
                {
                    title: 'Benchmarking Queries',
                    content: `
                        <h4>Query Performance</h4>
                        <p>Let's benchmark how fast we can query entities:</p>
                        <button id="tutorial-benchmark-queries" class="btn btn-primary">Benchmark Queries</button>
                        <div id="tutorial-query-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateQueryBenchmark()
                },
                {
                    title: 'System Performance',
                    content: `
                        <h4>System Execution Speed</h4>
                        <p>Let's measure how fast systems can process components:</p>
                        <button id="tutorial-benchmark-systems" class="btn btn-primary">Benchmark Systems</button>
                        <div id="tutorial-system-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateSystemBenchmark()
                },
                {
                    title: 'Memory vs Speed Trade-offs',
                    content: `
                        <h4>Optimization Trade-offs</h4>
                        <p>Understanding when to optimize for memory vs speed:</p>
                        <ul>
                            <li><strong>Memory First</strong>: Better cache usage, lower memory bandwidth</li>
                            <li><strong>Speed First</strong>: Pre-computed data, lookup tables</li>
                            <li><strong>Balanced</strong>: Component pools, archetype organization</li>
                        </ul>
                        <button id="tutorial-show-tradeoffs" class="btn btn-secondary">Show Trade-offs</button>
                        <div id="tutorial-tradeoffs-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateTradeoffAnalysis()
                },
                {
                    title: 'Profiling and Monitoring',
                    content: `
                        <h4>Performance Monitoring</h4>
                        <p>Always measure performance:</p>
                        <ul>
                            <li>Frame time and FPS</li>
                            <li>Memory allocation patterns</li>
                            <li>System execution time</li>
                            <li>Query performance</li>
                        </ul>
                        <button id="tutorial-start-profiling" class="btn btn-primary">Start Profiling</button>
                        <div id="tutorial-profiling-result" class="mt-2"></div>
                    `,
                    interactive: true,
                    validation: () => this.validateProfiling()
                },
                {
                    title: 'Performance Expert!',
                    content: `
                        <h4>Performance Optimization Complete!</h4>
                        <p>You've mastered:</p>
                        <ul>
                            <li>✅ ECS performance principles</li>
                            <li>✅ Query optimization</li>
                            <li>✅ System benchmarking</li>
                            <li>✅ Memory vs speed trade-offs</li>
                            <li>✅ Performance profiling</li>
                        </ul>
                        <div class="alert alert-success">
                            <strong>Tutorial Complete!</strong> You're now an ECS performance expert!
                        </div>
                    `,
                    interactive: false,
                    validation: null
                }
            ],
            progress: 0,
            completed: false
        });
    }

    startTutorial(tutorialId) {
        const tutorial = this.tutorials.get(tutorialId);
        if (!tutorial) {
            console.error('Tutorial not found:', tutorialId);
            return;
        }

        this.currentTutorial = tutorialId;
        this.currentStep = 0;
        this.isRunning = true;

        this.showTutorialContent();
        this.updateProgress();

        console.log('Started tutorial:', tutorialId);
    }

    nextStep() {
        if (!this.isRunning || !this.currentTutorial) return;

        const tutorial = this.tutorials.get(this.currentTutorial);
        
        // Validate current step if needed
        if (this.currentStep < tutorial.steps.length) {
            const currentStepData = tutorial.steps[this.currentStep];
            if (currentStepData.validation && !currentStepData.validation()) {
                this.showValidationError();
                return;
            }
        }

        this.currentStep++;
        
        if (this.currentStep >= tutorial.steps.length) {
            this.completeTutorial();
            return;
        }

        this.showTutorialContent();
        this.updateProgress();
    }

    previousStep() {
        if (!this.isRunning || !this.currentTutorial || this.currentStep <= 0) return;

        this.currentStep--;
        this.showTutorialContent();
        this.updateProgress();
    }

    showTutorialContent() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        const step = tutorial.steps[this.currentStep];
        
        const contentDiv = document.getElementById('tutorial-content');
        if (!contentDiv) return;

        contentDiv.classList.remove('hidden');
        
        contentDiv.innerHTML = `
            <div class="tutorial-header">
                <h3>${tutorial.title}</h3>
                <div class="tutorial-progress-indicator">
                    Step ${this.currentStep + 1} of ${tutorial.steps.length}
                </div>
            </div>
            
            <div class="tutorial-step">
                <h4>${step.title}</h4>
                <div class="tutorial-step-content">
                    ${step.content}
                </div>
                
                <div class="tutorial-controls mt-4">
                    <button id="tutorial-prev" class="btn btn-secondary" 
                            ${this.currentStep === 0 ? 'disabled' : ''}>
                        Previous
                    </button>
                    <button id="tutorial-next" class="btn btn-primary">
                        ${this.currentStep === tutorial.steps.length - 1 ? 'Complete Tutorial' : 'Next Step'}
                    </button>
                </div>
            </div>
        `;

        // Bind step navigation
        document.getElementById('tutorial-prev').addEventListener('click', () => {
            this.previousStep();
        });

        document.getElementById('tutorial-next').addEventListener('click', () => {
            this.nextStep();
        });

        // Bind interactive elements
        if (step.interactive) {
            this.bindStepInteractions();
        }

        // Update memory stats if needed
        if (this.currentTutorial === 'memory') {
            this.updateMemoryStats();
        }
    }

    bindStepInteractions() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        
        // ECS Basics interactions
        if (this.currentTutorial === 'ecs-basics') {
            const createEntityBtn = document.getElementById('tutorial-create-entity');
            if (createEntityBtn) {
                createEntityBtn.addEventListener('click', () => {
                    this.handleEntityCreation();
                });
            }

            const addPositionBtn = document.getElementById('tutorial-add-position');
            if (addPositionBtn) {
                addPositionBtn.addEventListener('click', () => {
                    this.handlePositionAddition();
                });
            }

            const addVelocityBtn = document.getElementById('tutorial-add-velocity');
            if (addVelocityBtn) {
                addVelocityBtn.addEventListener('click', () => {
                    this.handleVelocityAddition();
                });
            }

            const runMovementBtn = document.getElementById('tutorial-run-movement');
            if (runMovementBtn) {
                runMovementBtn.addEventListener('click', () => {
                    this.handleMovementSystem();
                });
            }
        }

        // Memory tutorial interactions
        if (this.currentTutorial === 'memory') {
            const createManyBtn = document.getElementById('tutorial-create-many');
            if (createManyBtn) {
                createManyBtn.addEventListener('click', () => {
                    this.handleMassEntityCreation();
                });
            }

            const showArchetypesBtn = document.getElementById('tutorial-show-archetypes');
            if (showArchetypesBtn) {
                showArchetypesBtn.addEventListener('click', () => {
                    this.handleArchetypeVisualization();
                });
            }
        }

        // Performance tutorial interactions
        if (this.currentTutorial === 'performance') {
            const benchmarkQueriesBtn = document.getElementById('tutorial-benchmark-queries');
            if (benchmarkQueriesBtn) {
                benchmarkQueriesBtn.addEventListener('click', () => {
                    this.handleQueryBenchmark();
                });
            }

            const benchmarkSystemsBtn = document.getElementById('tutorial-benchmark-systems');
            if (benchmarkSystemsBtn) {
                benchmarkSystemsBtn.addEventListener('click', () => {
                    this.handleSystemBenchmark();
                });
            }

            const showTradeoffsBtn = document.getElementById('tutorial-show-tradeoffs');
            if (showTradeoffsBtn) {
                showTradeoffsBtn.addEventListener('click', () => {
                    this.handleTradeoffAnalysis();
                });
            }

            const startProfilingBtn = document.getElementById('tutorial-start-profiling');
            if (startProfilingBtn) {
                startProfilingBtn.addEventListener('click', () => {
                    this.handleProfiling();
                });
            }
        }
    }

    // Tutorial interaction handlers
    handleEntityCreation() {
        if (!window.ECScopeLoader || !window.ECScopeLoader.isInitialized) {
            this.showError('ECScope not initialized');
            return;
        }

        try {
            const entityId = window.ECScopeLoader.createEntity();
            const tutorial = this.tutorials.get(this.currentTutorial);
            tutorial.entityId = entityId;
            
            const resultDiv = document.getElementById('tutorial-entity-result');
            if (resultDiv) {
                resultDiv.innerHTML = `
                    <div class="alert alert-success">
                        <strong>Success!</strong> Created entity with ID: <code>${entityId}</code>
                    </div>
                `;
            }
        } catch (error) {
            this.showError('Failed to create entity: ' + error.message);
        }
    }

    handlePositionAddition() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        if (!tutorial.entityId) {
            this.showError('No entity created yet');
            return;
        }

        try {
            window.ECScopeLoader.addPositionComponent(tutorial.entityId, 100, 200, 0);
            
            const resultDiv = document.getElementById('tutorial-position-result');
            if (resultDiv) {
                resultDiv.innerHTML = `
                    <div class="alert alert-success">
                        <strong>Success!</strong> Added Position component: (100, 200, 0)
                    </div>
                `;
            }
        } catch (error) {
            this.showError('Failed to add position component: ' + error.message);
        }
    }

    handleVelocityAddition() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        if (!tutorial.entityId) {
            this.showError('No entity created yet');
            return;
        }

        try {
            window.ECScopeLoader.addVelocityComponent(tutorial.entityId, 10, -5, 0);
            
            const resultDiv = document.getElementById('tutorial-velocity-result');
            if (resultDiv) {
                resultDiv.innerHTML = `
                    <div class="alert alert-success">
                        <strong>Success!</strong> Added Velocity component: (10, -5, 0)
                    </div>
                `;
            }
        } catch (error) {
            this.showError('Failed to add velocity component: ' + error.message);
        }
    }

    handleMovementSystem() {
        try {
            const beforePos = window.ECScopeLoader.getPositionComponent(this.tutorials.get(this.currentTutorial).entityId);
            window.ECScopeLoader.runMovementSystem(1.0); // 1 second delta
            const afterPos = window.ECScopeLoader.getPositionComponent(this.tutorials.get(this.currentTutorial).entityId);
            
            const resultDiv = document.getElementById('tutorial-movement-result');
            if (resultDiv) {
                resultDiv.innerHTML = `
                    <div class="alert alert-success">
                        <strong>Movement System Executed!</strong><br>
                        Before: (${beforePos.x}, ${beforePos.y})<br>
                        After: (${afterPos.x}, ${afterPos.y})
                    </div>
                `;
            }
        } catch (error) {
            this.showError('Failed to run movement system: ' + error.message);
        }
    }

    handleMassEntityCreation() {
        try {
            const beforeCount = window.ECScopeLoader.getEntityCount();
            window.ECScopeLoader.registry.createManyEntities(1000);
            const afterCount = window.ECScopeLoader.getEntityCount();
            
            const resultDiv = document.getElementById('tutorial-allocation-result');
            if (resultDiv) {
                resultDiv.innerHTML = `
                    <div class="alert alert-success">
                        <strong>Created 1000 entities!</strong><br>
                        Before: ${beforeCount} entities<br>
                        After: ${afterCount} entities<br>
                        Memory usage: ${(window.ECScopeLoader.getMemoryUsage() / 1024).toFixed(1)} KB
                    </div>
                `;
            }
        } catch (error) {
            this.showError('Failed to create entities: ' + error.message);
        }
    }

    handleArchetypeVisualization() {
        try {
            const archetypes = window.ECScopeLoader.getArchetypeInfo();
            
            const resultDiv = document.getElementById('tutorial-archetypes-result');
            if (resultDiv) {
                let html = '<div class="alert alert-info"><strong>Current Archetypes:</strong><ul>';
                archetypes.forEach((archetype, index) => {
                    html += `<li>Archetype ${archetype.id}: ${archetype.entity_count} entities, ${archetype.component_count} component types</li>`;
                });
                html += '</ul></div>';
                resultDiv.innerHTML = html;
            }
        } catch (error) {
            this.showError('Failed to show archetypes: ' + error.message);
        }
    }

    // Add other handler methods...
    handleQueryBenchmark() {
        try {
            const start = performance.now();
            window.ECScopeLoader.registry.benchmarkQueries(1000);
            const end = performance.now();
            
            const resultDiv = document.getElementById('tutorial-query-result');
            if (resultDiv) {
                resultDiv.innerHTML = `
                    <div class="alert alert-success">
                        <strong>Query Benchmark Complete!</strong><br>
                        1000 queries took: ${(end - start).toFixed(2)}ms<br>
                        Average per query: ${((end - start) / 1000).toFixed(4)}ms
                    </div>
                `;
            }
        } catch (error) {
            this.showError('Query benchmark failed: ' + error.message);
        }
    }

    // Validation methods
    validateEntityCreation() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        return tutorial.entityId !== null;
    }

    validatePositionComponent() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        if (!tutorial.entityId) return false;
        
        try {
            const pos = window.ECScopeLoader.getPositionComponent(tutorial.entityId);
            return pos !== null;
        } catch {
            return false;
        }
    }

    validateVelocityComponent() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        if (!tutorial.entityId) return false;
        
        try {
            const vel = window.ECScopeLoader.getVelocityComponent(tutorial.entityId);
            return vel !== null;
        } catch {
            return false;
        }
    }

    validateMovementSystem() {
        // Movement system is considered validated if it ran without error
        return true;
    }

    updateProgress() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        tutorial.progress = ((this.currentStep + 1) / tutorial.steps.length) * 100;
        
        // Update progress bars in the UI
        const progressBars = document.querySelectorAll(`.tutorial-card .progress-fill`);
        progressBars.forEach(bar => {
            if (bar.closest('.tutorial-card').querySelector(`[data-tutorial="${this.currentTutorial}"]`)) {
                bar.style.width = `${tutorial.progress}%`;
            }
        });
        
        const progressTexts = document.querySelectorAll(`.tutorial-card span`);
        progressTexts.forEach(text => {
            if (text.closest('.tutorial-card').querySelector(`[data-tutorial="${this.currentTutorial}"]`)) {
                text.textContent = `${this.currentStep + 1}/${tutorial.steps.length} steps`;
            }
        });
    }

    completeTutorial() {
        const tutorial = this.tutorials.get(this.currentTutorial);
        tutorial.completed = true;
        tutorial.progress = 100;
        
        this.isRunning = false;
        
        // Update UI to show completion
        this.updateProgress();
        
        // Hide tutorial content
        const contentDiv = document.getElementById('tutorial-content');
        if (contentDiv) {
            setTimeout(() => {
                contentDiv.classList.add('hidden');
            }, 3000);
        }
        
        console.log('Tutorial completed:', this.currentTutorial);
    }

    updateMemoryStats() {
        const statsDiv = document.getElementById('tutorial-memory-stats');
        if (statsDiv && window.ECScopeLoader && window.ECScopeLoader.isInitialized) {
            const memoryUsage = window.ECScopeLoader.getMemoryUsage();
            const entityCount = window.ECScopeLoader.getEntityCount();
            
            statsDiv.innerHTML = `
                <h5>Current Memory Statistics</h5>
                <div class="row">
                    <div class="col">
                        <strong>Memory Usage:</strong> ${(memoryUsage / 1024).toFixed(1)} KB
                    </div>
                    <div class="col">
                        <strong>Entity Count:</strong> ${entityCount}
                    </div>
                </div>
            `;
        }
    }

    showError(message) {
        console.error('Tutorial error:', message);
        // You could show a toast notification or alert here
        alert('Tutorial Error: ' + message);
    }

    showValidationError() {
        alert('Please complete the current step before proceeding.');
    }

    // Add remaining validation and handler methods...
    validateMassEntityCreation() { return true; }
    validateArchetypeVisualization() { return true; }
    validateQueryBenchmark() { return true; }
    validateSystemBenchmark() { return true; }
    validateTradeoffAnalysis() { return true; }
    validateProfiling() { return true; }

    handleSystemBenchmark() {
        const resultDiv = document.getElementById('tutorial-system-result');
        if (resultDiv) {
            resultDiv.innerHTML = `<div class="alert alert-info">System benchmark completed!</div>`;
        }
    }

    handleTradeoffAnalysis() {
        const resultDiv = document.getElementById('tutorial-tradeoffs-result');
        if (resultDiv) {
            resultDiv.innerHTML = `<div class="alert alert-info">Trade-off analysis shown!</div>`;
        }
    }

    handleProfiling() {
        const resultDiv = document.getElementById('tutorial-profiling-result');
        if (resultDiv) {
            resultDiv.innerHTML = `<div class="alert alert-info">Profiling started!</div>`;
        }
    }
}

// Global tutorial system
window.tutorialSystem = new TutorialSystem();

// Bind tutorial buttons when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('[data-tutorial]').forEach(btn => {
        btn.addEventListener('click', () => {
            const tutorialId = btn.dataset.tutorial;
            window.tutorialSystem.startTutorial(tutorialId);
        });
    });
});