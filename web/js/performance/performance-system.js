/**
 * @file performance-system.js
 * @brief Complete Performance Monitoring and Analysis System for ECScope WebAssembly
 * @description Production-ready performance profiling with real-time metrics and visualization
 */

'use strict';

/**
 * Complete Performance Monitoring System
 */
class WebPerformanceSystem {
    constructor(app) {
        this.app = app;
        this.isEnabled = true;
        this.samplingRate = 60; // 60 samples per second
        this.maxSamples = 3600; // 1 minute of samples at 60fps
        
        // Metrics storage
        this.metrics = {
            fps: new CircularBuffer(this.maxSamples),
            frameTime: new CircularBuffer(this.maxSamples),
            memoryUsage: new CircularBuffer(this.maxSamples),
            entityCount: new CircularBuffer(this.maxSamples),
            componentCount: new CircularBuffer(this.maxSamples),
            systemTime: new CircularBuffer(this.maxSamples),
            renderTime: new CircularBuffer(this.maxSamples),
            updateTime: new CircularBuffer(this.maxSamples),
            wasmHeapSize: new CircularBuffer(this.maxSamples),
            jsHeapSize: new CircularBuffer(this.maxSamples),
            drawCalls: new CircularBuffer(this.maxSamples),
            triangles: new CircularBuffer(this.maxSamples)
        };
        
        // Performance thresholds
        this.thresholds = {
            fps: { warning: 45, critical: 30 },
            frameTime: { warning: 22, critical: 33 }, // ms
            memory: { warning: 0.8, critical: 0.95 }, // percentage of max
            entityCount: { warning: 10000, critical: 50000 },
            drawCalls: { warning: 1000, critical: 2000 }
        };
        
        // Performance state
        this.currentState = {
            fps: 0,
            frameTime: 0,
            memoryUsage: 0,
            entityCount: 0,
            componentCount: 0,
            drawCalls: 0,
            triangles: 0,
            status: 'good' // good, warning, critical
        };
        
        // Profiling tools
        this.profiler = new ECSProfiler(this);
        this.memoryAnalyzer = new MemoryAnalyzer(this);
        this.performanceAnalyzer = new PerformanceAnalyzer(this);
        this.alertSystem = new PerformanceAlertSystem(this);
        
        // Visualization
        this.charts = new Map();
        this.visualizer = new PerformanceVisualizer(this);
        
        // Sampling control
        this.lastSampleTime = 0;
        this.sampleInterval = 1000 / this.samplingRate; // ms between samples
        
        this.initialize();
    }
    
    /**
     * Initialize performance monitoring system
     */
    initialize() {
        this.setupPerformanceObserver();
        this.setupMemoryMonitoring();
        this.createPerformanceUI();
        this.startMonitoring();
        
        console.log('Web Performance System initialized');
    }
    
    /**
     * Setup Performance Observer API for detailed metrics
     */
    setupPerformanceObserver() {
        if ('PerformanceObserver' in window) {
            // Observe various performance entries
            this.setupNavigationObserver();
            this.setupResourceObserver();
            this.setupMeasureObserver();
            this.setupLongTaskObserver();
        }
    }
    
    setupNavigationObserver() {
        try {
            const observer = new PerformanceObserver((list) => {
                const entries = list.getEntries();
                entries.forEach(entry => {
                    this.processNavigationTiming(entry);
                });
            });
            observer.observe({ entryTypes: ['navigation'] });
        } catch (error) {
            console.warn('Navigation performance observer not supported:', error);
        }
    }
    
    setupResourceObserver() {
        try {
            const observer = new PerformanceObserver((list) => {
                const entries = list.getEntries();
                entries.forEach(entry => {
                    this.processResourceTiming(entry);
                });
            });
            observer.observe({ entryTypes: ['resource'] });
        } catch (error) {
            console.warn('Resource performance observer not supported:', error);
        }
    }
    
    setupMeasureObserver() {
        try {
            const observer = new PerformanceObserver((list) => {
                const entries = list.getEntries();
                entries.forEach(entry => {
                    this.processUserTiming(entry);
                });
            });
            observer.observe({ entryTypes: ['measure'] });
        } catch (error) {
            console.warn('Measure performance observer not supported:', error);
        }
    }
    
    setupLongTaskObserver() {
        try {
            const observer = new PerformanceObserver((list) => {
                const entries = list.getEntries();
                entries.forEach(entry => {
                    this.processLongTask(entry);
                });
            });
            observer.observe({ entryTypes: ['longtask'] });
        } catch (error) {
            console.warn('Long task observer not supported:', error);
        }
    }
    
