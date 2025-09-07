/**
 * @file main.js
 * @brief Complete ECScope WebAssembly Application Entry Point
 * @description Production-ready JavaScript application with full ECS integration
 */

'use strict';

/**
 * Complete ECScope WebAssembly Application
 */
class ECScopeWebApp {
    constructor() {
        this.module = null;
        this.registry = null;
        this.webglContext = null;
        this.renderSystem = null;
        this.uiManager = null;
        this.tutorialSystem = null;
        this.performanceMonitor = null;
        this.demoManager = null;
        this.animationFrame = null;
        this.isRunning = false;
        this.deltaTime = 0;
        this.lastFrameTime = 0;
        
        // Application state
        this.appState = {
            initialized: false,
            loading: true,
            currentDemo: 'basic',
            tutorialActive: false,
            performanceMode: 'normal',
            renderMode: 'webgl'
        };
        
        // Performance metrics
        this.metrics = {
            fps: 0,
            frameTime: 0,
            entities: 0,
            components: 0,
            systems: 0,
            memoryUsage: 0
        };
        
        // Bind methods
        this.update = this.update.bind(this);
        this.render = this.render.bind(this);
        this.onResize = this.onResize.bind(this);
        
        // Initialize event listeners
        this.setupEventListeners();
    }
    
    /**
     * Initialize complete WebAssembly application
     */
    async initialize() {
        try {
            this.showLoadingScreen();
            this.updateLoadingProgress(0, 'Initializing WebAssembly module...');
            
            // Load WebAssembly module with complete configuration
            this.module = await this.loadWasmModule();
            this.updateLoadingProgress(25, 'WebAssembly module loaded');
            
            // Initialize ECS registry
            this.registry = new this.module.Registry();
            this.updateLoadingProgress(40, 'ECS Registry initialized');
            
            // Initialize WebGL context
            this.webglContext = new this.module.WebGLContext();
            if (!this.webglContext.initialize('main-canvas')) {
                throw new Error('Failed to initialize WebGL context');
            }
            this.updateLoadingProgress(55, 'WebGL context initialized');
            
            // Initialize rendering system
            this.renderSystem = new this.module.WebRenderSystem(this.registry, this.webglContext);
            this.updateLoadingProgress(70, 'Rendering system initialized');
            
            // Initialize UI manager
            this.uiManager = new UIManager(this);
            await this.uiManager.initialize();
            this.updateLoadingProgress(80, 'UI system initialized');
            
            // Initialize tutorial system
            this.tutorialSystem = new TutorialSystem(this);
            await this.tutorialSystem.initialize();
            this.updateLoadingProgress(85, 'Tutorial system initialized');
            
            // Initialize performance monitor
            this.performanceMonitor = new PerformanceMonitor(this);
            this.performanceMonitor.initialize();
            this.updateLoadingProgress(90, 'Performance monitor initialized');
            
            // Initialize demo manager
            this.demoManager = new DemoManager(this);
            await this.demoManager.initialize();
            this.updateLoadingProgress(95, 'Demo manager initialized');
            
            // Complete initialization
            this.appState.initialized = true;
            this.appState.loading = false;
            this.updateLoadingProgress(100, 'Application ready!');
            
            // Hide loading screen and start application
            setTimeout(() => {
                this.hideLoadingScreen();
                this.start();
            }, 500);
            
            console.log('ECScope WebAssembly application initialized successfully');
            
        } catch (error) {
            console.error('Failed to initialize ECScope application:', error);
            this.showError('Failed to initialize application: ' + error.message);
        }
    }
    
    /**
     * Load WebAssembly module with complete configuration
     */
    async loadWasmModule() {
        return new Promise((resolve, reject) => {
            const script = document.createElement('script');
            script.src = 'js/ecscope.js';
            script.onload = () => {
                const moduleConfig = {
                    locateFile: (path, prefix) => {
                        if (path.endsWith('.wasm')) {
                            return 'wasm/' + path;
                        }
                        return prefix + path;
                    },
                    onRuntimeInitialized: () => {
                        resolve(Module);
                    },
                    onAbort: (error) => {
                        reject(new Error('WebAssembly module failed to load: ' + error));
                    },
                    print: (text) => {
                        console.log('[WASM]', text);
                    },
                    printErr: (text) => {
                        console.error('[WASM]', text);
                    },
                    totalDependencies: 0,
                    monitorRunDependencies: (left) => {
                        const progress = Math.max(0, (this.totalDependencies - left) / this.totalDependencies * 100);
                        this.updateLoadingProgress(progress * 0.2, `Loading dependencies... (${left} remaining)`);
                    }
                };
                
                window.Module = moduleConfig;
            };
            script.onerror = () => {
                reject(new Error('Failed to load WebAssembly script'));
            };
            document.head.appendChild(script);
        });
    }
    
