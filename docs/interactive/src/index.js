/**
 * @file index.js
 * @brief Main entry point for ECScope Interactive Documentation System
 * 
 * This is the main entry point that initializes all documentation system components
 * including the code execution engine, visualization system, interactive tutorials,
 * and performance integration.
 */

import './styles/main.css';
import './styles/themes.css';
import './styles/components.css';

// Core system imports
import { DocumentationEngine } from './core/documentation-engine';
import { CodeExecutionEngine } from './execution/code-execution-engine';
import { InteractiveTutorialSystem } from './tutorials/tutorial-system';
import { VisualizationEngine } from './visualization/visualization-engine';
import { SearchSystem } from './search/search-system';
import { NavigationSystem } from './navigation/navigation-system';
import { PerformanceIntegration } from './integration/performance-integration';

// UI Components
import { Dashboard } from './components/dashboard/Dashboard';
import { CodePlayground } from './components/code/CodePlayground';
import { TutorialViewer } from './components/tutorials/TutorialViewer';
import { APIExplorer } from './components/api/APIExplorer';
import { PerformanceMonitor } from './components/performance/PerformanceMonitor';

// Utilities
import { Logger } from './utils/logger';
import { ConfigManager } from './utils/config-manager';
import { StorageManager } from './utils/storage-manager';

/**
 * Main Documentation Application
 */
class ECScopeDocumentationApp {
    constructor() {
        this.logger = new Logger('DocumentationApp');
        this.config = new ConfigManager();
        this.storage = new StorageManager();
        
        // Core systems
        this.documentationEngine = null;
        this.codeExecutionEngine = null;
        this.tutorialSystem = null;
        this.visualizationEngine = null;
        this.searchSystem = null;
        this.navigationSystem = null;
        this.performanceIntegration = null;
        
        // UI components
        this.dashboard = null;
        this.components = new Map();
        
        // State
        this.isInitialized = false;
        this.currentUser = null;
        this.currentSession = null;
    }
    
    /**
     * Initialize the entire documentation system
     */
    async initialize() {
        try {
            this.logger.info('Initializing ECScope Interactive Documentation System...');
            
            // Load configuration
            await this.config.load();
            this.logger.info('Configuration loaded:', this.config.getAll());
            
            // Initialize core systems
            await this.initializeCoreSystemsParallel();
            
            // Initialize UI components
            await this.initializeUIComponents();
            
            // Setup event listeners and integrations
            this.setupEventListeners();
            this.setupSystemIntegrations();
            
            // Initialize user session
            await this.initializeUserSession();
            
            // Start the application
            this.start();
            
            this.isInitialized = true;
            this.logger.info('Documentation system initialized successfully');
            
            // Show welcome message or tutorial
            this.showWelcome();
            
        } catch (error) {
            this.logger.error('Failed to initialize documentation system:', error);
            this.showErrorMessage('Failed to initialize the documentation system. Please refresh the page.');
        }
    }
    
    /**
     * Initialize core systems in parallel for better performance
     */
    async initializeCoreSystemsParallel() {
        const initTasks = [
            this.initializeDocumentationEngine(),
            this.initializeCodeExecutionEngine(),
            this.initializeTutorialSystem(),
            this.initializeVisualizationEngine(),
            this.initializeSearchSystem(),
            this.initializeNavigationSystem(),
            this.initializePerformanceIntegration()
        ];
        
        await Promise.all(initTasks);
    }
    
    /**
     * Initialize documentation engine
     */
    async initializeDocumentationEngine() {
        this.logger.info('Initializing Documentation Engine...');
        this.documentationEngine = new DocumentationEngine({
            apiEndpoint: this.config.get('api.endpoint'),
            contentPath: this.config.get('content.path'),
            enableLiveReload: this.config.get('development.liveReload')
        });
        
        await this.documentationEngine.initialize();
        this.logger.info('Documentation Engine initialized');
    }
    
    /**
     * Initialize code execution engine
     */
    async initializeCodeExecutionEngine() {
        this.logger.info('Initializing Code Execution Engine...');
        this.codeExecutionEngine = new CodeExecutionEngine({
            wasmPath: this.config.get('execution.wasmPath'),
            timeoutMs: this.config.get('execution.timeoutMs'),
            memoryLimit: this.config.get('execution.memoryLimit'),
            enableSafetyChecks: this.config.get('execution.enableSafety')
        });
        
        await this.codeExecutionEngine.initialize();
        this.logger.info('Code Execution Engine initialized');
    }
    