    /**
     * Setup memory monitoring
     */
    setupMemoryMonitoring() {
        // Monitor JavaScript heap if available
        if ('memory' in performance) {
            this.jsMemorySupported = true;
        }
        
        // Monitor WebAssembly heap
        this.wasmMemorySupported = true;
    }
    
    /**
     * Create performance monitoring UI
     */
    createPerformanceUI() {
        this.visualizer.createUI();
    }
    
    /**
     * Start performance monitoring
     */
    startMonitoring() {
        this.isMonitoring = true;
        console.log('Performance monitoring started');
    }
    
    /**
     * Stop performance monitoring
     */
    stopMonitoring() {
        this.isMonitoring = false;
        console.log('Performance monitoring stopped');
    }
    
    /**
     * Update performance metrics (called each frame)
     */
    update(deltaTime) {
        if (!this.isEnabled || !this.isMonitoring) return;
        
        const currentTime = performance.now();
        
        // Check if it's time to sample
        if (currentTime - this.lastSampleTime >= this.sampleInterval) {
            this.collectMetrics(deltaTime);
            this.analyzePerformance();
            this.updateVisualization();
            this.checkAlerts();
            
            this.lastSampleTime = currentTime;
        }
    }
    
    /**
     * Collect all performance metrics
     */
    collectMetrics(deltaTime) {
        const timestamp = performance.now();
        
        // Frame metrics
        const fps = Math.round(1000 / (deltaTime * 1000));
        const frameTime = deltaTime * 1000;
        
        this.metrics.fps.push({ value: fps, timestamp });
        this.metrics.frameTime.push({ value: frameTime, timestamp });
        
        // Memory metrics
        this.collectMemoryMetrics(timestamp);
        
        // ECS metrics
        this.collectECSMetrics(timestamp);
        
        // WebGL metrics
        this.collectWebGLMetrics(timestamp);
        
        // Update current state
        this.updateCurrentState(fps, frameTime);
    }
    
    /**
     * Collect memory metrics
     */
    collectMemoryMetrics(timestamp) {
        // JavaScript heap
        if (this.jsMemorySupported && performance.memory) {
            const jsHeapUsed = performance.memory.usedJSHeapSize;
            const jsHeapSize = performance.memory.totalJSHeapSize;
            const jsHeapUsage = jsHeapUsed / jsHeapSize;
            
            this.metrics.jsHeapSize.push({ 
                value: jsHeapUsed / 1024 / 1024, // MB
                timestamp 
            });
        }
        
        // WebAssembly heap
        if (this.wasmMemorySupported && this.app.getModule()) {
            const module = this.app.getModule();
            if (module._emscripten_get_heap_size) {
                const wasmHeapSize = module._emscripten_get_heap_size();
                this.metrics.wasmHeapSize.push({ 
                    value: wasmHeapSize / 1024 / 1024, // MB
                    timestamp 
                });
            }
        }
        
        // Total memory estimate
        const totalMemory = (this.metrics.jsHeapSize.current()?.value || 0) + 
                           (this.metrics.wasmHeapSize.current()?.value || 0);
        
        this.metrics.memoryUsage.push({ value: totalMemory, timestamp });
    }
    
    /**
     * Collect ECS-specific metrics
     */
    collectECSMetrics(timestamp) {
        const registry = this.app.getRegistry();
        if (registry) {
            // Entity count
            if (registry.getEntityCount) {
                const entityCount = registry.getEntityCount();
                this.metrics.entityCount.push({ value: entityCount, timestamp });
            }
            
            // Component count
            if (registry.getComponentCount) {
                const componentCount = registry.getComponentCount();
                this.metrics.componentCount.push({ value: componentCount, timestamp });
            }
        }
        
        // System timing (if profiler is active)
        if (this.profiler.isEnabled()) {
            const systemMetrics = this.profiler.getSystemMetrics();
            const totalSystemTime = Object.values(systemMetrics)
                .reduce((total, time) => total + time, 0);
            
            this.metrics.systemTime.push({ value: totalSystemTime, timestamp });
        }
    }
    
    /**
     * Collect WebGL rendering metrics
     */
    collectWebGLMetrics(timestamp) {
        const webglContext = this.app.getWebGLContext();
        if (webglContext && webglContext.getStats) {
            const stats = webglContext.getStats();
            
            this.metrics.drawCalls.push({ value: stats.draw_calls || 0, timestamp });
            this.metrics.triangles.push({ value: stats.triangles || 0, timestamp });
            
            // Reset counters for next frame
            if (webglContext.resetStats) {
                webglContext.resetStats();
            }
        }
    }
    