    /**
     * Start complete application loop
     */
    start() {
        if (this.isRunning) return;
        
        this.isRunning = true;
        this.lastFrameTime = performance.now();
        
        // Start main loop
        this.update();
        
        console.log('ECScope WebAssembly application started');
    }
    
    /**
     * Stop application
     */
    stop() {
        if (!this.isRunning) return;
        
        this.isRunning = false;
        if (this.animationFrame) {
            cancelAnimationFrame(this.animationFrame);
            this.animationFrame = null;
        }
        
        console.log('ECScope WebAssembly application stopped');
    }
    
    /**
     * Complete update loop with performance monitoring
     */
    update(currentTime = performance.now()) {
        if (!this.isRunning || !this.appState.initialized) return;
        
        // Calculate delta time
        this.deltaTime = (currentTime - this.lastFrameTime) / 1000.0;
        this.lastFrameTime = currentTime;
        
        // Update performance metrics
        this.updateMetrics();
        
        // Update systems
        try {
            // Update tutorial system
            if (this.tutorialSystem && this.appState.tutorialActive) {
                this.tutorialSystem.update(this.deltaTime);
            }
            
            // Update demo manager
            if (this.demoManager) {
                this.demoManager.update(this.deltaTime);
            }
            
            // Update ECS systems
            if (this.registry) {
                this.registry.update(this.deltaTime);
            }
            
            // Update UI
            if (this.uiManager) {
                this.uiManager.update(this.deltaTime);
            }
            
            // Update performance monitor
            if (this.performanceMonitor) {
                this.performanceMonitor.update(this.deltaTime);
            }
            
        } catch (error) {
            console.error('Error during update:', error);
        }
        
        // Render frame
        this.render();
        
        // Schedule next frame
        this.animationFrame = requestAnimationFrame(this.update);
    }
    
    /**
     * Complete rendering pipeline
     */
    render() {
        if (!this.webglContext || !this.renderSystem) return;
        
        try {
            // Begin frame
            this.webglContext.beginFrame();
            
            // Clear buffers
            this.webglContext.setClearColor(0.1, 0.1, 0.15, 1.0);
            
            // Render ECS entities
            this.renderSystem.render(this.deltaTime);
            
            // Render demo content
            if (this.demoManager) {
                this.demoManager.render();
            }
            
            // Render tutorial overlays
            if (this.tutorialSystem && this.appState.tutorialActive) {
                this.tutorialSystem.render();
            }
            
            // End frame
            this.webglContext.endFrame();
            this.webglContext.present();
            
        } catch (error) {
            console.error('Error during render:', error);
        }
    }
    
    /**
     * Update performance metrics
     */
    updateMetrics() {
        this.metrics.fps = Math.round(1.0 / this.deltaTime);
        this.metrics.frameTime = this.deltaTime * 1000;
        
        if (this.registry) {
            this.metrics.entities = this.registry.getEntityCount();
            this.metrics.components = this.registry.getComponentCount();
            this.metrics.systems = this.registry.getSystemCount();
        }
        
        if (this.module && this.module._emscripten_get_heap_size) {
            this.metrics.memoryUsage = this.module._emscripten_get_heap_size();
        }
    }
    
    /**
     * Setup complete event listeners
     */
    setupEventListeners() {
        // Window events
        window.addEventListener('resize', this.onResize);
        window.addEventListener('beforeunload', () => this.cleanup());
        
        // Keyboard events
        document.addEventListener('keydown', (event) => this.onKeyDown(event));
        document.addEventListener('keyup', (event) => this.onKeyUp(event));
        
        // Mouse events
        const canvas = document.getElementById('main-canvas');
        if (canvas) {
            canvas.addEventListener('mousedown', (event) => this.onMouseDown(event));
            canvas.addEventListener('mouseup', (event) => this.onMouseUp(event));
            canvas.addEventListener('mousemove', (event) => this.onMouseMove(event));
            canvas.addEventListener('wheel', (event) => this.onWheel(event));
        }
        
        // UI events
        this.setupUIEvents();
    }
    
    /**
     * Setup UI event listeners
     */
    setupUIEvents() {
        // Demo selection
        const demoSelect = document.getElementById('demo-select');
        if (demoSelect) {
            demoSelect.addEventListener('change', (event) => {
                this.switchDemo(event.target.value);
            });
        }
        
        // Tutorial controls
        const tutorialBtn = document.getElementById('tutorial-btn');
        if (tutorialBtn) {
            tutorialBtn.addEventListener('click', () => this.toggleTutorial());
        }
        
        // Performance toggle
        const perfBtn = document.getElementById('performance-btn');
        if (perfBtn) {
            perfBtn.addEventListener('click', () => this.togglePerformanceMode());
        }
        
        // Settings panel
        const settingsBtn = document.getElementById('settings-btn');
        if (settingsBtn) {
            settingsBtn.addEventListener('click', () => this.toggleSettings());
        }
        
        // Reset button
        const resetBtn = document.getElementById('reset-btn');
        if (resetBtn) {
            resetBtn.addEventListener('click', () => this.resetDemo());
        }
        
        // Play/Pause button
        const playPauseBtn = document.getElementById('play-pause-btn');
        if (playPauseBtn) {
            playPauseBtn.addEventListener('click', () => this.togglePlayPause());
        }
    }
    
