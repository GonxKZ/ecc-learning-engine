// =============================================================================
// ECScope Demo System
// =============================================================================

class DemoSystem {
    constructor() {
        this.demos = new Map();
        this.currentDemo = null;
        this.isRunning = false;
        
        this.initializeDemos();
    }

    initializeDemos() {
        // Particle System Demo
        this.demos.set('particles', {
            title: 'Particle System Demo',
            description: 'Thousands of particles demonstrating ECS performance',
            canvas: null,
            context: null,
            particles: [],
            maxParticles: 5000,
            
            init: (container) => {
                const canvas = document.createElement('canvas');
                canvas.width = 800;
                canvas.height = 600;
                canvas.style.width = '100%';
                canvas.style.height = 'auto';
                canvas.style.background = '#1a1a2e';
                canvas.style.borderRadius = '8px';
                
                const ctx = canvas.getContext('2d');
                container.appendChild(canvas);
                
                this.demos.get('particles').canvas = canvas;
                this.demos.get('particles').context = ctx;
                
                // Initialize particles
                this.initializeParticles();
                
                // Add controls
                const controls = document.createElement('div');
                controls.style.cssText = 'margin-top: 1rem; display: flex; gap: 1rem; flex-wrap: wrap;';
                controls.innerHTML = `
                    <button id="demo-particles-start" class="btn btn-primary">Start</button>
                    <button id="demo-particles-stop" class="btn btn-secondary">Stop</button>
                    <button id="demo-particles-reset" class="btn btn-warning">Reset</button>
                    <label>Particles: 
                        <input type="range" id="particle-count-slider" min="100" max="10000" value="1000" step="100">
                        <span id="particle-count-display">1000</span>
                    </label>
                `;
                container.appendChild(controls);
                
                this.bindParticleControls();
            },
            
            update: (deltaTime) => {
                this.updateParticles(deltaTime);
            },
            
            render: () => {
                this.renderParticles();
            },
            
            cleanup: () => {
                const demo = this.demos.get('particles');
                demo.particles = [];
                if (demo.canvas) {
                    demo.context.clearRect(0, 0, demo.canvas.width, demo.canvas.height);
                }
            }
        });

        // Physics Playground Demo
        this.demos.set('physics', {
            title: 'Physics Playground',
            description: 'Interactive physics simulation with collision detection',
            canvas: null,
            context: null,
            bodies: [],
            
            init: (container) => {
                const canvas = document.createElement('canvas');
                canvas.width = 800;
                canvas.height = 600;
                canvas.style.width = '100%';
                canvas.style.height = 'auto';
                canvas.style.background = '#0f0f23';
                canvas.style.borderRadius = '8px';
                
                const ctx = canvas.getContext('2d');
                container.appendChild(canvas);
                
                this.demos.get('physics').canvas = canvas;
                this.demos.get('physics').context = ctx;
                
                // Add controls
                const controls = document.createElement('div');
                controls.style.cssText = 'margin-top: 1rem; display: flex; gap: 1rem; flex-wrap: wrap;';
                controls.innerHTML = `
                    <button id="demo-physics-box" class="btn btn-secondary">Add Box</button>
                    <button id="demo-physics-circle" class="btn btn-secondary">Add Circle</button>
                    <button id="demo-physics-stack" class="btn btn-primary">Create Stack</button>
                    <button id="demo-physics-explode" class="btn btn-warning">Explode!</button>
                    <button id="demo-physics-reset" class="btn btn-danger">Reset</button>
                `;
                container.appendChild(controls);
                
                this.bindPhysicsControls();
                this.initializePhysicsWorld();
            },
            
            update: (deltaTime) => {
                this.updatePhysics(deltaTime);
            },
            
            render: () => {
                this.renderPhysics();
            },
            
            cleanup: () => {
                const demo = this.demos.get('physics');
                demo.bodies = [];
                if (demo.canvas) {
                    demo.context.clearRect(0, 0, demo.canvas.width, demo.canvas.height);
                }
                
                if (window.ECScopeLoader && window.ECScopeLoader.physicsWorld) {
                    window.ECScopeLoader.physicsWorld.reset();
                }
            }
        });

        // Memory Benchmark Demo
        this.demos.set('memory', {
            title: 'Memory Benchmark',
            description: 'Memory allocation and performance testing',
            stats: {
                entitiesCreated: 0,
                memoryUsed: 0,
                allocationsPerSecond: 0
            },
            
            init: (container) => {
                const content = document.createElement('div');
                content.innerHTML = `
                    <div class="memory-demo-content">
                        <div class="memory-stats" style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1rem; margin-bottom: 1rem;">
                            <div class="stat-card" style="background: var(--background-color); padding: 1rem; border-radius: var(--border-radius);">
                                <h5>Entities Created</h5>
                                <div id="demo-entities-created" style="font-size: 2rem; font-weight: bold; color: var(--primary-color);">0</div>
                            </div>
                            <div class="stat-card" style="background: var(--background-color); padding: 1rem; border-radius: var(--border-radius);">
                                <h5>Memory Used</h5>
                                <div id="demo-memory-used" style="font-size: 2rem; font-weight: bold; color: var(--info-color);">0 KB</div>
                            </div>
                            <div class="stat-card" style="background: var(--background-color); padding: 1rem; border-radius: var(--border-radius);">
                                <h5>Allocations/sec</h5>
                                <div id="demo-alloc-rate" style="font-size: 2rem; font-weight: bold; color: var(--success-color);">0</div>
                            </div>
                        </div>
                        
                        <div class="memory-controls" style="display: flex; gap: 1rem; flex-wrap: wrap; margin-bottom: 1rem;">
                            <button id="demo-memory-create-1k" class="btn btn-primary">Create 1,000 Entities</button>
                            <button id="demo-memory-create-10k" class="btn btn-primary">Create 10,000 Entities</button>
                            <button id="demo-memory-stress-test" class="btn btn-warning">Stress Test</button>
                            <button id="demo-memory-clear" class="btn btn-danger">Clear All</button>
                        </div>
                        
                        <div class="memory-chart" style="background: var(--background-color); border-radius: var(--border-radius); padding: 1rem;">
                            <h5>Memory Usage Over Time</h5>
                            <canvas id="demo-memory-chart" width="600" height="200" style="width: 100%; height: auto;"></canvas>
                        </div>
                    </div>
                `;
                container.appendChild(content);
                
                this.bindMemoryControls();
                this.initializeMemoryChart();
            },
            
            update: (deltaTime) => {
                this.updateMemoryStats();
            },
            
            render: () => {
                this.renderMemoryChart();
            },
            
            cleanup: () => {
                const demo = this.demos.get('memory');
                demo.stats = {
                    entitiesCreated: 0,
                    memoryUsed: 0,
                    allocationsPerSecond: 0
                };
                
                // Clear all entities if possible
                if (window.ECScopeLoader && window.ECScopeLoader.registry) {
                    // This would require a clear method in the registry
                    console.log('Memory demo cleanup');
                }
            }
        });

        // ECS Visualizer Demo
        this.demos.set('ecs-viz', {
            title: 'ECS Visualizer',
            description: 'Visualize entity archetypes and relationships',
            canvas: null,
            context: null,
            
            init: (container) => {
                const canvas = document.createElement('canvas');
                canvas.width = 800;
                canvas.height = 600;
                canvas.style.width = '100%';
                canvas.style.height = 'auto';
                canvas.style.background = '#2d3748';
                canvas.style.borderRadius = '8px';
                
                const ctx = canvas.getContext('2d');
                container.appendChild(canvas);
                
                this.demos.get('ecs-viz').canvas = canvas;
                this.demos.get('ecs-viz').context = ctx;
                
                // Add controls
                const controls = document.createElement('div');
                controls.style.cssText = 'margin-top: 1rem; display: flex; gap: 1rem; flex-wrap: wrap;';
                controls.innerHTML = `
                    <button id="demo-viz-create-entities" class="btn btn-primary">Create Sample Entities</button>
                    <button id="demo-viz-show-archetypes" class="btn btn-secondary">Show Archetypes</button>
                    <button id="demo-viz-animate" class="btn btn-success">Animate</button>
                    <button id="demo-viz-clear" class="btn btn-danger">Clear</button>
                `;
                container.appendChild(controls);
                
                this.bindVisualizerControls();
            },
            
            update: (deltaTime) => {
                // Update visualization
            },
            
            render: () => {
                this.renderECSVisualization();
            },
            
            cleanup: () => {
                const demo = this.demos.get('ecs-viz');
                if (demo.canvas) {
                    demo.context.clearRect(0, 0, demo.canvas.width, demo.canvas.height);
                }
            }
        });
    }