    /**
     * Update current performance state
     */
    updateCurrentState(fps, frameTime) {
        this.currentState.fps = fps;
        this.currentState.frameTime = frameTime;
        this.currentState.memoryUsage = this.metrics.memoryUsage.current()?.value || 0;
        this.currentState.entityCount = this.metrics.entityCount.current()?.value || 0;
        this.currentState.componentCount = this.metrics.componentCount.current()?.value || 0;
        this.currentState.drawCalls = this.metrics.drawCalls.current()?.value || 0;
        this.currentState.triangles = this.metrics.triangles.current()?.value || 0;
        
        // Determine overall status
        this.currentState.status = this.determinePerformanceStatus();
    }
    
    /**
     * Determine overall performance status
     */
    determinePerformanceStatus() {
        const { fps, frameTime, memoryUsage, drawCalls } = this.currentState;
        
        // Check critical thresholds
        if (fps < this.thresholds.fps.critical ||
            frameTime > this.thresholds.frameTime.critical ||
            drawCalls > this.thresholds.drawCalls.critical) {
            return 'critical';
        }
        
        // Check warning thresholds
        if (fps < this.thresholds.fps.warning ||
            frameTime > this.thresholds.frameTime.warning ||
            drawCalls > this.thresholds.drawCalls.warning) {
            return 'warning';
        }
        
        return 'good';
    }
    
    /**
     * Analyze performance trends and patterns
     */
    analyzePerformance() {
        this.performanceAnalyzer.analyze();
    }
    
    /**
     * Update performance visualization
     */
    updateVisualization() {
        this.visualizer.update(this.currentState, this.metrics);
    }
    
    /**
     * Check for performance alerts
     */
    checkAlerts() {
        this.alertSystem.checkAlerts(this.currentState);
    }
    
    /**
     * Process navigation timing data
     */
    processNavigationTiming(entry) {
        // Extract useful navigation metrics
        const navigationMetrics = {
            domContentLoaded: entry.domContentLoadedEventEnd - entry.domContentLoadedEventStart,
            loadComplete: entry.loadEventEnd - entry.loadEventStart,
            totalTime: entry.loadEventEnd - entry.fetchStart
        };
        
        console.log('Navigation metrics:', navigationMetrics);
    }
    
    /**
     * Process resource timing data
     */
    processResourceTiming(entry) {
        // Track resource loading performance
        if (entry.name.includes('.wasm') || entry.name.includes('.js')) {
            console.log(`Resource ${entry.name}: ${entry.duration.toFixed(2)}ms`);
        }
    }
    
    /**
     * Process user timing data
     */
    processUserTiming(entry) {
        // Handle custom performance marks and measures
        console.log(`Custom timing ${entry.name}: ${entry.duration.toFixed(2)}ms`);
    }
    
    /**
     * Process long task data
     */
    processLongTask(entry) {
        // Warn about long tasks that block the main thread
        console.warn(`Long task detected: ${entry.duration.toFixed(2)}ms`);
        this.alertSystem.reportLongTask(entry);
    }
    
    /**
     * Create custom performance mark
     */
    mark(name) {
        performance.mark(name);
    }
    
    /**
     * Create custom performance measure
     */
    measure(name, startMark, endMark) {
        performance.measure(name, startMark, endMark);
    }
    
    /**
     * Get current performance metrics
     */
    getCurrentMetrics() {
        return { ...this.currentState };
    }
    
    /**
     * Get historical metrics
     */
    getHistoricalMetrics() {
        const result = {};
        for (const [name, buffer] of Object.entries(this.metrics)) {
            result[name] = buffer.getAll();
        }
        return result;
    }
    
    /**
     * Get performance summary
     */
    getPerformanceSummary() {
        const summary = {};
        
        for (const [name, buffer] of Object.entries(this.metrics)) {
            const data = buffer.getAll();
            if (data.length > 0) {
                const values = data.map(d => d.value);
                summary[name] = {
                    current: values[values.length - 1],
                    average: values.reduce((a, b) => a + b, 0) / values.length,
                    min: Math.min(...values),
                    max: Math.max(...values),
                    count: values.length
                };
            }
        }
        
        return summary;
    }
    
    /**
     * Export performance data
     */
    exportPerformanceData() {
        const data = {
            timestamp: Date.now(),
            currentState: this.currentState,
            summary: this.getPerformanceSummary(),
            thresholds: this.thresholds,
            systemInfo: this.getSystemInfo()
        };
        
        return JSON.stringify(data, null, 2);
    }
    