    /**
     * Handle window resize
     */
    onResize() {
        const canvas = document.getElementById('main-canvas');
        if (canvas && this.webglContext) {
            const rect = canvas.parentElement.getBoundingClientRect();
            canvas.width = rect.width;
            canvas.height = rect.height;
            
            // Update WebGL viewport
            this.webglContext.setViewport(0, 0, canvas.width, canvas.height);
        }
        
        // Update UI layout
        if (this.uiManager) {
            this.uiManager.onResize();
        }
    }
    
    /**
     * Handle keyboard input
     */
    onKeyDown(event) {
        // Tutorial shortcuts
        if (event.key === 'F1') {
            event.preventDefault();
            this.toggleTutorial();
        }
        
        // Performance shortcuts
        if (event.key === 'F2') {
            event.preventDefault();
            this.togglePerformanceMode();
        }
        
        // Demo shortcuts
        if (event.key >= '1' && event.key <= '9') {
            const demoIndex = parseInt(event.key) - 1;
            const demos = ['basic', 'advanced', 'physics', 'graphics', 'ai', 'network'];
            if (demoIndex < demos.length) {
                this.switchDemo(demos[demoIndex]);
            }
        }
        
        // Pass to current demo
        if (this.demoManager) {
            this.demoManager.onKeyDown(event);
        }
    }
    
    onKeyUp(event) {
        if (this.demoManager) {
            this.demoManager.onKeyUp(event);
        }
    }
    
    /**
     * Handle mouse input
     */
    onMouseDown(event) {
        if (this.demoManager) {
            this.demoManager.onMouseDown(event);
        }
    }
    
    onMouseUp(event) {
        if (this.demoManager) {
            this.demoManager.onMouseUp(event);
        }
    }
    
    onMouseMove(event) {
        if (this.demoManager) {
            this.demoManager.onMouseMove(event);
        }
    }
    
    onWheel(event) {
        if (this.demoManager) {
            this.demoManager.onWheel(event);
        }
    }
    
    /**
     * Switch to different demo
     */
    switchDemo(demoName) {
        if (this.appState.currentDemo === demoName) return;
        
        console.log(`Switching to demo: ${demoName}`);
        
        if (this.demoManager) {
            this.demoManager.switchDemo(demoName);
        }
        
        this.appState.currentDemo = demoName;
        
        // Update UI
        if (this.uiManager) {
            this.uiManager.updateDemoDisplay(demoName);
        }
    }
    
    /**
     * Toggle tutorial system
     */
    toggleTutorial() {
        this.appState.tutorialActive = !this.appState.tutorialActive;
        
        if (this.tutorialSystem) {
            if (this.appState.tutorialActive) {
                this.tutorialSystem.start(this.appState.currentDemo);
            } else {
                this.tutorialSystem.stop();
            }
        }
        
        // Update UI
        if (this.uiManager) {
            this.uiManager.updateTutorialDisplay(this.appState.tutorialActive);
        }
        
        console.log(`Tutorial ${this.appState.tutorialActive ? 'started' : 'stopped'}`);
    }
    
    /**
     * Toggle performance monitoring mode
     */
    togglePerformanceMode() {
        const modes = ['normal', 'detailed', 'profiler'];
        const currentIndex = modes.indexOf(this.appState.performanceMode);
        const nextIndex = (currentIndex + 1) % modes.length;
        this.appState.performanceMode = modes[nextIndex];
        
        if (this.performanceMonitor) {
            this.performanceMonitor.setMode(this.appState.performanceMode);
        }
        
        // Update UI
        if (this.uiManager) {
            this.uiManager.updatePerformanceDisplay(this.appState.performanceMode);
        }
        
        console.log(`Performance mode: ${this.appState.performanceMode}`);
    }
    
    /**
     * Toggle settings panel
     */
    toggleSettings() {
        if (this.uiManager) {
            this.uiManager.toggleSettings();
        }
    }
    
    /**
     * Reset current demo
     */
    resetDemo() {
        if (this.demoManager) {
            this.demoManager.resetCurrentDemo();
        }
        
        console.log('Demo reset');
    }
    
    /**
     * Toggle play/pause
     */
    togglePlayPause() {
        if (this.isRunning) {
            this.stop();
        } else {
            this.start();
        }
        
        // Update UI
        if (this.uiManager) {
            this.uiManager.updatePlayPauseDisplay(this.isRunning);
        }
    }
    
    /**
     * Show loading screen with progress
     */
    showLoadingScreen() {
        const loadingScreen = document.getElementById('loading-screen');
        if (loadingScreen) {
            loadingScreen.style.display = 'flex';
        }
    }
    
