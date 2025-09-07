// =============================================================================
// ECScope WebAssembly Loader
// =============================================================================

class ECScopeWasmLoader {
    constructor() {
        this.module = null;
        this.isInitialized = false;
        this.loadingProgress = 0;
        this.registry = null;
        this.physicsWorld = null;
        this.performanceMonitor = null;
        this.tutorial = null;
        
        this.onProgress = null;
        this.onLoaded = null;
        this.onError = null;
        this.onInitialized = null;
    }

    async load() {
        try {
            this.updateProgress(10, 'Loading WebAssembly module...');
            
            // Import the ECScope WebAssembly module
            const ECScope = await import('../ecscope_web.js');
            
            this.updateProgress(30, 'Initializing WebAssembly...');
            
            // Initialize the module
            this.module = await ECScope.default({
                onRuntimeInitialized: () => {
                    this.updateProgress(60, 'Runtime initialized...');
                    this.initializeComponents();
                },
                onProgress: (progress) => {
                    const percent = Math.max(30, Math.min(50, 30 + progress * 20));
                    this.updateProgress(percent, 'Loading WebAssembly...');
                },
                print: (text) => {
                    console.log('[ECScope]', text);
                },
                printErr: (text) => {
                    console.error('[ECScope Error]', text);
                }
            });
            
        } catch (error) {
            console.error('Failed to load ECScope WebAssembly:', error);
            if (this.onError) {
                this.onError(error);
            }
            throw error;
        }
    }

    initializeComponents() {
        try {
            this.updateProgress(70, 'Creating ECS components...');
            
            // Create main ECS registry
            this.registry = new this.module.ECSRegistry();
            
            this.updateProgress(80, 'Initializing physics...');
            
            // Create physics world
            this.physicsWorld = new this.module.PhysicsWorld();
            
            this.updateProgress(85, 'Setting up performance monitoring...');
            
            // Create performance monitor
            this.performanceMonitor = new this.module.ECSPerformanceMonitor();
            
            this.updateProgress(90, 'Creating tutorial system...');
            
            // Create tutorial system
            this.tutorial = new this.module.ECSTutorial();
            
            this.updateProgress(95, 'Initializing core systems...');
            
            // Initialize the core WebAssembly systems
            const config = JSON.stringify({
                enable_graphics: true,
                enable_physics: true,
                enable_simd: this.checkSIMDSupport(),
                enable_threading: this.checkThreadingSupport()
            });
            
            const coreInitialized = this.module._ecscope_wasm_initialize(
                this.module.stringToUTF8(config)
            );
            
            if (!coreInitialized) {
                throw new Error('Failed to initialize ECScope core systems');
            }
            
            this.isInitialized = true;
            this.updateProgress(100, 'ECScope WebAssembly ready!');
            
            if (this.onLoaded) {
                this.onLoaded();
            }
            
            if (this.onInitialized) {
                this.onInitialized();
            }
            
        } catch (error) {
            console.error('Failed to initialize ECScope components:', error);
            if (this.onError) {
                this.onError(error);
            }
            throw error;
        }
    }

    updateProgress(percent, status) {
        this.loadingProgress = percent;
        if (this.onProgress) {
            this.onProgress(percent, status);
        }
    }

    checkSIMDSupport() {
        try {
            // Check for WebAssembly SIMD support
            return typeof WebAssembly !== 'undefined' && 
                   WebAssembly.validate(new Uint8Array([
                       0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
                       0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7b,
                       0x03, 0x02, 0x01, 0x00,
                       0x0a, 0x0a, 0x01, 0x08, 0x00, 0xfd, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x0b
                   ]));
        } catch (e) {
            return false;
        }
    }

    checkThreadingSupport() {
        return typeof SharedArrayBuffer !== 'undefined' && 
               typeof Worker !== 'undefined' &&
               typeof Atomics !== 'undefined';
    }

    checkWebGLSupport() {
        try {
            const canvas = document.createElement('canvas');
            const gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
            return gl !== null;
        } catch (e) {
            return false;
        }
    }

    getSystemInfo() {
        return {
            wasmSupport: typeof WebAssembly !== 'undefined',
            simdSupport: this.checkSIMDSupport(),
            threadingSupport: this.checkThreadingSupport(),
            webglSupport: this.checkWebGLSupport(),
            memoryAPI: typeof performance !== 'undefined' && 
                      typeof performance.memory !== 'undefined'
        };
    }

    // ECS API wrappers
    createEntity() {
        if (!this.isInitialized || !this.registry) {
            throw new Error('ECScope not initialized');
        }
        return this.registry.createEntity();
    }