    /**
     * Initialize tutorial system
     */
    async initializeTutorialSystem() {
        this.logger.info('Initializing Tutorial System...');
        this.tutorialSystem = new InteractiveTutorialSystem({
            tutorialsPath: this.config.get('tutorials.path'),
            enableAdaptiveLearning: this.config.get('learning.adaptiveDifficulty'),
            enableProgressTracking: this.config.get('learning.progressTracking')
        });
        
        await this.tutorialSystem.initialize();
        this.logger.info('Tutorial System initialized');
    }
    
    /**
     * Initialize visualization engine
     */
    async initializeVisualizationEngine() {
        this.logger.info('Initializing Visualization Engine...');
        this.visualizationEngine = new VisualizationEngine({
            canvas: document.getElementById('visualizationCanvas'),
            enableWebGL: this.config.get('visualization.enableWebGL'),
            enableVR: this.config.get('visualization.enableVR')
        });
        
        await this.visualizationEngine.initialize();
        this.logger.info('Visualization Engine initialized');
    }
    
    /**
     * Initialize search system
     */
    async initializeSearchSystem() {
        this.logger.info('Initializing Search System...');
        this.searchSystem = new SearchSystem({
            enableSemanticSearch: this.config.get('search.enableSemantic'),
            indexPath: this.config.get('search.indexPath'),
            enableFuzzySearch: this.config.get('search.enableFuzzy')
        });
        
        await this.searchSystem.initialize();
        this.logger.info('Search System initialized');
    }
    
    /**
     * Initialize navigation system
     */
    async initializeNavigationSystem() {
        this.logger.info('Initializing Navigation System...');
        this.navigationSystem = new NavigationSystem({
            enableSmartNavigation: this.config.get('navigation.enableSmart'),
            enableBreadcrumbs: this.config.get('navigation.enableBreadcrumbs'),
            historyLength: this.config.get('navigation.historyLength')
        });
        
        await this.navigationSystem.initialize();
        this.logger.info('Navigation System initialized');
    }
    
    /**
     * Initialize performance integration
     */
    async initializePerformanceIntegration() {
        this.logger.info('Initializing Performance Integration...');
        this.performanceIntegration = new PerformanceIntegration({
            enableLiveMetrics: this.config.get('performance.liveMetrics'),
            metricsEndpoint: this.config.get('performance.endpoint'),
            updateInterval: this.config.get('performance.updateInterval')
        });
        
        await this.performanceIntegration.initialize();
        this.logger.info('Performance Integration initialized');
    }
    
    /**
     * Initialize UI components
     */
    async initializeUIComponents() {
        this.logger.info('Initializing UI Components...');
        
        // Main dashboard
        this.dashboard = new Dashboard({
            container: document.getElementById('app'),
            systems: {
                documentation: this.documentationEngine,
                codeExecution: this.codeExecutionEngine,
                tutorials: this.tutorialSystem,
                visualization: this.visualizationEngine,
                search: this.searchSystem,
                navigation: this.navigationSystem,
                performance: this.performanceIntegration
            }
        });
        
        // Individual components
        this.components.set('codePlayground', new CodePlayground({
            container: document.getElementById('codePlayground'),
            executionEngine: this.codeExecutionEngine,
            visualizationEngine: this.visualizationEngine
        }));
        
        this.components.set('tutorialViewer', new TutorialViewer({
            container: document.getElementById('tutorialViewer'),
            tutorialSystem: this.tutorialSystem
        }));
        
        this.components.set('apiExplorer', new APIExplorer({
            container: document.getElementById('apiExplorer'),
            documentationEngine: this.documentationEngine
        }));
        
        this.components.set('performanceMonitor', new PerformanceMonitor({
            container: document.getElementById('performanceMonitor'),
            performanceIntegration: this.performanceIntegration
        }));
        
        // Initialize all components
        await this.dashboard.initialize();
        for (const [name, component] of this.components) {
            await component.initialize();
            this.logger.info(`Component '${name}' initialized`);
        }
        
        this.logger.info('UI Components initialized');
    }
    
    /**
     * Setup event listeners
     */
    setupEventListeners() {
        // Global error handling
        window.addEventListener('error', (event) => {
            this.logger.error('Global error:', event.error);
            this.handleGlobalError(event.error);
        });
        
        // Unhandled promise rejections
        window.addEventListener('unhandledrejection', (event) => {
            this.logger.error('Unhandled promise rejection:', event.reason);
            this.handleGlobalError(event.reason);
        });
        
        // Theme switching
        document.addEventListener('theme-change', (event) => {
            this.handleThemeChange(event.detail.theme);
        });
        
        // Keyboard shortcuts
        document.addEventListener('keydown', (event) => {
            this.handleKeyboardShortcuts(event);
        });
        
        // Resize handling
        window.addEventListener('resize', () => {
            this.handleWindowResize();
        });
        
        // Page visibility
        document.addEventListener('visibilitychange', () => {
            this.handleVisibilityChange();
        });
        
        // Before unload (save state)
        window.addEventListener('beforeunload', () => {
            this.saveUserState();
        });
    }
    