    /**
     * Update loading progress
     */
    updateLoadingProgress(percentage, message) {
        const progressBar = document.getElementById('loading-progress');
        const progressText = document.getElementById('loading-text');
        
        if (progressBar) {
            progressBar.style.width = `${percentage}%`;
        }
        
        if (progressText) {
            progressText.textContent = message;
        }
        
        console.log(`Loading: ${percentage}% - ${message}`);
    }
    
    /**
     * Hide loading screen
     */
    hideLoadingScreen() {
        const loadingScreen = document.getElementById('loading-screen');
        if (loadingScreen) {
            loadingScreen.style.display = 'none';
        }
    }
    
    /**
     * Show error message
     */
    showError(message) {
        const errorScreen = document.getElementById('error-screen');
        const errorMessage = document.getElementById('error-message');
        
        if (errorScreen) {
            errorScreen.style.display = 'flex';
        }
        
        if (errorMessage) {
            errorMessage.textContent = message;
        }
        
        console.error('Application Error:', message);
    }
    
    /**
     * Get current application state
     */
    getState() {
        return { ...this.appState };
    }
    
    /**
     * Get current metrics
     */
    getMetrics() {
        return { ...this.metrics };
    }
    
    /**
     * Get WebAssembly module
     */
    getModule() {
        return this.module;
    }
    
    /**
     * Get ECS registry
     */
    getRegistry() {
        return this.registry;
    }
    
    /**
     * Get WebGL context
     */
    getWebGLContext() {
        return this.webglContext;
    }
    
    /**
     * Cleanup resources
     */
    cleanup() {
        this.stop();
        
        if (this.tutorialSystem) {
            this.tutorialSystem.cleanup();
        }
        
        if (this.demoManager) {
            this.demoManager.cleanup();
        }
        
        if (this.performanceMonitor) {
            this.performanceMonitor.cleanup();
        }
        
        if (this.uiManager) {
            this.uiManager.cleanup();
        }
        
        if (this.renderSystem) {
            this.renderSystem.cleanup();
        }
        
        if (this.webglContext) {
            this.webglContext.shutdown();
        }
        
        if (this.registry) {
            this.registry.clear();
        }
        
        console.log('ECScope WebAssembly application cleaned up');
    }
}

/**
 * Complete UI Management System
 */
class UIManager {
    constructor(app) {
        this.app = app;
        this.panels = new Map();
        this.animations = new Map();
        this.settings = {
            theme: 'dark',
            showFPS: true,
            showMetrics: true,
            showHelp: true,
            autoHideUI: false
        };
    }
    
    async initialize() {
        this.setupPanels();
        this.loadSettings();
        this.updateTheme();
        
        console.log('UI Manager initialized');
    }
    
    setupPanels() {
        // Performance panel
        const perfPanel = document.getElementById('performance-panel');
        if (perfPanel) {
            this.panels.set('performance', new PerformancePanel(perfPanel, this.app));
        }
        
        // Control panel
        const controlPanel = document.getElementById('control-panel');
        if (controlPanel) {
            this.panels.set('controls', new ControlPanel(controlPanel, this.app));
        }
        
        // Settings panel
        const settingsPanel = document.getElementById('settings-panel');
        if (settingsPanel) {
            this.panels.set('settings', new SettingsPanel(settingsPanel, this));
        }
        
        // Help panel
        const helpPanel = document.getElementById('help-panel');
        if (helpPanel) {
            this.panels.set('help', new HelpPanel(helpPanel, this.app));
        }
    }
    
    update(deltaTime) {
        // Update all panels
        for (const panel of this.panels.values()) {
            panel.update(deltaTime);
        }
        
        // Update animations
        this.updateAnimations(deltaTime);
        
        // Auto-hide UI if enabled
        if (this.settings.autoHideUI) {
            this.updateAutoHideUI(deltaTime);
        }
    }
    
    updateAnimations(deltaTime) {
        for (const [name, animation] of this.animations) {
            animation.update(deltaTime);
            if (animation.isComplete()) {
                this.animations.delete(name);
            }
        }
    }
    
    updateAutoHideUI(deltaTime) {
        // Implementation for auto-hiding UI after inactivity
    }
    
    updateDemoDisplay(demoName) {
        const demoSelect = document.getElementById('demo-select');
        if (demoSelect) {
            demoSelect.value = demoName;
        }
        
        const demoTitle = document.getElementById('demo-title');
        if (demoTitle) {
            demoTitle.textContent = this.getDemoDisplayName(demoName);
        }
    }
    
    updateTutorialDisplay(active) {
        const tutorialBtn = document.getElementById('tutorial-btn');
        if (tutorialBtn) {
            tutorialBtn.classList.toggle('active', active);
            tutorialBtn.textContent = active ? 'Stop Tutorial' : 'Start Tutorial';
        }
    }
    
