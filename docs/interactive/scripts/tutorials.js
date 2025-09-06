/**
 * ECScope Interactive Documentation - Tutorial System
 * Handles interactive tutorials with live code execution and step-by-step guidance
 */

class ECScope_TutorialSystem {
    constructor() {
        this.currentTutorial = null;
        this.currentStep = 0;
        this.tutorialData = {};
        this.codeExamples = {};
        this.executionResults = {};
        
        this.initialize();
    }

    initialize() {
        this.loadTutorialDefinitions();
        this.setupTutorialEventListeners();
    }

    setupTutorialEventListeners() {
        document.addEventListener('click', (e) => {
            if (e.target.matches('.tutorial-start')) {
                const tutorialId = e.target.getAttribute('data-tutorial');
                this.startTutorial(tutorialId);
            }

            if (e.target.matches('.tutorial-next')) {
                this.nextStep();
            }

            if (e.target.matches('.tutorial-prev')) {
                this.previousStep();
            }

            if (e.target.matches('.tutorial-reset')) {
                this.resetTutorial();
            }

            if (e.target.matches('.code-run')) {
                const codeBlock = e.target.closest('.tutorial-code-block');
                this.executeCode(codeBlock);
            }

            if (e.target.matches('.hint-toggle')) {
                this.toggleHint(e.target);
            }
        });
    }

    async loadTutorialDefinitions() {
        // Define tutorial content
        this.tutorialData = {
            'basic-ecs': {
                title: 'ECS Fundamentals',
                description: 'Learn the basics of Entity-Component-System architecture',
                estimatedTime: '15 minutes',
                difficulty: 'Beginner',
                prerequisites: ['Basic C++ knowledge'],
                steps: [
                    {
                        title: 'Understanding Entities',
                        content: this.generateEntityStep(),
                        code: this.getEntityExampleCode(),
                        interactiveElements: ['entity-counter', 'entity-visualizer']
                    },
                    {
                        title: 'Working with Components',
                        content: this.generateComponentStep(),
                        code: this.getComponentExampleCode(),
                        interactiveElements: ['component-editor', 'component-visualizer']
                    },
                    {
                        title: 'Creating Systems',
                        content: this.generateSystemStep(),
                        code: this.getSystemExampleCode(),
                        interactiveElements: ['system-profiler', 'system-scheduler']
                    },
                    {
                        title: 'Putting It All Together',
                        content: this.generateIntegrationStep(),
                        code: this.getIntegrationExampleCode(),
                        interactiveElements: ['full-ecs-demo']
                    }
                ]
            },
            'memory-lab': {
                title: 'Memory Management Laboratory',
                description: 'Explore memory allocation strategies and their performance impact',
                estimatedTime: '25 minutes',
                difficulty: 'Intermediate',
                prerequisites: ['ECS Fundamentals', 'Memory concepts'],
                steps: [
                    {
                        title: 'Memory Allocation Basics',
                        content: this.generateMemoryBasicsStep(),
                        code: this.getMemoryBasicsCode(),
                        interactiveElements: ['allocation-visualizer', 'memory-tracker']
                    },
                    {
                        title: 'Custom Allocators',
                        content: this.generateAllocatorStep(),
                        code: this.getAllocatorExampleCode(),
                        interactiveElements: ['allocator-comparison', 'performance-graph']
                    },
                    {
                        title: 'Cache Behavior Analysis',
                        content: this.generateCacheStep(),
                        code: this.getCacheExampleCode(),
                        interactiveElements: ['cache-simulator', 'access-pattern-viz']
                    },
                    {
                        title: 'Real-World Optimization',
                        content: this.generateOptimizationStep(),
                        code: this.getOptimizationCode(),
                        interactiveElements: ['benchmark-runner', 'optimization-suggestions']
                    }
                ]
            },
            'physics': {
                title: 'Physics System Deep Dive',
                description: 'Understand physics simulation in the ECS context',
                estimatedTime: '30 minutes',
                difficulty: 'Advanced',
                prerequisites: ['ECS Fundamentals', 'Basic physics'],
                steps: [
                    {
                        title: 'Physics Components',
                        content: this.generatePhysicsComponentsStep(),
                        code: this.getPhysicsComponentsCode(),
                        interactiveElements: ['physics-component-editor']
                    },
                    {
                        title: 'Collision Detection',
                        content: this.generateCollisionStep(),
                        code: this.getCollisionCode(),
                        interactiveElements: ['collision-visualizer', 'algorithm-stepper']
                    },
                    {
                        title: 'Constraint Solving',
                        content: this.generateConstraintStep(),
                        code: this.getConstraintCode(),
                        interactiveElements: ['constraint-editor', 'solver-visualizer']
                    },
                    {
                        title: 'Performance Optimization',
                        content: this.generatePhysicsOptimizationStep(),
                        code: this.getPhysicsOptimizationCode(),
                        interactiveElements: ['spatial-partitioning-viz', 'performance-profiler']
                    }
                ]
            }
        };
    }