    /**
     * Setup system integrations
     */
    setupSystemIntegrations() {
        // Connect code execution to visualization
        this.codeExecutionEngine.on('execution-complete', (result) => {
            this.visualizationEngine.visualizeExecutionResult(result);
        });
        
        // Connect tutorial system to code execution
        this.tutorialSystem.on('code-exercise', (exercise) => {
            this.codeExecutionEngine.loadExercise(exercise);
        });
        
        // Connect search to navigation
        this.searchSystem.on('search-result-selected', (result) => {
            this.navigationSystem.navigateTo(result.path);
        });
        
        // Connect performance integration to UI
        this.performanceIntegration.on('metrics-updated', (metrics) => {
            this.components.get('performanceMonitor').updateMetrics(metrics);
        });
        
        // Connect documentation engine to all systems
        this.documentationEngine.on('content-loaded', (content) => {
            this.updateAllSystemsWithContent(content);
        });
        
        this.logger.info('System integrations established');
    }
    
    /**
     * Initialize user session
     */
    async initializeUserSession() {
        // Load user preferences
        const userPrefs = await this.storage.get('userPreferences') || {};
        this.applyUserPreferences(userPrefs);
        
        // Load user progress
        const userProgress = await this.storage.get('userProgress') || {};
        this.tutorialSystem.loadUserProgress(userProgress);
        
        // Initialize session tracking
        this.currentSession = {
            id: this.generateSessionId(),
            startTime: Date.now(),
            lastActivity: Date.now(),
            visitedPages: [],
            completedTutorials: [],
            codeExecutions: 0
        };
        
        this.logger.info('User session initialized:', this.currentSession.id);
    }
    
    /**
     * Start the application
     */
    start() {
        // Render the main dashboard
        this.dashboard.render();
        
        // Start performance monitoring
        this.performanceIntegration.startMonitoring();
        
        // Start update loops
        this.startUpdateLoop();
        
        // Initialize router
        this.navigationSystem.start();
        
        this.logger.info('Documentation application started');
    }
    
    /**
     * Start the main update loop
     */
    startUpdateLoop() {
        const update = (timestamp) => {
            const deltaTime = timestamp - (this.lastUpdateTime || timestamp);
            this.lastUpdateTime = timestamp;
            
            // Update systems
            this.visualizationEngine.update(deltaTime);
            this.performanceIntegration.update(deltaTime);
            
            // Update session activity
            this.currentSession.lastActivity = Date.now();
            
            // Continue loop
            requestAnimationFrame(update);
        };
        
        requestAnimationFrame(update);
    }
    
    /**
     * Handle keyboard shortcuts
     */
    handleKeyboardShortcuts(event) {
        // Ctrl/Cmd + K: Focus search
        if ((event.ctrlKey || event.metaKey) && event.key === 'k') {
            event.preventDefault();
            this.searchSystem.focusSearchInput();
        }
        
        // Ctrl/Cmd + Enter: Execute code
        if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
            const activePlayground = this.components.get('codePlayground');
            if (activePlayground && activePlayground.isActive()) {
                activePlayground.executeCode();
            }
        }
        
        // Escape: Close modals
        if (event.key === 'Escape') {
            this.closeActiveModals();
        }
        