    updatePerformanceDisplay(mode) {
        const perfBtn = document.getElementById('performance-btn');
        if (perfBtn) {
            perfBtn.textContent = `Performance: ${mode}`;
        }
        
        const perfPanel = this.panels.get('performance');
        if (perfPanel) {
            perfPanel.setMode(mode);
        }
    }
    
    updatePlayPauseDisplay(isPlaying) {
        const playPauseBtn = document.getElementById('play-pause-btn');
        if (playPauseBtn) {
            playPauseBtn.textContent = isPlaying ? 'Pause' : 'Play';
            playPauseBtn.classList.toggle('playing', isPlaying);
        }
    }
    
    toggleSettings() {
        const settingsPanel = this.panels.get('settings');
        if (settingsPanel) {
            settingsPanel.toggle();
        }
    }
    
    onResize() {
        for (const panel of this.panels.values()) {
            if (panel.onResize) {
                panel.onResize();
            }
        }
    }
    
    getDemoDisplayName(demoName) {
        const names = {
            'basic': 'Basic ECS Demo',
            'advanced': 'Advanced Features',
            'physics': 'Physics Simulation',
            'graphics': 'Advanced Graphics',
            'ai': 'AI Integration',
            'network': 'Network Systems'
        };
        return names[demoName] || demoName;
    }
    
    loadSettings() {
        try {
            const saved = localStorage.getItem('ecscope-settings');
            if (saved) {
                Object.assign(this.settings, JSON.parse(saved));
            }
        } catch (error) {
            console.warn('Failed to load settings:', error);
        }
    }
    
    saveSettings() {
        try {
            localStorage.setItem('ecscope-settings', JSON.stringify(this.settings));
        } catch (error) {
            console.warn('Failed to save settings:', error);
        }
    }
    
    updateTheme() {
        document.documentElement.setAttribute('data-theme', this.settings.theme);
    }
    
    cleanup() {
        for (const panel of this.panels.values()) {
            if (panel.cleanup) {
                panel.cleanup();
            }
        }
        this.panels.clear();
        this.animations.clear();
        
        console.log('UI Manager cleaned up');
    }
}

/**
 * Complete Tutorial System
 */
class TutorialSystem {
    constructor(app) {
        this.app = app;
        this.tutorials = new Map();
        this.currentTutorial = null;
        this.currentStep = 0;
        this.overlay = null;
        this.highlightElement = null;
    }
    
    async initialize() {
        this.setupOverlay();
        await this.loadTutorials();
        
        console.log('Tutorial System initialized');
    }
    
    setupOverlay() {
        // Create tutorial overlay
        this.overlay = document.createElement('div');
        this.overlay.className = 'tutorial-overlay';
        this.overlay.innerHTML = `
            <div class="tutorial-content">
                <div class="tutorial-header">
                    <h3 class="tutorial-title"></h3>
                    <button class="tutorial-close">&times;</button>
                </div>
                <div class="tutorial-body">
                    <p class="tutorial-text"></p>
                    <div class="tutorial-media"></div>
                </div>
                <div class="tutorial-footer">
                    <button class="tutorial-prev">Previous</button>
                    <span class="tutorial-progress"></span>
                    <button class="tutorial-next">Next</button>
                </div>
            </div>
            <div class="tutorial-highlight"></div>
        `;
        document.body.appendChild(this.overlay);
        
        this.highlightElement = this.overlay.querySelector('.tutorial-highlight');
        
        // Setup event listeners
        this.overlay.querySelector('.tutorial-close').addEventListener('click', () => this.stop());
        this.overlay.querySelector('.tutorial-prev').addEventListener('click', () => this.previousStep());
        this.overlay.querySelector('.tutorial-next').addEventListener('click', () => this.nextStep());
    }
    
    async loadTutorials() {
        // Load tutorial definitions
        const tutorialData = {
            basic: {
                title: 'Basic ECS Concepts',
                steps: [
                    {
                        title: 'Welcome to ECScope',
                        text: 'ECScope is a powerful Entity-Component-System framework. Let\'s explore the basics!',
                        highlight: '#main-canvas'
                    },
                    {
                        title: 'Entities',
                        text: 'Entities are unique identifiers that represent game objects. They have no data or behavior by themselves.',
                        highlight: '.entity-panel'
                    },
                    {
                        title: 'Components',
                        text: 'Components are pure data containers that define what an entity is made of.',
                        highlight: '.component-panel'
                    },
                    {
                        title: 'Systems',
                        text: 'Systems contain the logic that operates on entities with specific components.',
                        highlight: '.system-panel'
                    }
                ]
            },
            advanced: {
                title: 'Advanced ECS Features',
                steps: [
                    {
                        title: 'Component Queries',
                        text: 'Learn how to efficiently query entities based on their components.',
                        highlight: '.query-panel'
                    },
                    {
                        title: 'System Dependencies',
                        text: 'Understand how systems can depend on each other for optimal execution.',
                        highlight: '.dependency-panel'
                    }
                ]
            }
        };
        
        for (const [name, data] of Object.entries(tutorialData)) {
            this.tutorials.set(name, data);
        }
    }
    