    startTutorial(tutorialId) {
        const tutorial = this.tutorialData[tutorialId];
        if (!tutorial) {
            console.error(`Tutorial '${tutorialId}' not found`);
            return;
        }

        this.currentTutorial = tutorialId;
        this.currentStep = 0;
        
        this.displayTutorialInterface(tutorial);
        this.loadTutorialStep(0);
        
        // Track tutorial start
        this.trackTutorialEvent('tutorial_started', { tutorialId });
    }

    displayTutorialInterface(tutorial) {
        const container = document.getElementById(this.getTutorialContainerId());
        if (!container) return;

        container.innerHTML = `
            <div class="tutorial-interface">
                <div class="tutorial-header">
                    <div class="tutorial-info">
                        <h1 class="tutorial-title">${tutorial.title}</h1>
                        <p class="tutorial-description">${tutorial.description}</p>
                        <div class="tutorial-meta">
                            <span class="tutorial-time"><i class="fas fa-clock"></i> ${tutorial.estimatedTime}</span>
                            <span class="tutorial-difficulty ${tutorial.difficulty.toLowerCase()}">
                                <i class="fas fa-signal"></i> ${tutorial.difficulty}
                            </span>
                        </div>
                    </div>
                    <div class="tutorial-progress">
                        <div class="progress-bar">
                            <div class="progress-fill" style="width: 0%"></div>
                        </div>
                        <span class="progress-text">Step 1 of ${tutorial.steps.length}</span>
                    </div>
                </div>
                
                <div class="tutorial-body">
                    <div class="tutorial-navigation">
                        <div class="step-indicators">
                            ${tutorial.steps.map((step, index) => `
                                <div class="step-indicator ${index === 0 ? 'active' : ''}" data-step="${index}">
                                    <span class="step-number">${index + 1}</span>
                                    <span class="step-title">${step.title}</span>
                                </div>
                            `).join('')}
                        </div>
                    </div>
                    
                    <div class="tutorial-content">
                        <div id="tutorial-step-content" class="step-content"></div>
                        
                        <div class="tutorial-controls">
                            <button class="button tutorial-prev" disabled>
                                <i class="fas fa-arrow-left"></i> Previous
                            </button>
                            <button class="button tutorial-reset">
                                <i class="fas fa-redo"></i> Reset
                            </button>
                            <button class="button primary tutorial-next">
                                Next <i class="fas fa-arrow-right"></i>
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        `;

        // Setup step indicator clicks
        container.querySelectorAll('.step-indicator').forEach(indicator => {
            indicator.addEventListener('click', () => {
                const stepIndex = parseInt(indicator.getAttribute('data-step'));
                this.jumpToStep(stepIndex);
            });
        });
    }

    loadTutorialStep(stepIndex) {
        const tutorial = this.tutorialData[this.currentTutorial];
        const step = tutorial.steps[stepIndex];
        
        if (!step) return;

        this.currentStep = stepIndex;
        this.updateStepIndicators();
        this.updateProgressBar();
        this.displayStepContent(step);
        this.initializeStepInteractiveElements(step);
        
        // Update navigation buttons
        this.updateNavigationButtons();
    }