    /**
     * Get system information
     */
    getSystemInfo() {
        return {
            userAgent: navigator.userAgent,
            platform: navigator.platform,
            language: navigator.language,
            cookieEnabled: navigator.cookieEnabled,
            onLine: navigator.onLine,
            webgl: this.getWebGLInfo(),
            memory: this.getMemoryInfo()
        };
    }
    
    /**
     * Get WebGL information
     */
    getWebGLInfo() {
        const canvas = document.createElement('canvas');
        const gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
        
        if (!gl) return null;
        
        return {
            version: gl.getParameter(gl.VERSION),
            vendor: gl.getParameter(gl.VENDOR),
            renderer: gl.getParameter(gl.RENDERER),
            shadingLanguageVersion: gl.getParameter(gl.SHADING_LANGUAGE_VERSION),
            maxTextureSize: gl.getParameter(gl.MAX_TEXTURE_SIZE),
            maxVertexAttribs: gl.getParameter(gl.MAX_VERTEX_ATTRIBS),
            maxViewportDims: gl.getParameter(gl.MAX_VIEWPORT_DIMS)
        };
    }
    
    /**
     * Get memory information
     */
    getMemoryInfo() {
        if ('memory' in performance) {
            return {
                jsHeapSizeLimit: performance.memory.jsHeapSizeLimit,
                totalJSHeapSize: performance.memory.totalJSHeapSize,
                usedJSHeapSize: performance.memory.usedJSHeapSize
            };
        }
        return null;
    }
    
    /**
     * Enable/disable performance monitoring
     */
    setEnabled(enabled) {
        this.isEnabled = enabled;
        
        if (enabled) {
            this.startMonitoring();
        } else {
            this.stopMonitoring();
        }
    }
    
    /**
     * Set sampling rate
     */
    setSamplingRate(rate) {
        this.samplingRate = Math.max(1, Math.min(120, rate));
        this.sampleInterval = 1000 / this.samplingRate;
    }
    
    /**
     * Cleanup performance monitoring
     */
    cleanup() {
        this.stopMonitoring();
        this.visualizer.cleanup();
        console.log('Web Performance System cleaned up');
    }
}

/**
 * Circular buffer for efficient metric storage
 */
class CircularBuffer {
    constructor(maxSize) {
        this.maxSize = maxSize;
        this.buffer = new Array(maxSize);
        this.head = 0;
        this.size = 0;
    }
    
    push(value) {
        this.buffer[this.head] = value;
        this.head = (this.head + 1) % this.maxSize;
        
        if (this.size < this.maxSize) {
            this.size++;
        }
    }
    
    current() {
        if (this.size === 0) return null;
        const index = (this.head - 1 + this.maxSize) % this.maxSize;
        return this.buffer[index];
    }
    
    getAll() {
        const result = [];
        for (let i = 0; i < this.size; i++) {
            const index = (this.head - this.size + i + this.maxSize) % this.maxSize;
            result.push(this.buffer[index]);
        }
        return result;
    }
    
    clear() {
        this.head = 0;
        this.size = 0;
    }
}

/**
 * ECS-specific profiler
 */
class ECSProfiler {
    constructor(performanceSystem) {
        this.performanceSystem = performanceSystem;
        this.enabled = false;
        this.systemMetrics = new Map();
        this.frameSystemTimes = new Map();
    }
    
    enable() {
        this.enabled = true;
        console.log('ECS Profiler enabled');
    }
    
    disable() {
        this.enabled = false;
        console.log('ECS Profiler disabled');
    }
    
    isEnabled() {
        return this.enabled;
    }
    
    startSystemTiming(systemName) {
        if (!this.enabled) return;
        
        performance.mark(`system-${systemName}-start`);
    }
    
    endSystemTiming(systemName) {
        if (!this.enabled) return;
        
        performance.mark(`system-${systemName}-end`);
        performance.measure(`system-${systemName}`, `system-${systemName}-start`, `system-${systemName}-end`);
        
        // Get the measure
        const entries = performance.getEntriesByName(`system-${systemName}`, 'measure');
        if (entries.length > 0) {
            const duration = entries[entries.length - 1].duration;
            this.frameSystemTimes.set(systemName, duration);
        }
    }
    
    getSystemMetrics() {
        return Object.fromEntries(this.frameSystemTimes);
    }
    
    reset() {
        this.frameSystemTimes.clear();
    }
}

/**
 * Memory analyzer
 */
class MemoryAnalyzer {
    constructor(performanceSystem) {
        this.performanceSystem = performanceSystem;
        this.memoryLeakDetector = new MemoryLeakDetector();
    }
    