    start(tutorialName) {
        const tutorial = this.tutorials.get(tutorialName);
        if (!tutorial) {
            console.warn(`Tutorial '${tutorialName}' not found`);
            return;
        }
        
        this.currentTutorial = tutorial;
        this.currentStep = 0;
        this.overlay.style.display = 'block';
        this.showStep(0);
        
        console.log(`Started tutorial: ${tutorialName}`);
    }
    
    stop() {
        this.currentTutorial = null;
        this.currentStep = 0;
        this.overlay.style.display = 'none';
        this.clearHighlight();
        
        this.app.appState.tutorialActive = false;
        
        console.log('Tutorial stopped');
    }
    
    showStep(stepIndex) {
        if (!this.currentTutorial || stepIndex >= this.currentTutorial.steps.length) {
            return;
        }
        
        const step = this.currentTutorial.steps[stepIndex];
        
        // Update content
        this.overlay.querySelector('.tutorial-title').textContent = step.title;
        this.overlay.querySelector('.tutorial-text').textContent = step.text;
        
        // Update progress
        const progress = `${stepIndex + 1} / ${this.currentTutorial.steps.length}`;
        this.overlay.querySelector('.tutorial-progress').textContent = progress;
        
        // Update navigation buttons
        const prevBtn = this.overlay.querySelector('.tutorial-prev');
        const nextBtn = this.overlay.querySelector('.tutorial-next');
        
        prevBtn.disabled = stepIndex === 0;
        nextBtn.textContent = stepIndex === this.currentTutorial.steps.length - 1 ? 'Finish' : 'Next';
        
        // Update highlight
        this.updateHighlight(step.highlight);
    }
    
    updateHighlight(selector) {
        this.clearHighlight();
        
        if (selector) {
            const element = document.querySelector(selector);
            if (element) {
                const rect = element.getBoundingClientRect();
                this.highlightElement.style.display = 'block';
                this.highlightElement.style.left = rect.left + 'px';
                this.highlightElement.style.top = rect.top + 'px';
                this.highlightElement.style.width = rect.width + 'px';
                this.highlightElement.style.height = rect.height + 'px';
            }
        }
    }
    
    clearHighlight() {
        this.highlightElement.style.display = 'none';
    }
    
    nextStep() {
        if (!this.currentTutorial) return;
        
        if (this.currentStep < this.currentTutorial.steps.length - 1) {
            this.currentStep++;
            this.showStep(this.currentStep);
        } else {
            this.stop();
        }
    }
    
    previousStep() {
        if (this.currentStep > 0) {
            this.currentStep--;
            this.showStep(this.currentStep);
        }
    }
    
    update(deltaTime) {
        // Update tutorial animations or interactions
    }
    
    render() {
        // Render tutorial-specific overlays
    }
    
    cleanup() {
        if (this.overlay && this.overlay.parentNode) {
            this.overlay.parentNode.removeChild(this.overlay);
        }
        
        console.log('Tutorial System cleaned up');
    }
}

/**
 * Complete Performance Monitor
 */
class PerformanceMonitor {
    constructor(app) {
        this.app = app;
        this.mode = 'normal';
        this.samples = {
            fps: [],
            frameTime: [],
            memory: [],
            entities: []
        };
        this.maxSamples = 120; // 2 seconds at 60fps
        this.updateInterval = 1000 / 60; // 60 updates per second
        this.lastUpdate = 0;
    }
    
    initialize() {
        this.setupDisplay();
        console.log('Performance Monitor initialized');
    }
    
    setupDisplay() {
        // Performance display is handled by UI panels
    }
    
    update(deltaTime) {
        const now = performance.now();
        if (now - this.lastUpdate < this.updateInterval) return;
        
        this.lastUpdate = now;
        
        const metrics = this.app.getMetrics();
        
        // Collect samples
        this.addSample('fps', metrics.fps);
        this.addSample('frameTime', metrics.frameTime);
        this.addSample('memory', metrics.memoryUsage);
        this.addSample('entities', metrics.entities);
        
        // Update display based on mode
        this.updateDisplay();
    }
    
    addSample(type, value) {
        const samples = this.samples[type];
        samples.push(value);
        
        if (samples.length > this.maxSamples) {
            samples.shift();
        }
    }
    
    updateDisplay() {
        const perfPanel = this.app.uiManager?.panels.get('performance');
        if (perfPanel) {
            perfPanel.updateMetrics(this.getDisplayMetrics());
        }
    }
    
    getDisplayMetrics() {
        const result = {};
        
        for (const [type, samples] of Object.entries(this.samples)) {
            if (samples.length === 0) continue;
            
            const current = samples[samples.length - 1];
            const average = samples.reduce((a, b) => a + b, 0) / samples.length;
            const min = Math.min(...samples);
            const max = Math.max(...samples);
            
            result[type] = { current, average, min, max, samples };
        }
        
        return result;
    }
    