    displayStepContent(step) {
        const contentContainer = document.getElementById('tutorial-step-content');
        if (!contentContainer) return;

        contentContainer.innerHTML = `
            <div class="step-main">
                <h2 class="step-title">${step.title}</h2>
                <div class="step-description">
                    ${step.content}
                </div>
            </div>
            
            <div class="step-interactive">
                <div class="code-section">
                    <div class="code-header">
                        <h3>Interactive Code Example</h3>
                        <div class="code-controls">
                            <button class="button code-run">
                                <i class="fas fa-play"></i> Run Code
                            </button>
                            <button class="button hint-toggle">
                                <i class="fas fa-lightbulb"></i> Hint
                            </button>
                        </div>
                    </div>
                    
                    <div class="tutorial-code-block">
                        <div class="code-editor">
                            <pre><code class="language-cpp">${step.code}</code></pre>
                        </div>
                        <div class="code-output">
                            <div class="output-header">Output</div>
                            <div class="output-content" id="code-output-${this.currentStep}">
                                Click "Run Code" to see the results
                            </div>
                        </div>
                    </div>
                    
                    <div class="code-hint" style="display: none;">
                        ${this.getHintForStep(this.currentTutorial, this.currentStep)}
                    </div>
                </div>
                
                <div class="interactive-elements">
                    ${this.renderInteractiveElements(step.interactiveElements)}
                </div>
            </div>
        `;

        // Syntax highlighting
        if (typeof Prism !== 'undefined') {
            Prism.highlightAllUnder(contentContainer);
        }
    }

    renderInteractiveElements(elements) {
        if (!elements || elements.length === 0) return '';

        return elements.map(elementType => {
            switch (elementType) {
                case 'entity-counter':
                    return this.renderEntityCounter();
                case 'entity-visualizer':
                    return this.renderEntityVisualizer();
                case 'component-editor':
                    return this.renderComponentEditor();
                case 'system-profiler':
                    return this.renderSystemProfiler();
                case 'allocation-visualizer':
                    return this.renderAllocationVisualizer();
                case 'performance-graph':
                    return this.renderPerformanceGraph();
                case 'cache-simulator':
                    return this.renderCacheSimulator();
                case 'physics-component-editor':
                    return this.renderPhysicsComponentEditor();
                case 'collision-visualizer':
                    return this.renderCollisionVisualizer();
                default:
                    return `<div class="interactive-placeholder">${elementType} (Coming Soon)</div>`;
            }
        }).join('');
    }

    // Interactive Element Renderers
    renderEntityCounter() {
        return `
            <div class="interactive-widget entity-counter">
                <h4>Entity Counter</h4>
                <div class="counter-display">
                    <span class="entity-count">0</span>
                    <span class="entity-label">Entities</span>
                </div>
                <div class="counter-controls">
                    <button class="button" onclick="this.closest('.entity-counter').querySelector('.entity-count').textContent = parseInt(this.closest('.entity-counter').querySelector('.entity-count').textContent) + 1">
                        Add Entity
                    </button>
                    <button class="button" onclick="this.closest('.entity-counter').querySelector('.entity-count').textContent = Math.max(0, parseInt(this.closest('.entity-counter').querySelector('.entity-count').textContent) - 1)">
                        Remove Entity
                    </button>
                    <button class="button" onclick="this.closest('.entity-counter').querySelector('.entity-count').textContent = '0'">
                        Reset
                    </button>
                </div>
            </div>
        `;
    }

    renderEntityVisualizer() {
        return `
            <div class="interactive-widget entity-visualizer">
                <h4>Entity Visualization</h4>
                <div class="entity-canvas">
                    <svg width="400" height="200" viewBox="0 0 400 200">
                        <rect width="400" height="200" fill="var(--bg-secondary)" stroke="var(--border-light)"/>
                        <text x="200" y="100" text-anchor="middle" fill="var(--text-tertiary)">
                            Entities will appear here
                        </text>
                    </svg>
                </div>
            </div>
        `;
    }