    launchDemo(demoType) {
        if (this.currentDemo) {
            this.closeCurrentDemo();
        }

        const demo = this.demos.get(demoType);
        if (!demo) {
            console.error('Demo not found:', demoType);
            return;
        }

        this.currentDemo = demoType;
        
        // Show modal with demo
        const modal = document.getElementById('demo-modal');
        const title = document.getElementById('demo-title');
        const container = document.getElementById('demo-container');
        
        if (title) title.textContent = demo.title;
        if (container) container.innerHTML = '';
        
        // Initialize demo
        if (demo.init) {
            demo.init(container);
        }
        
        // Show modal
        if (modal) {
            modal.classList.add('active');
        }
        
        // Start demo loop
        this.startDemo();
        
        console.log('Launched demo:', demoType);
    }

    startDemo() {
        if (this.isRunning) return;
        
        this.isRunning = true;
        this.lastFrameTime = performance.now();
        this.demoLoop();
    }

    stopDemo() {
        this.isRunning = false;
    }

    demoLoop(currentTime = performance.now()) {
        if (!this.isRunning || !this.currentDemo) return;
        
        const deltaTime = (currentTime - this.lastFrameTime) / 1000.0;
        this.lastFrameTime = currentTime;
        
        const demo = this.demos.get(this.currentDemo);
        
        // Update demo
        if (demo.update) {
            demo.update(deltaTime);
        }
        
        // Render demo
        if (demo.render) {
            demo.render();
        }
        
        requestAnimationFrame((time) => this.demoLoop(time));
    }

