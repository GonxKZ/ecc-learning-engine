/**
 * @file code-execution-engine.js
 * @brief Safe code execution engine for running C++ examples in the browser
 * 
 * This engine provides a secure environment for executing C++ code examples
 * with real-time performance monitoring, safety checks, and educational
 * insights integration.
 */

import { EventEmitter } from 'events';
import { Logger } from '../utils/logger';
import { SecurityValidator } from '../utils/security-validator';
import { PerformanceMonitor } from '../utils/performance-monitor';

/**
 * Code execution engine with safety and performance monitoring
 */
export class CodeExecutionEngine extends EventEmitter {
    constructor(config) {
        super();
        this.config = config;
        this.logger = new Logger('CodeExecutionEngine');
        
        // Core components
        this.securityValidator = new SecurityValidator();
        this.performanceMonitor = new PerformanceMonitor();
        
        // Execution environment
        this.wasmModule = null;
        this.executionContext = null;
        
        // State tracking
        this.activeExecutions = new Map();
        this.executionHistory = [];
        this.maxHistoryLength = 100;
        
        // Resource limits
        this.resourceLimits = {
            maxExecutionTime: config.timeoutMs || 30000,
            maxMemory: this.parseMemoryLimit(config.memoryLimit || '128MB'),
            maxCPUUsage: 80, // percentage
            maxOutputLength: 10000 // characters
        };
        
        // Safety configuration
        this.safetyConfig = {
            enableSafetyChecks: config.enableSafetyChecks !== false,
            allowedIncludes: this.getDefaultAllowedIncludes(),
            blockedFunctions: this.getDefaultBlockedFunctions(),
            enableSandboxing: true
        };
    }
    
    /**
     * Initialize the code execution engine
     */
    async initialize() {
        try {
            this.logger.info('Initializing Code Execution Engine...');
            
            // Check WebAssembly support
            if (!this.isWebAssemblySupported()) {
                throw new Error('WebAssembly is not supported in this browser');
            }
            
            // Load WebAssembly module
            await this.loadWasmModule();
            
            // Initialize execution context
            this.initializeExecutionContext();
            
            // Setup performance monitoring
            this.performanceMonitor.initialize();
            
            // Setup security validator
            await this.securityValidator.initialize();
            
            this.logger.info('Code Execution Engine initialized successfully');
            this.emit('initialized');
            
        } catch (error) {
            this.logger.error('Failed to initialize Code Execution Engine:', error);
            throw error;
        }
    }
    
    /**
     * Execute code with safety checks and performance monitoring
     */
    async executeCode(code, options = {}) {
        const executionId = this.generateExecutionId();
        
        try {
            this.logger.info(`Starting code execution: ${executionId}`);
            
            // Validate input
            this.validateExecutionInput(code, options);
            
            // Security validation
            const securityResult = await this.securityValidator.validateCode(code);
            if (!securityResult.safe) {
                throw new SecurityError('Code contains unsafe operations', securityResult.issues);
            }
            
            // Prepare execution environment
            const executionContext = this.createExecutionContext(options);
            
            // Start performance monitoring
            const performanceSession = this.performanceMonitor.startSession(executionId);
            
            // Track execution
            const executionInfo = {
                id: executionId,
                code,
                options,
                startTime: Date.now(),
                status: 'running'
            };
            this.activeExecutions.set(executionId, executionInfo);
            
            // Execute code with timeout
            const result = await this.executeWithTimeout(
                () => this.executeCodeInSandbox(code, executionContext),
                this.resourceLimits.maxExecutionTime
            );
            
            // Stop performance monitoring
            const performanceData = performanceSession.stop();
            
            // Process execution result
            const executionResult = this.processExecutionResult(
                result, 
                performanceData, 
                executionId
            );
            
            // Update execution info
            executionInfo.status = 'completed';
            executionInfo.endTime = Date.now();
            executionInfo.result = executionResult;
            
            // Add to history
            this.addToHistory(executionInfo);
            
            // Clean up active execution
            this.activeExecutions.delete(executionId);
            
            this.logger.info(`Code execution completed: ${executionId}`);
            this.emit('execution-complete', executionResult);
            
            return executionResult;
            
        } catch (error) {
            this.handleExecutionError(executionId, error);
            throw error;
        }
    }
    
    /**
     * Execute code in WebAssembly sandbox
     */
    async executeCodeInSandbox(code, context) {
        // Compile C++ code to WebAssembly
        const compiledModule = await this.compileCode(code, context.compileOptions);
        
        // Create isolated execution environment
        const sandbox = this.createSandbox(context);
        
        // Execute in sandbox with resource monitoring
        const result = await sandbox.execute(compiledModule, {
            stdin: context.input || '',
            env: context.environment || {},
            args: context.arguments || []
        });
        
        return result;
    }
    