    renderComponentEditor() {
        return `
            <div class="interactive-widget component-editor">
                <h4>Component Editor</h4>
                <div class="editor-interface">
                    <div class="component-list">
                        <div class="component-item active">
                            <span class="component-name">Position</span>
                            <span class="component-type">Vec3</span>
                        </div>
                        <div class="component-item">
                            <span class="component-name">Velocity</span>
                            <span class="component-type">Vec3</span>
                        </div>
                        <div class="component-item">
                            <span class="component-name">Health</span>
                            <span class="component-type">f32</span>
                        </div>
                    </div>
                    <div class="component-properties">
                        <div class="property">
                            <label>X:</label>
                            <input type="number" value="0" step="0.1">
                        </div>
                        <div class="property">
                            <label>Y:</label>
                            <input type="number" value="0" step="0.1">
                        </div>
                        <div class="property">
                            <label>Z:</label>
                            <input type="number" value="0" step="0.1">
                        </div>
                    </div>
                </div>
            </div>
        `;
    }

    renderAllocationVisualizer() {
        return `
            <div class="interactive-widget allocation-visualizer">
                <h4>Memory Allocation Visualizer</h4>
                <div class="memory-display">
                    <div class="allocator-comparison">
                        <div class="allocator-column">
                            <h5>Standard Allocator</h5>
                            <div class="memory-blocks standard">
                                <div class="memory-block allocated" style="width: 20%;" title="Object 1 (20 bytes)"></div>
                                <div class="memory-block free" style="width: 10%;" title="Free (10 bytes)"></div>
                                <div class="memory-block allocated" style="width: 30%;" title="Object 2 (30 bytes)"></div>
                                <div class="memory-block free" style="width: 40%;" title="Free (40 bytes)"></div>
                            </div>
                            <div class="stats">
                                <span>Fragmentation: 25%</span>
                                <span>Efficiency: 75%</span>
                            </div>
                        </div>
                        <div class="allocator-column">
                            <h5>Arena Allocator</h5>
                            <div class="memory-blocks arena">
                                <div class="memory-block allocated" style="width: 50%;" title="Allocated (50 bytes)"></div>
                                <div class="memory-block free" style="width: 50%;" title="Free (50 bytes)"></div>
                            </div>
                            <div class="stats">
                                <span>Fragmentation: 0%</span>
                                <span>Efficiency: 100%</span>
                            </div>
                        </div>
                    </div>
                </div>
                <div class="allocator-controls">
                    <button class="button" onclick="window.ecscopeDoc.tutorialSystem.simulateAllocation('standard')">
                        Allocate (Standard)
                    </button>
                    <button class="button" onclick="window.ecscopeDoc.tutorialSystem.simulateAllocation('arena')">
                        Allocate (Arena)
                    </button>
                    <button class="button" onclick="window.ecscopeDoc.tutorialSystem.resetAllocator()">
                        Reset
                    </button>
                </div>
            </div>
        `;
    }

    // Step Navigation
    nextStep() {
        const tutorial = this.tutorialData[this.currentTutorial];
        if (this.currentStep < tutorial.steps.length - 1) {
            this.loadTutorialStep(this.currentStep + 1);
            this.trackTutorialEvent('step_completed', { 
                tutorialId: this.currentTutorial, 
                step: this.currentStep 
            });
        } else {
            this.completeTutorial();
        }
    }

    previousStep() {
        if (this.currentStep > 0) {
            this.loadTutorialStep(this.currentStep - 1);
        }
    }

    jumpToStep(stepIndex) {
        const tutorial = this.tutorialData[this.currentTutorial];
        if (stepIndex >= 0 && stepIndex < tutorial.steps.length) {
            this.loadTutorialStep(stepIndex);
        }
    }

    resetTutorial() {
        this.loadTutorialStep(0);
        this.trackTutorialEvent('tutorial_reset', { tutorialId: this.currentTutorial });
    }