    analyze() {
        this.memoryLeakDetector.check();
    }
    
    getMemoryReport() {
        return this.memoryLeakDetector.getReport();
    }
}

/**
 * Memory leak detector
 */
class MemoryLeakDetector {
    constructor() {
        this.samples = [];
        this.maxSamples = 100;
        this.threshold = 1.5; // MB growth threshold
    }
    
    check() {
        if ('memory' in performance) {
            const sample = {
                timestamp: Date.now(),
                used: performance.memory.usedJSHeapSize,
                total: performance.memory.totalJSHeapSize
            };
            
            this.samples.push(sample);
            
            if (this.samples.length > this.maxSamples) {
                this.samples.shift();
            }
            
            this.detectLeaks();
        }
    }
    
    detectLeaks() {
        if (this.samples.length < 10) return;
        
        const recent = this.samples.slice(-10);
        const older = this.samples.slice(-20, -10);
        
        if (older.length < 10) return;
        
        const recentAvg = recent.reduce((sum, s) => sum + s.used, 0) / recent.length;
        const olderAvg = older.reduce((sum, s) => sum + s.used, 0) / older.length;
        
        const growthMB = (recentAvg - olderAvg) / 1024 / 1024;
        
        if (growthMB > this.threshold) {
            console.warn(`Potential memory leak detected: ${growthMB.toFixed(2)}MB growth`);
        }
    }
    
    getReport() {
        return {
            samples: this.samples.length,
            currentUsage: this.samples.length > 0 ? this.samples[this.samples.length - 1].used : 0,
            trend: this.calculateTrend()
        };
    }
    
    calculateTrend() {
        if (this.samples.length < 2) return 'insufficient-data';
        
        const first = this.samples[0];
        const last = this.samples[this.samples.length - 1];
        const growthMB = (last.used - first.used) / 1024 / 1024;
        
        if (growthMB > this.threshold) return 'increasing';
        if (growthMB < -this.threshold) return 'decreasing';
        return 'stable';
    }
}

/**
 * Performance analyzer for trend detection
 */
class PerformanceAnalyzer {
    constructor(performanceSystem) {
        this.performanceSystem = performanceSystem;
        this.trends = new Map();
    }
    
    analyze() {
        this.analyzeFPSTrend();
        this.analyzeMemoryTrend();
        this.analyzeSystemLoadTrend();
    }
    
    analyzeFPSTrend() {
        const fpsData = this.performanceSystem.metrics.fps.getAll();
        if (fpsData.length < 10) return;
        
        const trend = this.calculateTrend(fpsData.map(d => d.value));
        this.trends.set('fps', trend);
    }
    
    analyzeMemoryTrend() {
        const memoryData = this.performanceSystem.metrics.memoryUsage.getAll();
        if (memoryData.length < 10) return;
        
        const trend = this.calculateTrend(memoryData.map(d => d.value));
        this.trends.set('memory', trend);
    }
    
    analyzeSystemLoadTrend() {
        const frameTimeData = this.performanceSystem.metrics.frameTime.getAll();
        if (frameTimeData.length < 10) return;
        
        const trend = this.calculateTrend(frameTimeData.map(d => d.value));
        this.trends.set('systemLoad', trend);
    }
    
    calculateTrend(values) {
        if (values.length < 2) return 'stable';
        
        // Simple linear regression slope
        const n = values.length;
        const sumX = (n * (n - 1)) / 2;
        const sumY = values.reduce((sum, val) => sum + val, 0);
        const sumXY = values.reduce((sum, val, i) => sum + (i * val), 0);
        const sumXX = (n * (n - 1) * (2 * n - 1)) / 6;
        
        const slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
        
        if (Math.abs(slope) < 0.01) return 'stable';
        return slope > 0 ? 'increasing' : 'decreasing';
    }
    
    getTrends() {
        return Object.fromEntries(this.trends);
    }
}

/**
 * Performance alert system
 */
class PerformanceAlertSystem {
    constructor(performanceSystem) {
        this.performanceSystem = performanceSystem;
        this.alerts = [];
        this.maxAlerts = 50;
        this.alertCooldown = new Map();
        this.cooldownPeriod = 5000; // 5 seconds
    }
    
    checkAlerts(currentState) {
        this.checkFPSAlert(currentState.fps);
        this.checkFrameTimeAlert(currentState.frameTime);
        this.checkMemoryAlert(currentState.memoryUsage);
        this.checkDrawCallAlert(currentState.drawCalls);
    }
    