    closeCurrentDemo() {
        if (!this.currentDemo) return;
        
        this.stopDemo();
        
        const demo = this.demos.get(this.currentDemo);
        if (demo.cleanup) {
            demo.cleanup();
        }
        
        this.currentDemo = null;
        
        // Hide modal
        const modal = document.getElementById('demo-modal');
        if (modal) {
            modal.classList.remove('active');
        }
    }

    // Particle System Implementation
    initializeParticles() {
        const demo = this.demos.get('particles');
        demo.particles = [];
        
        for (let i = 0; i < 1000; i++) {
            demo.particles.push(this.createParticle());
        }
    }

    createParticle() {
        const canvas = this.demos.get('particles').canvas;
        return {
            x: Math.random() * canvas.width,
            y: Math.random() * canvas.height,
            vx: (Math.random() - 0.5) * 200,
            vy: (Math.random() - 0.5) * 200,
            radius: 1 + Math.random() * 3,
            color: `hsl(${Math.random() * 360}, 70%, 60%)`,
            life: 1.0,
            decay: Math.random() * 0.5 + 0.5
        };
    }

    updateParticles(deltaTime) {
        const demo = this.demos.get('particles');
        const canvas = demo.canvas;
        
        for (let particle of demo.particles) {
            // Update position
            particle.x += particle.vx * deltaTime;
            particle.y += particle.vy * deltaTime;
            
            // Bounce off walls
            if (particle.x < 0 || particle.x > canvas.width) {
                particle.vx *= -0.8;
                particle.x = Math.max(0, Math.min(canvas.width, particle.x));
            }
            if (particle.y < 0 || particle.y > canvas.height) {
                particle.vy *= -0.8;
                particle.y = Math.max(0, Math.min(canvas.height, particle.y));
            }
            
            // Update life
            particle.life -= particle.decay * deltaTime;
            if (particle.life <= 0) {
                // Respawn particle
                Object.assign(particle, this.createParticle());
            }
        }
    }