    destroyEntity(entityId) {
        if (!this.isInitialized || !this.registry) {
            throw new Error('ECScope not initialized');
        }
        return this.registry.destroyEntity(entityId);
    }

    addPositionComponent(entityId, x, y, z = 0) {
        if (!this.isInitialized || !this.registry) {
            throw new Error('ECScope not initialized');
        }
        return this.registry.addPositionComponent(entityId, x, y, z);
    }

    addVelocityComponent(entityId, x, y, z = 0) {
        if (!this.isInitialized || !this.registry) {
            throw new Error('ECScope not initialized');
        }
        return this.registry.addVelocityComponent(entityId, x, y, z);
    }

    getPositionComponent(entityId) {
        if (!this.isInitialized || !this.registry) {
            throw new Error('ECScope not initialized');
        }
        return this.registry.getPositionComponent(entityId);
    }

    getVelocityComponent(entityId) {
        if (!this.isInitialized || !this.registry) {
            throw new Error('ECScope not initialized');
        }
        return this.registry.getVelocityComponent(entityId);
    }

    runMovementSystem(deltaTime) {
        if (!this.isInitialized || !this.registry) {
            throw new Error('ECScope not initialized');
        }
        return this.registry.runMovementSystem(deltaTime);
    }

    getEntityCount() {
        if (!this.isInitialized || !this.registry) {
            return 0;
        }
        return this.registry.getEntityCount();
    }

    getArchetypeInfo() {
        if (!this.isInitialized || !this.registry) {
            return [];
        }
        return this.registry.getArchetypeInfo();
    }

    // Physics API wrappers
    createPhysicsBox(x, y, width, height, isDynamic = true) {
        if (!this.isInitialized || !this.physicsWorld) {
            throw new Error('ECScope not initialized');
        }
        
        if (isDynamic) {
            return this.physicsWorld.createDynamicBody(x, y, width, height);
        } else {
            return this.physicsWorld.createStaticBody(x, y, width, height);
        }
    }

    createPhysicsCircle(x, y, radius, isDynamic = true) {
        if (!this.isInitialized || !this.physicsWorld) {
            throw new Error('ECScope not initialized');
        }
        return this.physicsWorld.createCircleBody(x, y, radius, !isDynamic);
    }

    stepPhysics(deltaTime) {
        if (!this.isInitialized || !this.physicsWorld) {
            throw new Error('ECScope not initialized');
        }
        
        if (deltaTime !== undefined) {
            return this.physicsWorld.stepWithTime(deltaTime);
        } else {
            return this.physicsWorld.step();
        }
    }

    getPhysicsBodies() {
        if (!this.isInitialized || !this.physicsWorld) {
            return [];
        }
        return this.physicsWorld.getAllBodies();
    }

    setGravity(x, y) {
        if (!this.isInitialized || !this.physicsWorld) {
            throw new Error('ECScope not initialized');
        }
        return this.physicsWorld.setGravity(x, y);
    }

    // Performance monitoring
    getPerformanceStats() {
        if (!this.isInitialized) {
            return {
                fps: 0,
                frameTime: 0,
                memoryUsage: 0
            };
        }

        const fps = this.module._ecscope_wasm_get_fps();
        const frameTime = this.module._ecscope_wasm_get_frame_time();
        
        return {
            fps: fps,
            frameTime: frameTime,
            memoryUsage: this.getMemoryUsage()
        };
    }

    getMemoryUsage() {
        if (!this.isInitialized || !this.registry) {
            return 0;
        }
        return this.registry.getMemoryUsage();
    }

    reportMemoryStats() {
        if (!this.isInitialized) {
            return;
        }
        this.module._ecscope_wasm_report_memory_stats();
    }

    // Frame management
    beginFrame() {
        if (!this.isInitialized) {
            return;
        }
        this.module._ecscope_wasm_begin_frame();
    }

    endFrame() {
        if (!this.isInitialized) {
            return;
        }
        this.module._ecscope_wasm_end_frame();
    }

    // Tutorial system
    getTutorial() {
        if (!this.isInitialized || !this.tutorial) {
            throw new Error('ECScope not initialized');
        }
        return this.tutorial;
    }

    // Cleanup
    shutdown() {
        if (this.isInitialized) {
            this.module._ecscope_wasm_shutdown();
        }
        
        this.registry = null;
        this.physicsWorld = null;
        this.performanceMonitor = null;
        this.tutorial = null;
        this.module = null;
        this.isInitialized = false;
    }
}

// Global ECScope loader instance
window.ECScopeLoader = new ECScopeWasmLoader();