    checkFPSAlert(fps) {
        const thresholds = this.performanceSystem.thresholds.fps;
        
        if (fps < thresholds.critical) {
            this.createAlert('fps-critical', `Critical FPS: ${fps}`, 'critical');
        } else if (fps < thresholds.warning) {
            this.createAlert('fps-warning', `Low FPS: ${fps}`, 'warning');
        }
    }
    
    checkFrameTimeAlert(frameTime) {
        const thresholds = this.performanceSystem.thresholds.frameTime;
        
        if (frameTime > thresholds.critical) {
            this.createAlert('frametime-critical', `High frame time: ${frameTime.toFixed(1)}ms`, 'critical');
        } else if (frameTime > thresholds.warning) {
            this.createAlert('frametime-warning', `Elevated frame time: ${frameTime.toFixed(1)}ms`, 'warning');
        }
    }
    
    checkMemoryAlert(memoryUsage) {
        const thresholds = this.performanceSystem.thresholds.memory;
        const maxMemory = 512; // MB estimate
        const usagePercent = memoryUsage / maxMemory;
        
        if (usagePercent > thresholds.critical) {
            this.createAlert('memory-critical', `High memory usage: ${memoryUsage.toFixed(1)}MB`, 'critical');
        } else if (usagePercent > thresholds.warning) {
            this.createAlert('memory-warning', `Elevated memory usage: ${memoryUsage.toFixed(1)}MB`, 'warning');
        }
    }
    
    checkDrawCallAlert(drawCalls) {
        const thresholds = this.performanceSystem.thresholds.drawCalls;
        
        if (drawCalls > thresholds.critical) {
            this.createAlert('drawcalls-critical', `High draw calls: ${drawCalls}`, 'critical');
        } else if (drawCalls > thresholds.warning) {
            this.createAlert('drawcalls-warning', `Many draw calls: ${drawCalls}`, 'warning');
        }
    }
    
    createAlert(type, message, severity) {
        // Check cooldown
        const lastAlert = this.alertCooldown.get(type);
        const now = Date.now();
        
        if (lastAlert && (now - lastAlert) < this.cooldownPeriod) {
            return; // Still in cooldown
        }
        
        const alert = {
            id: this.generateAlertId(),
            type,
            message,
            severity,
            timestamp: now
        };
        
        this.alerts.push(alert);
        this.alertCooldown.set(type, now);
        
        // Limit number of stored alerts
        if (this.alerts.length > this.maxAlerts) {
            this.alerts.shift();
        }
        
        // Log alert
        this.logAlert(alert);
        
        // Notify UI
        this.notifyUI(alert);
    }
    
    reportLongTask(entry) {
        this.createAlert(
            'longtask',
            `Long task detected: ${entry.duration.toFixed(1)}ms`,
            'warning'
        );
    }
    
    generateAlertId() {
        return Date.now().toString(36) + Math.random().toString(36).substr(2);
    }
    
    logAlert(alert) {
        const logMethod = alert.severity === 'critical' ? 'error' : 'warn';
        console[logMethod](`Performance Alert [${alert.severity.toUpperCase()}]: ${alert.message}`);
    }
    
    notifyUI(alert) {
        // Dispatch custom event for UI to handle
        window.dispatchEvent(new CustomEvent('performance-alert', { detail: alert }));
    }
    
    getAlerts() {
        return [...this.alerts];
    }
    
    clearAlerts() {
        this.alerts.length = 0;
        this.alertCooldown.clear();
    }
}

/**
 * Performance visualization system
 */
class PerformanceVisualizer {
    constructor(performanceSystem) {
        this.performanceSystem = performanceSystem;
        this.charts = new Map();
        this.updateInterval = 100; // Update charts every 100ms
        this.lastUpdate = 0;
    }
    