    /**
     * Compile C++ code to WebAssembly
     */
    async compileCode(code, options = {}) {
        // Preprocess code
        const preprocessedCode = this.preprocessCode(code);
        
        // Add ECScope headers and includes
        const fullCode = this.wrapWithECScopeIncludes(preprocessedCode);
        
        // Compile using Emscripten (via WebAssembly backend)
        const compilationResult = await this.wasmModule.compile(fullCode, {
            optimizationLevel: options.optimization || 'O2',
            includeDebugInfo: options.debug || false,
            linkLibraries: this.getRequiredLibraries(code),
            compileFlags: this.getCompileFlags(options)
        });
        
        if (!compilationResult.success) {
            throw new CompilationError(compilationResult.errors);
        }
        
        return compilationResult.module;
    }
    
    /**
     * Create secure execution sandbox
     */
    createSandbox(context) {
        return {
            async execute(module, executionOptions) {
                // Create isolated memory space
                const memory = new WebAssembly.Memory({ 
                    initial: 256, // 16MB
                    maximum: 512  // 32MB
                });
                
                // Create execution instance
                const instance = await WebAssembly.instantiate(module, {
                    env: {
                        memory: memory,
                        // Provide safe system calls
                        ...this.getSafeSystemCalls(),
                        // ECScope-specific functions
                        ...this.getECScopeFunctions()
                    }
                });
                
                // Execute main function
                const startTime = performance.now();
                const result = instance.exports.main();
                const endTime = performance.now();
                
                // Capture output
                const output = this.captureOutput(instance);
                
                return {
                    exitCode: result,
                    output: output,
                    executionTime: endTime - startTime,
                    memoryUsage: this.getMemoryUsage(memory),
                    instance: instance
                };
            }
        };
    }
    
    /**
     * Process execution result with educational insights
     */
    processExecutionResult(rawResult, performanceData, executionId) {
        const result = {
            id: executionId,
            success: rawResult.exitCode === 0,
            output: rawResult.output,
            exitCode: rawResult.exitCode,
            executionTime: rawResult.executionTime,
            
            // Performance metrics
            performance: {
                executionTimeMs: rawResult.executionTime,
                memoryUsage: rawResult.memoryUsage,
                cpuUsage: performanceData.cpuUsage,
                cacheMetrics: performanceData.cacheMetrics,
                instructions: performanceData.instructions
            },
            
            // Educational insights
            insights: this.generateEducationalInsights(rawResult, performanceData),
            
            // Optimization suggestions
            optimizations: this.generateOptimizationSuggestions(rawResult, performanceData),
            
            // Visualization data
            visualizations: this.generateVisualizationData(rawResult, performanceData),
            
            // Metadata
            timestamp: Date.now(),
            codeHash: this.hashCode(rawResult.originalCode)
        };
        
        return result;
    }
    
    /**
     * Generate educational insights from execution results
     */
    generateEducationalInsights(result, performanceData) {
        const insights = [];
        
        // Performance insights
        if (result.executionTime > 100) {
            insights.push({
                type: 'performance',
                level: 'info',
                title: 'Execution Time',
                message: `This code took ${result.executionTime.toFixed(2)}ms to execute. Consider optimizing for better performance.`,
                suggestions: [
                    'Use more efficient algorithms',
                    'Minimize memory allocations',
                    'Consider caching frequently used values'
                ]
            });
        }
        
        // Memory insights
        if (result.memoryUsage.peak > 1024 * 1024) { // > 1MB
            insights.push({
                type: 'memory',
                level: 'warning',
                title: 'High Memory Usage',
                message: `Peak memory usage was ${this.formatMemory(result.memoryUsage.peak)}. This might indicate memory inefficiency.`,
                suggestions: [
                    'Use object pooling for frequently allocated objects',
                    'Consider RAII patterns for automatic cleanup',
                    'Profile memory allocations to find hotspots'
                ]
            });
        }
        
        // Cache insights
        if (performanceData.cacheMetrics && performanceData.cacheMetrics.hitRatio < 0.8) {
            insights.push({
                type: 'cache',
                level: 'tip',
                title: 'Cache Performance',
                message: `Cache hit ratio is ${(performanceData.cacheMetrics.hitRatio * 100).toFixed(1)}%. Better data layout could improve performance.`,
                suggestions: [
                    'Use Structure of Arrays (SoA) instead of Array of Structures (AoS)',
                    'Group related data together',
                    'Minimize random memory access patterns'
                ]
            });
        }
        
        // Code quality insights
        const codeQuality = this.analyzeCodeQuality(result.originalCode);
        if (codeQuality.issues.length > 0) {
            insights.push({
                type: 'code-quality',
                level: 'info',
                title: 'Code Quality',
                message: 'Some code quality improvements could be made:',
                suggestions: codeQuality.issues
            });
        }
        
        return insights;
    }
    