    renderParticles() {
        const demo = this.demos.get('particles');
        const ctx = demo.context;
        const canvas = demo.canvas;
        
        // Clear canvas with fade effect
        ctx.fillStyle = 'rgba(26, 26, 46, 0.1)';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        // Draw particles
        for (let particle of demo.particles) {
            ctx.globalAlpha = particle.life;
            ctx.fillStyle = particle.color;
            ctx.beginPath();
            ctx.arc(particle.x, particle.y, particle.radius, 0, Math.PI * 2);
            ctx.fill();
        }
        
        ctx.globalAlpha = 1.0;
    }

    bindParticleControls() {
        document.getElementById('demo-particles-start')?.addEventListener('click', () => {
            this.startDemo();
        });
        
        document.getElementById('demo-particles-stop')?.addEventListener('click', () => {
            this.stopDemo();
        });
        
        document.getElementById('demo-particles-reset')?.addEventListener('click', () => {
            this.initializeParticles();
        });
        
        const slider = document.getElementById('particle-count-slider');
        const display = document.getElementById('particle-count-display');
        
        slider?.addEventListener('input', (e) => {
            const count = parseInt(e.target.value);
            display.textContent = count;
            
            const demo = this.demos.get('particles');
            demo.particles = [];
            for (let i = 0; i < count; i++) {
                demo.particles.push(this.createParticle());
            }
        });
    }

    // Physics Demo Implementation
    initializePhysicsWorld() {
        if (!window.ECScopeLoader || !window.ECScopeLoader.physicsWorld) {
            console.warn('Physics world not available');
            return;
        }
        
        const canvas = this.demos.get('physics').canvas;
        
        // Create ground
        window.ECScopeLoader.createPhysicsBox(canvas.width / 2, canvas.height - 20, canvas.width, 40, false);
        
        // Create some initial objects
        for (let i = 0; i < 5; i++) {
            window.ECScopeLoader.createPhysicsBox(
                200 + i * 50, 
                100 + i * 60, 
                20, 20, true
            );
        }
    }

    updatePhysics(deltaTime) {
        if (window.ECScopeLoader && window.ECScopeLoader.physicsWorld) {
            window.ECScopeLoader.stepPhysics(deltaTime);
        }
    }

    renderPhysics() {
        const demo = this.demos.get('physics');
        const ctx = demo.context;
        const canvas = demo.canvas;
        
        // Clear canvas
        ctx.fillStyle = '#0f0f23';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        if (!window.ECScopeLoader || !window.ECScopeLoader.physicsWorld) {
            ctx.fillStyle = 'white';
            ctx.font = '16px Arial';
            ctx.textAlign = 'center';
            ctx.fillText('Physics world not initialized', canvas.width / 2, canvas.height / 2);
            return;
        }
        
        // Get and render physics bodies
        try {
            const bodies = window.ECScopeLoader.getPhysicsBodies();
            
            for (let body of bodies) {
                const pos = body.position;
                
                ctx.fillStyle = body.type === 0 ? '#4CAF50' : '#2196F3'; // Static vs Dynamic
                ctx.fillRect(pos.x - 10, pos.y - 10, 20, 20);
            }
        } catch (error) {
            console.error('Physics rendering error:', error);
        }
    }

    bindPhysicsControls() {
        // Implementation for physics controls...
        console.log('Physics controls bound');
    }

    // Memory Demo Implementation  
    bindMemoryControls() {
        document.getElementById('demo-memory-create-1k')?.addEventListener('click', () => {
            this.createEntities(1000);
        });
        
        document.getElementById('demo-memory-create-10k')?.addEventListener('click', () => {
            this.createEntities(10000);
        });
        
        document.getElementById('demo-memory-stress-test')?.addEventListener('click', () => {
            this.runMemoryStressTest();
        });
        
        document.getElementById('demo-memory-clear')?.addEventListener('click', () => {
            this.clearAllEntities();
        });
    }