    createUI() {
        // Create performance monitoring UI
        const existingPanel = document.getElementById('performance-monitor-panel');
        if (existingPanel) {
            existingPanel.remove();
        }
        
        const panel = document.createElement('div');
        panel.id = 'performance-monitor-panel';
        panel.className = 'performance-monitor-panel';
        panel.innerHTML = `
            <div class="performance-header">
                <h3>Performance Monitor</h3>
                <div class="performance-controls">
                    <button class="toggle-btn" data-action="toggle">⏸️</button>
                    <button class="export-btn" data-action="export">💾</button>
                    <button class="clear-btn" data-action="clear">🗑️</button>
                </div>
            </div>
            <div class="performance-metrics">
                <div class="metric-group">
                    <div class="metric-card fps">
                        <div class="metric-label">FPS</div>
                        <div class="metric-value">--</div>
                        <div class="metric-chart" id="fps-chart"></div>
                    </div>
                    <div class="metric-card frametime">
                        <div class="metric-label">Frame Time</div>
                        <div class="metric-value">-- ms</div>
                        <div class="metric-chart" id="frametime-chart"></div>
                    </div>
                </div>
                <div class="metric-group">
                    <div class="metric-card memory">
                        <div class="metric-label">Memory</div>
                        <div class="metric-value">-- MB</div>
                        <div class="metric-chart" id="memory-chart"></div>
                    </div>
                    <div class="metric-card entities">
                        <div class="metric-label">Entities</div>
                        <div class="metric-value">--</div>
                        <div class="metric-chart" id="entities-chart"></div>
                    </div>
                </div>
            </div>
            <div class="performance-status">
                <div class="status-indicator">
                    <div class="status-light"></div>
                    <span class="status-text">Good</span>
                </div>
                <div class="alerts-counter">
                    <span class="alerts-count">0</span> alerts
                </div>
            </div>
            <div class="performance-alerts" id="performance-alerts"></div>
        `;
        
        // Add to UI
        document.body.appendChild(panel);
        
        // Setup event listeners
        this.setupEventListeners(panel);
        
        // Initialize mini charts
        this.initializeCharts();
    }
    
    setupEventListeners(panel) {
        const controls = panel.querySelector('.performance-controls');
        controls.addEventListener('click', (event) => {
            const action = event.target.getAttribute('data-action');
            switch (action) {
                case 'toggle':
                    this.toggleMonitoring();
                    break;
                case 'export':
                    this.exportData();
                    break;
                case 'clear':
                    this.clearData();
                    break;
            }
        });
        
        // Listen for alerts
        window.addEventListener('performance-alert', (event) => {
            this.displayAlert(event.detail);
        });
    }
    
    initializeCharts() {
        // Simple mini chart implementation
        this.charts.set('fps', new MiniChart('fps-chart', { color: '#4CAF50', max: 60 }));
        this.charts.set('frametime', new MiniChart('frametime-chart', { color: '#FF9800', max: 33 }));
        this.charts.set('memory', new MiniChart('memory-chart', { color: '#2196F3', max: 100 }));
        this.charts.set('entities', new MiniChart('entities-chart', { color: '#9C27B0', max: 1000 }));
    }
    
    update(currentState, metrics) {
        const now = performance.now();
        if (now - this.lastUpdate < this.updateInterval) return;
        
        // Update metric displays
        this.updateMetricValues(currentState);
        
        // Update charts
        this.updateCharts(currentState);
        
        // Update status
        this.updateStatus(currentState);
        
        this.lastUpdate = now;
    }
    
    updateMetricValues(currentState) {
        const panel = document.getElementById('performance-monitor-panel');
        if (!panel) return;
        
        // Update metric values
        const updates = {
            '.fps .metric-value': `${currentState.fps}`,
            '.frametime .metric-value': `${currentState.frameTime.toFixed(1)} ms`,
            '.memory .metric-value': `${currentState.memoryUsage.toFixed(1)} MB`,
            '.entities .metric-value': `${currentState.entityCount}`
        };
        
        for (const [selector, value] of Object.entries(updates)) {
            const element = panel.querySelector(selector);
            if (element) {
                element.textContent = value;
            }
        }
    }
    
    updateCharts(currentState) {
        this.charts.get('fps')?.addValue(currentState.fps);
        this.charts.get('frametime')?.addValue(currentState.frameTime);
        this.charts.get('memory')?.addValue(currentState.memoryUsage);
        this.charts.get('entities')?.addValue(currentState.entityCount);
    }
    
    updateStatus(currentState) {
        const panel = document.getElementById('performance-monitor-panel');
        if (!panel) return;
        
        const statusLight = panel.querySelector('.status-light');
        const statusText = panel.querySelector('.status-text');
        
        if (statusLight && statusText) {
            statusLight.className = `status-light ${currentState.status}`;
            statusText.textContent = this.capitalize(currentState.status);
        }
    }
    
    displayAlert(alert) {
        const alertsContainer = document.getElementById('performance-alerts');
        if (!alertsContainer) return;
        
        const alertElement = document.createElement('div');
        alertElement.className = `alert ${alert.severity}`;
        alertElement.innerHTML = `
            <div class="alert-icon">${this.getAlertIcon(alert.severity)}</div>
            <div class="alert-message">${alert.message}</div>
            <div class="alert-time">${new Date(alert.timestamp).toLocaleTimeString()}</div>
            <button class="alert-close">&times;</button>
        `;
        
        alertsContainer.insertBefore(alertElement, alertsContainer.firstChild);
        
        // Auto-remove after 10 seconds
        setTimeout(() => {
            if (alertElement.parentNode) {
                alertElement.remove();
            }
        }, 10000);
        
        // Setup close button
        alertElement.querySelector('.alert-close').addEventListener('click', () => {
            alertElement.remove();
        });
        
        // Update alerts counter
        this.updateAlertsCounter();
    }
    