    /**
     * Generate optimization suggestions
     */
    generateOptimizationSuggestions(result, performanceData) {
        const suggestions = [];
        
        // Algorithm optimization
        if (result.executionTime > 50) {
            suggestions.push({
                type: 'algorithm',
                priority: 'high',
                title: 'Algorithm Optimization',
                description: 'Consider using more efficient algorithms or data structures',
                examples: [
                    'Use hash tables for O(1) lookups instead of linear search',
                    'Consider caching expensive calculations',
                    'Use early termination conditions where possible'
                ]
            });
        }
        
        // Memory optimization
        if (result.memoryUsage.allocations > 100) {
            suggestions.push({
                type: 'memory',
                priority: 'medium',
                title: 'Reduce Memory Allocations',
                description: 'Minimize dynamic memory allocations for better performance',
                examples: [
                    'Use stack allocation where possible',
                    'Implement object pooling for frequently created objects',
                    'Pre-allocate containers to avoid reallocations'
                ]
            });
        }
        
        // ECS-specific optimizations
        if (this.detectECSUsage(result.originalCode)) {
            suggestions.push({
                type: 'ecs',
                priority: 'high',
                title: 'ECS Optimization',
                description: 'Optimize Entity-Component-System patterns',
                examples: [
                    'Use archetype-based storage for better cache locality',
                    'Batch similar operations together',
                    'Minimize structural changes during updates'
                ]
            });
        }
        
        return suggestions;
    }
    
    /**
     * Generate visualization data for execution results
     */
    generateVisualizationData(result, performanceData) {
        return {
            // Memory usage over time
            memoryTimeline: performanceData.memoryTimeline || [],
            
            // CPU usage graph
            cpuUsage: performanceData.cpuTimeline || [],
            
            // Call stack visualization
            callStack: performanceData.callStack || [],
            
            // Memory layout (if ECS code)
            memoryLayout: this.generateMemoryLayoutVisualization(result),
            
            // Performance comparison data
            performanceComparison: this.generatePerformanceComparison(result),
            
            // Code execution flow
            executionFlow: this.generateExecutionFlowData(result)
        };
    }
    
    /**
     * Stop code execution
     */
    stopExecution(executionId) {
        const execution = this.activeExecutions.get(executionId);
        if (!execution) {
            throw new Error(`No active execution found with ID: ${executionId}`);
        }
        
        // Terminate execution
        execution.status = 'stopped';
        
        // Clean up resources
        this.cleanupExecution(executionId);
        
        this.logger.info(`Execution stopped: ${executionId}`);
        this.emit('execution-stopped', { id: executionId });
    }
    
    /**
     * Get execution history
     */
    getExecutionHistory(limit = 10) {
        return this.executionHistory.slice(-limit);
    }
    
    /**
     * Get active executions
     */
    getActiveExecutions() {
        return Array.from(this.activeExecutions.values());
    }
    
    /**
     * Load exercise for tutorial system
     */
    loadExercise(exercise) {
        return {
            id: exercise.id,
            title: exercise.title,
            description: exercise.description,
            starterCode: exercise.starterCode,
            solution: exercise.solution,
            tests: exercise.tests,
            hints: exercise.hints,
            expectedOutput: exercise.expectedOutput,
            isLoaded: true
        };
    }
    
    /**
     * Validate exercise solution
     */
    async validateSolution(exerciseId, code) {
        const exercise = this.loadedExercises.get(exerciseId);
        if (!exercise) {
            throw new Error(`Exercise not found: ${exerciseId}`);
        }
        
        // Run user code
        const userResult = await this.executeCode(code, {
            timeout: 10000,
            memoryLimit: '64MB'
        });
        
        // Run tests
        const testResults = [];
        for (const test of exercise.tests) {
            const testResult = await this.runTest(code, test);
            testResults.push(testResult);
        }
        
        // Validate output
        const isCorrect = this.validateOutput(userResult.output, exercise.expectedOutput);
        
        return {
            correct: isCorrect && testResults.every(t => t.passed),
            score: this.calculateScore(testResults),
            testResults: testResults,
            feedback: this.generateFeedback(testResults, userResult),
            hints: isCorrect ? [] : this.getNextHint(exercise, testResults)
        };
    }
    
    // Utility methods
    