    completeTutorial() {
        this.trackTutorialEvent('tutorial_completed', { tutorialId: this.currentTutorial });
        
        const container = document.getElementById('tutorial-step-content');
        if (container) {
            container.innerHTML = `
                <div class="tutorial-completion">
                    <div class="completion-icon">
                        <i class="fas fa-trophy"></i>
                    </div>
                    <h2>Tutorial Complete!</h2>
                    <p>Congratulations! You've completed the ${this.tutorialData[this.currentTutorial].title} tutorial.</p>
                    
                    <div class="completion-stats">
                        <div class="stat">
                            <span class="stat-value">${this.tutorialData[this.currentTutorial].steps.length}</span>
                            <span class="stat-label">Steps Completed</span>
                        </div>
                        <div class="stat">
                            <span class="stat-value">${this.calculateCompletionTime()}</span>
                            <span class="stat-label">Time Spent</span>
                        </div>
                    </div>
                    
                    <div class="completion-actions">
                        <button class="button primary" onclick="window.ecscopeDoc.tutorialSystem.showNextTutorialSuggestions()">
                            What's Next?
                        </button>
                        <button class="button" onclick="window.ecscopeDoc.tutorialSystem.resetTutorial()">
                            Restart Tutorial
                        </button>
                    </div>
                </div>
            `;
        }
    }

    // Code Execution
    async executeCode(codeBlock) {
        const code = codeBlock.querySelector('code').textContent;
        const outputElement = codeBlock.querySelector('.output-content');
        
        outputElement.innerHTML = '<div class="execution-loading"><i class="fas fa-spinner fa-spin"></i> Executing...</div>';
        
        try {
            // Simulate code execution (in real implementation, this would compile and run C++)
            const result = await this.simulateCodeExecution(code);
            this.displayExecutionResult(outputElement, result);
        } catch (error) {
            this.displayExecutionError(outputElement, error);
        }
    }

    async simulateCodeExecution(code) {
        // Simulate execution delay
        await new Promise(resolve => setTimeout(resolve, 1000 + Math.random() * 1000));
        
        // Generate simulated output based on code content
        if (code.includes('Entity')) {
            return {
                success: true,
                output: 'Entity created with ID: 42\nComponents added successfully\nMemory usage: 64 bytes',
                metrics: {
                    executionTime: '0.5ms',
                    memoryUsed: '64 bytes',
                    entitiesCreated: 1
                }
            };
        } else if (code.includes('allocator')) {
            return {
                success: true,
                output: 'Arena allocator created\nAllocated 1000 entities in 0.2ms\nMemory efficiency: 95%',
                metrics: {
                    executionTime: '0.2ms',
                    memoryUsed: '1.2MB',
                    efficiency: '95%'
                }
            };
        } else {
            return {
                success: true,
                output: 'Code executed successfully\nNo output to display',
                metrics: {
                    executionTime: '1.2ms'
                }
            };
        }
    }

    displayExecutionResult(outputElement, result) {
        if (result.success) {
            outputElement.innerHTML = `
                <div class="execution-success">
                    <div class="output-text">${result.output.replace(/\n/g, '<br>')}</div>
                    ${result.metrics ? `
                        <div class="execution-metrics">
                            ${Object.entries(result.metrics).map(([key, value]) => 
                                `<span class="metric"><strong>${key}:</strong> ${value}</span>`
                            ).join('')}
                        </div>
                    ` : ''}
                </div>
            `;
        } else {
            this.displayExecutionError(outputElement, result.error);
        }
    }

    displayExecutionError(outputElement, error) {
        outputElement.innerHTML = `
            <div class="execution-error">
                <div class="error-message">
                    <i class="fas fa-exclamation-triangle"></i>
                    Error: ${error.message || error}
                </div>
                ${error.suggestions ? `
                    <div class="error-suggestions">
                        <strong>Suggestions:</strong>
                        <ul>${error.suggestions.map(s => `<li>${s}</li>`).join('')}</ul>
                    </div>
                ` : ''}
            </div>
        `;
    }

    // UI Updates
    updateStepIndicators() {
        const indicators = document.querySelectorAll('.step-indicator');
        indicators.forEach((indicator, index) => {
            indicator.classList.toggle('active', index === this.currentStep);
            indicator.classList.toggle('completed', index < this.currentStep);
        });
    }

    updateProgressBar() {
        const tutorial = this.tutorialData[this.currentTutorial];
        const progress = ((this.currentStep + 1) / tutorial.steps.length) * 100;
        
        const progressFill = document.querySelector('.progress-fill');
        const progressText = document.querySelector('.progress-text');
        
        if (progressFill) {
            progressFill.style.width = `${progress}%`;
        }
        
        if (progressText) {
            progressText.textContent = `Step ${this.currentStep + 1} of ${tutorial.steps.length}`;
        }
    }