        // F1: Show help
        if (event.key === 'F1') {
            event.preventDefault();
            this.showHelpModal();
        }
    }
    
    /**
     * Show welcome message or tutorial
     */
    showWelcome() {
        const isFirstTime = !this.storage.get('hasVisited');
        
        if (isFirstTime) {
            this.showFirstTimeWelcome();
            this.storage.set('hasVisited', true);
        } else {
            this.showReturningUserWelcome();
        }
    }
    
    /**
     * Show first-time welcome experience
     */
    showFirstTimeWelcome() {
        const welcomeModal = document.createElement('div');
        welcomeModal.className = 'welcome-modal';
        welcomeModal.innerHTML = `
            <div class="welcome-content">
                <h2>Welcome to ECScope Interactive Documentation!</h2>
                <p>This is a comprehensive educational platform for learning about ECS architecture, 
                   memory management, and performance optimization.</p>
                
                <div class="welcome-features">
                    <div class="feature">
                        <h3>🚀 Live Code Execution</h3>
                        <p>Run and experiment with real C++ code examples</p>
                    </div>
                    <div class="feature">
                        <h3>📊 Performance Analysis</h3>
                        <p>See real-time performance metrics and optimization suggestions</p>
                    </div>
                    <div class="feature">
                        <h3>🎓 Interactive Tutorials</h3>
                        <p>Learn through hands-on exercises and adaptive difficulty</p>
                    </div>
                    <div class="feature">
                        <h3>🔍 Intelligent Search</h3>
                        <p>Find information quickly with semantic search</p>
                    </div>
                </div>
                
                <div class="welcome-actions">
                    <button id="startTutorial" class="btn btn-primary">Start with Tutorial</button>
                    <button id="exploreAPI" class="btn btn-secondary">Explore API</button>
                    <button id="closeWelcome" class="btn btn-text">Skip for now</button>
                </div>
            </div>
        `;
        
        document.body.appendChild(welcomeModal);
        
        // Add event listeners
        document.getElementById('startTutorial').addEventListener('click', () => {
            this.tutorialSystem.startTutorial('ecs-basics-intro');
            this.closeWelcome();
        });
        
        document.getElementById('exploreAPI').addEventListener('click', () => {
            this.navigationSystem.navigateTo('/api/overview');
            this.closeWelcome();
        });
        
        document.getElementById('closeWelcome').addEventListener('click', () => {
            this.closeWelcome();
        });
    }
    
    /**
     * Handle global errors
     */
    handleGlobalError(error) {
        // Log error details
        this.logger.error('Global error caught:', error);
        
        // Show user-friendly error message
        this.showErrorMessage('An unexpected error occurred. The development team has been notified.');
        
        // Send error report if analytics enabled
        if (this.config.get('analytics.enabled')) {
            this.sendErrorReport(error);
        }
    }
    
    /**
     * Show error message to user
     */
    showErrorMessage(message) {
        const errorNotification = document.createElement('div');
        errorNotification.className = 'error-notification';
        errorNotification.innerHTML = `
            <div class="error-content">
                <span class="error-icon">⚠️</span>
                <span class="error-message">${message}</span>
                <button class="error-close">×</button>
            </div>
        `;
        
        document.body.appendChild(errorNotification);
        
        // Auto-remove after 5 seconds
        setTimeout(() => {
            if (errorNotification.parentNode) {
                errorNotification.parentNode.removeChild(errorNotification);
            }
        }, 5000);
        
        // Manual close
        errorNotification.querySelector('.error-close').addEventListener('click', () => {
            errorNotification.parentNode.removeChild(errorNotification);
        });
    }
    
    /**
     * Save user state before page unload
     */
    saveUserState() {
        const state = {
            currentPath: this.navigationSystem.getCurrentPath(),
            userProgress: this.tutorialSystem.getUserProgress(),
            sessionData: this.currentSession,
            preferences: this.getUserPreferences()
        };
        
        this.storage.set('userState', state);
    }
    
    /**
     * Generate unique session ID
     */
    generateSessionId() {
        return `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
    }
    
    /**
     * Get user preferences
     */
    getUserPreferences() {
        return {
            theme: document.documentElement.getAttribute('data-theme') || 'auto',
            language: this.config.get('language'),
            codeEditorSettings: this.components.get('codePlayground')?.getSettings() || {},
            tutorialSettings: this.tutorialSystem.getSettings()
        };
    }
    
    /**
     * Apply user preferences
     */
    applyUserPreferences(preferences) {
        if (preferences.theme) {
            document.documentElement.setAttribute('data-theme', preferences.theme);
        }
        
        if (preferences.codeEditorSettings) {
            // Apply when code playground is ready
            document.addEventListener('codePlaygroundReady', () => {
                this.components.get('codePlayground')?.applySettings(preferences.codeEditorSettings);
            });
        }
        
        if (preferences.tutorialSettings) {
            this.tutorialSystem.applySettings(preferences.tutorialSettings);
        }
    }
    
    /**
     * Close welcome modal
     */
    closeWelcome() {
        const welcomeModal = document.querySelector('.welcome-modal');
        if (welcomeModal) {
            welcomeModal.remove();
        }
    }
}

/**
 * Initialize the application when DOM is ready
 */
document.addEventListener('DOMContentLoaded', async () => {
    const app = new ECScopeDocumentationApp();
    
    // Make app globally available for debugging
    window.ecscopeApp = app;
    
    // Initialize the application
    await app.initialize();
});

// Handle module hot reloading in development
if (module.hot) {
    module.hot.accept();
}