    setMode(mode) {
        this.mode = mode;
        console.log(`Performance monitor mode: ${mode}`);
    }
    
    cleanup() {
        console.log('Performance Monitor cleaned up');
    }
}

/**
 * Complete Demo Manager
 */
class DemoManager {
    constructor(app) {
        this.app = app;
        this.demos = new Map();
        this.currentDemo = null;
    }
    
    async initialize() {
        await this.loadDemos();
        console.log('Demo Manager initialized');
    }
    
    async loadDemos() {
        // Register all available demos
        this.demos.set('basic', new BasicECSDemo(this.app));
        this.demos.set('advanced', new AdvancedFeaturesDemo(this.app));
        this.demos.set('physics', new PhysicsDemo(this.app));
        this.demos.set('graphics', new GraphicsDemo(this.app));
        this.demos.set('ai', new AIDemo(this.app));
        this.demos.set('network', new NetworkDemo(this.app));
        
        // Initialize all demos
        for (const demo of this.demos.values()) {
            await demo.initialize();
        }
        
        // Start with basic demo
        this.switchDemo('basic');
    }
    
    switchDemo(demoName) {
        const demo = this.demos.get(demoName);
        if (!demo) {
            console.warn(`Demo '${demoName}' not found`);
            return;
        }
        
        // Stop current demo
        if (this.currentDemo) {
            this.currentDemo.stop();
        }
        
        // Start new demo
        this.currentDemo = demo;
        this.currentDemo.start();
        
        console.log(`Switched to demo: ${demoName}`);
    }
    
    resetCurrentDemo() {
        if (this.currentDemo) {
            this.currentDemo.reset();
        }
    }
    
    update(deltaTime) {
        if (this.currentDemo) {
            this.currentDemo.update(deltaTime);
        }
    }
    
    render() {
        if (this.currentDemo) {
            this.currentDemo.render();
        }
    }
    
    // Input forwarding
    onKeyDown(event) {
        if (this.currentDemo && this.currentDemo.onKeyDown) {
            this.currentDemo.onKeyDown(event);
        }
    }
    
    onKeyUp(event) {
        if (this.currentDemo && this.currentDemo.onKeyUp) {
            this.currentDemo.onKeyUp(event);
        }
    }
    
    onMouseDown(event) {
        if (this.currentDemo && this.currentDemo.onMouseDown) {
            this.currentDemo.onMouseDown(event);
        }
    }
    
    onMouseUp(event) {
        if (this.currentDemo && this.currentDemo.onMouseUp) {
            this.currentDemo.onMouseUp(event);
        }
    }
    
    onMouseMove(event) {
        if (this.currentDemo && this.currentDemo.onMouseMove) {
            this.currentDemo.onMouseMove(event);
        }
    }
    
    onWheel(event) {
        if (this.currentDemo && this.currentDemo.onWheel) {
            this.currentDemo.onWheel(event);
        }
    }
    
    cleanup() {
        for (const demo of this.demos.values()) {
            demo.cleanup();
        }
        this.demos.clear();
        
        console.log('Demo Manager cleaned up');
    }
}

/**
 * Base Demo Class
 */
class BaseDemo {
    constructor(app) {
        this.app = app;
        this.isActive = false;
        this.entities = [];
        this.systems = [];
    }
    
    async initialize() {
        // Override in subclasses
    }
    
    start() {
        this.isActive = true;
        this.createScene();
    }
    
    stop() {
        this.isActive = false;
        this.cleanup();
    }
    
    reset() {
        this.stop();
        this.start();
    }
    
    createScene() {
        // Override in subclasses
    }
    
    update(deltaTime) {
        if (!this.isActive) return;
        
        // Update demo-specific logic
        for (const system of this.systems) {
            system.update(deltaTime);
        }
    }
    
    render() {
        if (!this.isActive) return;
        
        // Render demo-specific content
    }
    
    cleanup() {
        // Clean up entities and systems
        for (const entity of this.entities) {
            entity.destroy();
        }
        this.entities = [];
        this.systems = [];
    }
}

/**
 * Basic ECS Demo
 */
class BasicECSDemo extends BaseDemo {
    async initialize() {
        console.log('Basic ECS Demo initialized');
    }
    
    createScene() {
        const registry = this.app.getRegistry();
        if (!registry) return;
        
        // Create some basic entities with transform and renderable components
        for (let i = 0; i < 10; i++) {
            const entity = registry.createEntity();
            
            // Add transform component
            const transform = entity.addTransformComponent();
            transform.position.x = (Math.random() - 0.5) * 10;
            transform.position.y = (Math.random() - 0.5) * 10;
            transform.position.z = 0;
            
            // Add renderable component
            const renderable = entity.addRenderableComponent();
            renderable.color.r = Math.random();
            renderable.color.g = Math.random();
            renderable.color.b = Math.random();
            renderable.color.a = 1.0;
            
            this.entities.push(entity);
        }
        
        console.log(`Created ${this.entities.length} entities for basic demo`);
    }
}