    updateNavigationButtons() {
        const prevButton = document.querySelector('.tutorial-prev');
        const nextButton = document.querySelector('.tutorial-next');
        const tutorial = this.tutorialData[this.currentTutorial];
        
        if (prevButton) {
            prevButton.disabled = this.currentStep === 0;
        }
        
        if (nextButton) {
            if (this.currentStep === tutorial.steps.length - 1) {
                nextButton.innerHTML = 'Complete <i class="fas fa-check"></i>';
            } else {
                nextButton.innerHTML = 'Next <i class="fas fa-arrow-right"></i>';
            }
        }
    }

    // Utility Methods
    getTutorialContainerId() {
        return `tutorial-${this.currentTutorial}`;
    }

    calculateCompletionTime() {
        // In a real implementation, track actual time
        return '12 minutes';
    }

    trackTutorialEvent(event, data) {
        // Analytics tracking
        console.log('Tutorial Event:', event, data);
    }

    toggleHint(button) {
        const codeSection = button.closest('.code-section');
        const hint = codeSection.querySelector('.code-hint');
        
        if (hint.style.display === 'none') {
            hint.style.display = 'block';
            button.innerHTML = '<i class="fas fa-eye-slash"></i> Hide Hint';
        } else {
            hint.style.display = 'none';
            button.innerHTML = '<i class="fas fa-lightbulb"></i> Hint';
        }
    }

    // Content Generators (would be moved to separate files in production)
    generateBasicECSTutorial() {
        return `
            <section id="tutorial-basic-ecs" class="content-section">
                <h1 class="section-title">ECS Fundamentals Tutorial</h1>
                <div class="tutorial-intro">
                    <p>Welcome to the interactive Entity-Component-System tutorial! You'll learn the core concepts of ECS architecture through hands-on examples and live code execution.</p>
                </div>
                <div id="tutorial-basic-ecs-container"></div>
            </section>
        `;
    }

    generateMemoryLabTutorial() {
        return `
            <section id="tutorial-memory-lab" class="content-section">
                <h1 class="section-title">Memory Management Laboratory</h1>
                <div class="tutorial-intro">
                    <p>Explore advanced memory management techniques and see their real-world performance impact through interactive experiments.</p>
                </div>
                <div id="tutorial-memory-lab-container"></div>
            </section>
        `;
    }

    getEntityExampleCode() {
        return `#include <ecscope/ecs.hpp>

int main() {
    // Create ECS registry
    ecscope::ecs::Registry registry;
    
    // Create entities
    auto entity1 = registry.create_entity();
    auto entity2 = registry.create_entity();
    
    // Add components
    registry.add_component<Position>(entity1, {0.0f, 0.0f, 0.0f});
    registry.add_component<Velocity>(entity1, {1.0f, 0.0f, 0.0f});
    
    std::cout << "Created entities: " << entity1 << ", " << entity2 << std::endl;
    std::cout << "Entity 1 has Position: " << registry.has_component<Position>(entity1) << std::endl;
    
    return 0;
}`;
    }

    getHintForStep(tutorialId, stepIndex) {
        const hints = {
            'basic-ecs': [
                'Think of entities as simple IDs that tie components together.',
                'Components are pure data structures - no behavior!',
                'Systems contain all the logic and operate on component data.',
                'The registry manages the relationships between entities and components.'
            ],
            'memory-lab': [
                'Arena allocators eliminate fragmentation by allocating in order.',
                'Pool allocators are perfect for fixed-size objects.',
                'Cache locality is crucial for performance - keep related data together.',
                'Profile first, optimize second - measure the real bottlenecks.'
            ]
        };

        return hints[tutorialId]?.[stepIndex] || 'No hint available for this step.';
    }
}

// Initialize tutorial system
document.addEventListener('DOMContentLoaded', () => {
    if (window.ecscopeDoc) {
        window.ecscopeDoc.tutorialSystem = new ECScope_TutorialSystem();
    }
});

// Export for module systems
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ECScope_TutorialSystem;
}