    isWebAssemblySupported() {
        return typeof WebAssembly === 'object' && 
               typeof WebAssembly.instantiate === 'function';
    }
    
    async loadWasmModule() {
        try {
            // In a real implementation, this would load the ECScope WebAssembly module
            const response = await fetch(this.config.wasmPath || '/wasm/ecscope.wasm');
            const bytes = await response.arrayBuffer();
            this.wasmModule = await WebAssembly.compile(bytes);
            
        } catch (error) {
            this.logger.error('Failed to load WebAssembly module:', error);
            throw error;
        }
    }
    
    initializeExecutionContext() {
        this.executionContext = {
            globalScope: new Map(),
            includeDirectories: ['/include/ecscope'],
            linkLibraries: ['ecscope'],
            compilerFlags: ['-std=c++20', '-O2']
        };
    }
    
    parseMemoryLimit(limitStr) {
        const units = {
            'KB': 1024,
            'MB': 1024 * 1024,
            'GB': 1024 * 1024 * 1024
        };
        
        const match = limitStr.match(/^(\d+)\s*([KMGT]?B)$/i);
        if (!match) return 128 * 1024 * 1024; // Default 128MB
        
        const [, amount, unit] = match;
        return parseInt(amount) * (units[unit.toUpperCase()] || 1);
    }
    
    getDefaultAllowedIncludes() {
        return [
            'iostream',
            'vector',
            'string',
            'memory',
            'algorithm',
            'chrono',
            'ecscope/core.hpp',
            'ecscope/ecs.hpp',
            'ecscope/memory.hpp',
            'ecscope/physics.hpp',
            'ecscope/renderer.hpp'
        ];
    }
    
    getDefaultBlockedFunctions() {
        return [
            'system',
            'exec',
            'fork',
            'exit',
            'abort',
            'signal',
            'kill',
            'fopen',
            'fwrite',
            'fread',
            'malloc',
            'free',
            'new',
            'delete'
        ];
    }
    
    generateExecutionId() {
        return `exec_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    }
    
    validateExecutionInput(code, options) {
        if (!code || typeof code !== 'string') {
            throw new Error('Code must be a non-empty string');
        }
        
        if (code.length > 100000) { // 100KB limit
            throw new Error('Code is too long (max 100KB)');
        }
        
        if (options && typeof options !== 'object') {
            throw new Error('Options must be an object');
        }
    }
    
    createExecutionContext(options) {
        return {
            compileOptions: {
                optimization: options.optimization || 'O2',
                debug: options.debug || false,
                warnings: options.warnings || true
            },
            input: options.input || '',
            arguments: options.arguments || [],
            environment: options.environment || {},
            timeout: Math.min(options.timeout || 30000, this.resourceLimits.maxExecutionTime)
        };
    }
    
    async executeWithTimeout(fn, timeoutMs) {
        return Promise.race([
            fn(),
            new Promise((_, reject) => {
                setTimeout(() => reject(new Error('Execution timeout')), timeoutMs);
            })
        ]);
    }
    
    addToHistory(executionInfo) {
        this.executionHistory.push(executionInfo);
        if (this.executionHistory.length > this.maxHistoryLength) {
            this.executionHistory.shift();
        }
    }
    
    handleExecutionError(executionId, error) {
        const execution = this.activeExecutions.get(executionId);
        if (execution) {
            execution.status = 'error';
            execution.error = error.message;
            execution.endTime = Date.now();
        }
        
        this.cleanupExecution(executionId);
        
        this.logger.error(`Execution failed: ${executionId}`, error);
        this.emit('execution-error', { id: executionId, error });
    }
    
    cleanupExecution(executionId) {
        this.activeExecutions.delete(executionId);
        // Additional cleanup logic would go here
    }
    
    hashCode(str) {
        let hash = 0;
        if (str.length === 0) return hash;
        for (let i = 0; i < str.length; i++) {
            const char = str.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash = hash & hash; // Convert to 32-bit integer
        }
        return hash.toString();
    }
    
    formatMemory(bytes) {
        const units = ['B', 'KB', 'MB', 'GB'];
        let size = bytes;
        let unitIndex = 0;
        
        while (size >= 1024 && unitIndex < units.length - 1) {
            size /= 1024;
            unitIndex++;
        }
        
        return `${size.toFixed(2)} ${units[unitIndex]}`;
    }
}

/**
 * Custom error classes
 */
class SecurityError extends Error {
    constructor(message, issues = []) {
        super(message);
        this.name = 'SecurityError';
        this.issues = issues;
    }
}

class CompilationError extends Error {
    constructor(errors = []) {
        super(`Compilation failed: ${errors.join(', ')}`);
        this.name = 'CompilationError';
        this.errors = errors;
    }
}