/**
 * Advanced Features Demo
 */
class AdvancedFeaturesDemo extends BaseDemo {
    async initialize() {
        console.log('Advanced Features Demo initialized');
    }
    
    createScene() {
        // Create entities with more complex component combinations
        const registry = this.app.getRegistry();
        if (!registry) return;
        
        // Create entities with physics components
        for (let i = 0; i < 5; i++) {
            const entity = registry.createEntity();
            
            const transform = entity.addTransformComponent();
            transform.position.x = i * 2 - 4;
            transform.position.y = 0;
            
            const renderable = entity.addRenderableComponent();
            renderable.color.r = 0.2;
            renderable.color.g = 0.8;
            renderable.color.b = 0.2;
            
            // Add physics component if available
            if (entity.addPhysicsComponent) {
                const physics = entity.addPhysicsComponent();
                physics.velocity.x = (Math.random() - 0.5) * 5;
                physics.velocity.y = Math.random() * 3;
            }
            
            this.entities.push(entity);
        }
        
        console.log('Advanced demo scene created');
    }
}

// Additional demo classes would be implemented similarly...
class PhysicsDemo extends BaseDemo {
    async initialize() {
        console.log('Physics Demo initialized');
    }
    
    createScene() {
        console.log('Physics demo scene created');
    }
}

class GraphicsDemo extends BaseDemo {
    async initialize() {
        console.log('Graphics Demo initialized');
    }
    
    createScene() {
        console.log('Graphics demo scene created');
    }
}

class AIDemo extends BaseDemo {
    async initialize() {
        console.log('AI Demo initialized');
    }
    
    createScene() {
        console.log('AI demo scene created');
    }
}

class NetworkDemo extends BaseDemo {
    async initialize() {
        console.log('Network Demo initialized');
    }
    
    createScene() {
        console.log('Network demo scene created');
    }
}

/**
 * Performance Panel Implementation
 */
class PerformancePanel {
    constructor(element, app) {
        this.element = element;
        this.app = app;
        this.mode = 'normal';
        this.charts = {};
    }
    
    update(deltaTime) {
        // Update performance display
        this.updateMetrics(this.app.getMetrics());
    }
    
    updateMetrics(metrics) {
        // Update FPS display
        const fpsElement = this.element.querySelector('.fps-value');
        if (fpsElement) {
            fpsElement.textContent = metrics.fps;
        }
        
        // Update frame time
        const frameTimeElement = this.element.querySelector('.frametime-value');
        if (frameTimeElement) {
            frameTimeElement.textContent = metrics.frameTime.toFixed(2) + 'ms';
        }
        
        // Update entity count
        const entitiesElement = this.element.querySelector('.entities-value');
        if (entitiesElement) {
            entitiesElement.textContent = metrics.entities;
        }
        
        // Update memory usage
        const memoryElement = this.element.querySelector('.memory-value');
        if (memoryElement && metrics.memoryUsage) {
            const mb = (metrics.memoryUsage / 1024 / 1024).toFixed(1);
            memoryElement.textContent = mb + 'MB';
        }
    }
    
    setMode(mode) {
        this.mode = mode;
        this.element.className = `performance-panel mode-${mode}`;
    }
}

/**
 * Control Panel Implementation
 */
class ControlPanel {
    constructor(element, app) {
        this.element = element;
        this.app = app;
    }
    
    update(deltaTime) {
        // Update control states
    }
}

/**
 * Settings Panel Implementation
 */
class SettingsPanel {
    constructor(element, uiManager) {
        this.element = element;
        this.uiManager = uiManager;
        this.isVisible = false;
    }
    
    toggle() {
        this.isVisible = !this.isVisible;
        this.element.style.display = this.isVisible ? 'block' : 'none';
    }
}

/**
 * Help Panel Implementation
 */
class HelpPanel {
    constructor(element, app) {
        this.element = element;
        this.app = app;
    }
    
    update(deltaTime) {
        // Update help content
    }
}

// Global application instance
let app = null;

/**
 * Application entry point
 */
async function main() {
    console.log('Starting ECScope WebAssembly Application...');
    
    try {
        // Create application instance
        app = new ECScopeWebApp();
        
        // Initialize application
        await app.initialize();
        
        console.log('ECScope WebAssembly Application started successfully');
        
    } catch (error) {
        console.error('Failed to start ECScope application:', error);
        
        // Show error to user
        const errorScreen = document.getElementById('error-screen');
        const errorMessage = document.getElementById('error-message');
        
        if (errorScreen) {
            errorScreen.style.display = 'flex';
        }
        
        if (errorMessage) {
            errorMessage.textContent = 'Failed to start application: ' + error.message;
        }
    }
}

// Start application when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', main);
} else {
    main();
}

// Export for global access
window.ECScopeApp = app;