    getAlertIcon(severity) {
        const icons = {
            warning: '⚠️',
            critical: '🚨',
            info: 'ℹ️'
        };
        return icons[severity] || 'ℹ️';
    }
    
    updateAlertsCounter() {
        const panel = document.getElementById('performance-monitor-panel');
        const alertsContainer = document.getElementById('performance-alerts');
        const counter = panel?.querySelector('.alerts-count');
        
        if (counter && alertsContainer) {
            const alertCount = alertsContainer.querySelectorAll('.alert').length;
            counter.textContent = alertCount.toString();
        }
    }
    
    toggleMonitoring() {
        const enabled = this.performanceSystem.isEnabled;
        this.performanceSystem.setEnabled(!enabled);
        
        const toggleBtn = document.querySelector('.performance-controls .toggle-btn');
        if (toggleBtn) {
            toggleBtn.textContent = enabled ? '▶️' : '⏸️';
        }
    }
    
    exportData() {
        const data = this.performanceSystem.exportPerformanceData();
        const blob = new Blob([data], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        
        const a = document.createElement('a');
        a.href = url;
        a.download = `ecscope-performance-${Date.now()}.json`;
        a.click();
        
        URL.revokeObjectURL(url);
    }
    
    clearData() {
        // Clear metrics
        for (const buffer of Object.values(this.performanceSystem.metrics)) {
            buffer.clear();
        }
        
        // Clear charts
        for (const chart of this.charts.values()) {
            chart.clear();
        }
        
        // Clear alerts
        this.performanceSystem.alertSystem.clearAlerts();
        const alertsContainer = document.getElementById('performance-alerts');
        if (alertsContainer) {
            alertsContainer.innerHTML = '';
        }
        
        this.updateAlertsCounter();
    }
    
    capitalize(str) {
        return str.charAt(0).toUpperCase() + str.slice(1);
    }
    
    cleanup() {
        const panel = document.getElementById('performance-monitor-panel');
        if (panel) {
            panel.remove();
        }
        
        this.charts.clear();
    }
}

/**
 * Simple mini chart for performance metrics
 */
class MiniChart {
    constructor(containerId, options = {}) {
        this.container = document.getElementById(containerId);
        this.options = {
            color: '#4CAF50',
            max: 100,
            width: 100,
            height: 30,
            ...options
        };
        this.values = [];
        this.maxValues = 50;
        
        this.createCanvas();
    }
    
    createCanvas() {
        if (!this.container) return;
        
        this.canvas = document.createElement('canvas');
        this.canvas.width = this.options.width;
        this.canvas.height = this.options.height;
        this.canvas.style.width = '100%';
        this.canvas.style.height = '100%';
        
        this.ctx = this.canvas.getContext('2d');
        this.container.appendChild(this.canvas);
    }
    
    addValue(value) {
        this.values.push(value);
        if (this.values.length > this.maxValues) {
            this.values.shift();
        }
        this.draw();
    }
    
    draw() {
        if (!this.ctx) return;
        
        const { width, height } = this.canvas;
        const { color, max } = this.options;
        
        // Clear canvas
        this.ctx.clearRect(0, 0, width, height);
        
        if (this.values.length < 2) return;
        
        // Draw line
        this.ctx.strokeStyle = color;
        this.ctx.lineWidth = 1;
        this.ctx.beginPath();
        
        const stepX = width / (this.maxValues - 1);
        const startOffset = (this.maxValues - this.values.length) * stepX;
        
        this.values.forEach((value, index) => {
            const x = startOffset + index * stepX;
            const y = height - (value / max) * height;
            
            if (index === 0) {
                this.ctx.moveTo(x, y);
            } else {
                this.ctx.lineTo(x, y);
            }
        });
        
        this.ctx.stroke();
    }
    
    clear() {
        this.values = [];
        if (this.ctx) {
            this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        }
    }
}

// Export for use in main application
if (typeof window !== 'undefined') {
    window.WebPerformanceSystem = WebPerformanceSystem;
    window.ECSProfiler = ECSProfiler;
    window.PerformanceVisualizer = PerformanceVisualizer;
}