    createEntities(count) {
        if (!window.ECScopeLoader || !window.ECScopeLoader.registry) {
            window.showToast('ECS not initialized', 'error');
            return;
        }
        
        const startTime = performance.now();
        
        try {
            window.ECScopeLoader.registry.createManyEntities(count);
            
            const endTime = performance.now();
            const duration = endTime - startTime;
            
            window.showToast(`Created ${count} entities in ${duration.toFixed(1)}ms`, 'success');
        } catch (error) {
            window.showToast('Failed to create entities: ' + error.message, 'error');
        }
    }

    runMemoryStressTest() {
        window.showToast('Running memory stress test...', 'info');
        
        // Create entities in batches to avoid blocking
        let created = 0;
        const batchSize = 1000;
        const targetCount = 50000;
        
        const createBatch = () => {
            if (created < targetCount) {
                this.createEntities(batchSize);
                created += batchSize;
                
                setTimeout(createBatch, 10); // Small delay between batches
            } else {
                window.showToast('Memory stress test complete', 'success');
            }
        };
        
        createBatch();
    }

    clearAllEntities() {
        // This would require implementation in the ECS system
        window.showToast('Entity clearing not implemented yet', 'warning');
    }

    initializeMemoryChart() {
        // Simple memory chart implementation
        const canvas = document.getElementById('demo-memory-chart');
        if (canvas) {
            const ctx = canvas.getContext('2d');
            this.demos.get('memory').chartContext = ctx;
            this.demos.get('memory').memoryData = [];
        }
    }

    updateMemoryStats() {
        if (!window.ECScopeLoader) return;
        
        const stats = this.demos.get('memory').stats;
        stats.entitiesCreated = window.ECScopeLoader.getEntityCount();
        stats.memoryUsed = window.ECScopeLoader.getMemoryUsage();
        
        // Update UI
        const elements = {
            'demo-entities-created': stats.entitiesCreated,
            'demo-memory-used': (stats.memoryUsed / 1024).toFixed(1) + ' KB',
            'demo-alloc-rate': stats.allocationsPerSecond
        };
        
        Object.entries(elements).forEach(([id, value]) => {
            const element = document.getElementById(id);
            if (element) {
                element.textContent = value;
            }
        });
    }

    renderMemoryChart() {
        // Simple memory chart rendering
        const demo = this.demos.get('memory');
        if (!demo.chartContext) return;
        
        const ctx = demo.chartContext;
        const canvas = ctx.canvas;
        
        ctx.fillStyle = '#f8f9fa';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        // Add memory data point
        demo.memoryData.push(demo.stats.memoryUsed);
        if (demo.memoryData.length > 100) {
            demo.memoryData.shift();
        }
        
        // Draw memory usage line
        if (demo.memoryData.length > 1) {
            ctx.strokeStyle = '#17a2b8';
            ctx.lineWidth = 2;
            ctx.beginPath();
            
            const maxMemory = Math.max(...demo.memoryData, 1024);
            
            for (let i = 0; i < demo.memoryData.length; i++) {
                const x = (i / demo.memoryData.length) * canvas.width;
                const y = canvas.height - (demo.memoryData[i] / maxMemory) * canvas.height;
                
                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            }
            
            ctx.stroke();
        }
    }

    // ECS Visualizer Implementation
    bindVisualizerControls() {
        // Implementation for ECS visualizer controls...
        console.log('ECS visualizer controls bound');
    }

    renderECSVisualization() {
        const demo = this.demos.get('ecs-viz');
        const ctx = demo.context;
        const canvas = demo.canvas;
        
        // Clear canvas
        ctx.fillStyle = '#2d3748';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        
        // Simple visualization placeholder
        ctx.fillStyle = 'white';
        ctx.font = '16px Arial';
        ctx.textAlign = 'center';
        ctx.fillText('ECS Visualization Coming Soon', canvas.width / 2, canvas.height / 2);
    }
}

// Global demo system
window.demoSystem = new DemoSystem();

// Initialize demo system when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    // Bind demo launch buttons
    document.querySelectorAll('[data-demo]').forEach(btn => {
        btn.addEventListener('click', () => {
            const demoType = btn.dataset.demo;
            window.demoSystem.launchDemo(demoType);
        });
    });
    
    // Bind close demo button
    document.getElementById('close-demo')?.addEventListener('click', () => {
        window.demoSystem.closeCurrentDemo();
    });
});