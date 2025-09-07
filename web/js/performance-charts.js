// =============================================================================
// ECScope Performance Charts and Monitoring
// =============================================================================

class PerformanceCharts {
    constructor() {
        this.frameTimeChart = null;
        this.memoryChart = null;
        this.frameTimeData = [];
        this.memoryData = [];
        this.maxDataPoints = 300; // 5 minutes at 60 FPS
        
        this.colors = {
            primary: '#0066cc',
            success: '#28a745',
            warning: '#ffc107',
            danger: '#dc3545',
            info: '#17a2b8'
        };
    }

    initialize() {
        this.initializeFrameTimeChart();
        this.initializeMemoryChart();
    }

    initializeFrameTimeChart() {
        const canvas = document.getElementById('frame-time-chart');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        
        // Simple line chart implementation
        this.frameTimeChart = {
            canvas: canvas,
            ctx: ctx,
            data: [],
            maxPoints: this.maxDataPoints,
            targetFrameTime: 16.67, // 60 FPS target
            
            draw: function() {
                const width = this.canvas.width;
                const height = this.canvas.height;
                
                // Clear canvas
                this.ctx.clearRect(0, 0, width, height);
                
                if (this.data.length < 2) return;
                
                // Find min/max for scaling
                const maxTime = Math.max(...this.data, this.targetFrameTime * 2);
                const minTime = 0;
                
                // Draw background
                this.ctx.fillStyle = '#f8f9fa';
                this.ctx.fillRect(0, 0, width, height);
                
                // Draw target line
                const targetY = height - (this.targetFrameTime / maxTime) * height;
                this.ctx.strokeStyle = '#28a745';
                this.ctx.setLineDash([5, 5]);
                this.ctx.beginPath();
                this.ctx.moveTo(0, targetY);
                this.ctx.lineTo(width, targetY);
                this.ctx.stroke();
                this.ctx.setLineDash([]);
                
                // Draw frame time line
                this.ctx.strokeStyle = '#0066cc';
                this.ctx.lineWidth = 2;
                this.ctx.beginPath();
                
                for (let i = 0; i < this.data.length; i++) {
                    const x = (i / (this.data.length - 1)) * width;
                    const y = height - (this.data[i] / maxTime) * height;
                    
                    if (i === 0) {
                        this.ctx.moveTo(x, y);
                    } else {
                        this.ctx.lineTo(x, y);
                    }
                }
                
                this.ctx.stroke();
                
                // Draw labels
                this.ctx.fillStyle = '#6c757d';
                this.ctx.font = '12px Arial';
                this.ctx.fillText(`Target: ${this.targetFrameTime.toFixed(1)}ms`, 10, 20);
                if (this.data.length > 0) {
                    const current = this.data[this.data.length - 1];
                    this.ctx.fillText(`Current: ${current.toFixed(1)}ms`, 10, 40);
                }
                this.ctx.fillText(`Max: ${maxTime.toFixed(1)}ms`, 10, height - 10);
            },
            
            addData: function(frameTime) {
                this.data.push(frameTime);
                if (this.data.length > this.maxPoints) {
                    this.data.shift();
                }
                this.draw();
            }
        };
    }

    initializeMemoryChart() {
        const canvas = document.getElementById('memory-chart');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        
        this.memoryChart = {
            canvas: canvas,
            ctx: ctx,
            data: [],
            maxPoints: this.maxDataPoints,
            
            draw: function() {
                const width = this.canvas.width;
                const height = this.canvas.height;
                
                // Clear canvas
                this.ctx.clearRect(0, 0, width, height);
                
                if (this.data.length < 2) return;
                
                // Find min/max for scaling
                const maxMemory = Math.max(...this.data, 1024); // At least 1MB scale
                const minMemory = 0;
                
                // Draw background
                this.ctx.fillStyle = '#f8f9fa';
                this.ctx.fillRect(0, 0, width, height);
                
                // Draw memory usage line
                this.ctx.strokeStyle = '#17a2b8';
                this.ctx.lineWidth = 2;
                this.ctx.beginPath();
                
                for (let i = 0; i < this.data.length; i++) {
                    const x = (i / (this.data.length - 1)) * width;
                    const y = height - (this.data[i] / maxMemory) * height;
                    
                    if (i === 0) {
                        this.ctx.moveTo(x, y);
                    } else {
                        this.ctx.lineTo(x, y);
                    }
                }
                
                this.ctx.stroke();
                
                // Fill area under the line
                this.ctx.fillStyle = 'rgba(23, 162, 184, 0.2)';
                this.ctx.beginPath();
                this.ctx.moveTo(0, height);
                
                for (let i = 0; i < this.data.length; i++) {
                    const x = (i / (this.data.length - 1)) * width;
                    const y = height - (this.data[i] / maxMemory) * height;
                    this.ctx.lineTo(x, y);
                }
                
                this.ctx.lineTo(width, height);
                this.ctx.fill();
                
                // Draw labels
                this.ctx.fillStyle = '#6c757d';
                this.ctx.font = '12px Arial';
                if (this.data.length > 0) {
                    const current = this.data[this.data.length - 1];
                    this.ctx.fillText(`Current: ${(current / 1024).toFixed(1)} KB`, 10, 20);
                }
                this.ctx.fillText(`Max: ${(maxMemory / 1024).toFixed(1)} KB`, 10, height - 10);
            },
            
            addData: function(memoryUsage) {
                this.data.push(memoryUsage);
                if (this.data.length > this.maxPoints) {
                    this.data.shift();
                }
                this.draw();
            }
        };
    }

    updateFrameTime(frameTime) {
        if (this.frameTimeChart) {
            this.frameTimeChart.addData(frameTime);
        }
        
        this.frameTimeData.push(frameTime);
        if (this.frameTimeData.length > this.maxDataPoints) {
            this.frameTimeData.shift();
        }
    }

    updateMemoryUsage(memoryUsage) {
        if (this.memoryChart) {
            this.memoryChart.addData(memoryUsage);
        }
        
        this.memoryData.push(memoryUsage);
        if (this.memoryData.length > this.maxDataPoints) {
            this.memoryData.shift();
        }
    }

    getStatistics() {
        return {
            frameTime: this.calculateFrameTimeStats(),
            memory: this.calculateMemoryStats()
        };
    }

    calculateFrameTimeStats() {
        if (this.frameTimeData.length === 0) {
            return {
                average: 0,
                min: 0,
                max: 0,
                p95: 0,
                p99: 0,
                fps: 0
            };
        }
        
        const sorted = [...this.frameTimeData].sort((a, b) => a - b);
        const sum = sorted.reduce((acc, val) => acc + val, 0);
        
        const p95Index = Math.floor(sorted.length * 0.95);
        const p99Index = Math.floor(sorted.length * 0.99);
        
        const average = sum / sorted.length;
        
        return {
            average: average,
            min: sorted[0],
            max: sorted[sorted.length - 1],
            p95: sorted[p95Index],
            p99: sorted[p99Index],
            fps: average > 0 ? 1000 / average : 0
        };
    }

    calculateMemoryStats() {
        if (this.memoryData.length === 0) {
            return {
                current: 0,
                average: 0,
                min: 0,
                max: 0
            };
        }
        
        const sorted = [...this.memoryData].sort((a, b) => a - b);
        const sum = sorted.reduce((acc, val) => acc + val, 0);
        
        return {
            current: this.memoryData[this.memoryData.length - 1],
            average: sum / sorted.length,
            min: sorted[0],
            max: sorted[sorted.length - 1]
        };
    }

    reset() {
        this.frameTimeData = [];
        this.memoryData = [];
        
        if (this.frameTimeChart) {
            this.frameTimeChart.data = [];
            this.frameTimeChart.draw();
        }
        
        if (this.memoryChart) {
            this.memoryChart.data = [];
            this.memoryChart.draw();
        }
    }

    // Performance budget monitoring
    checkPerformanceBudget() {
        const stats = this.calculateFrameTimeStats();
        const targetFrameTime = 16.67; // 60 FPS
        const budgetThreshold = targetFrameTime * 1.2; // 20% tolerance
        
        return {
            withinBudget: stats.average <= budgetThreshold,
            averageFrameTime: stats.average,
            targetFrameTime: targetFrameTime,
            budgetUtilization: stats.average / targetFrameTime,
            droppedFrames: this.frameTimeData.filter(time => time > budgetThreshold).length,
            totalFrames: this.frameTimeData.length
        };
    }

    // Export data for analysis
    exportData() {
        return {
            frameTime: [...this.frameTimeData],
            memory: [...this.memoryData],
            timestamp: Date.now(),
            metadata: {
                maxDataPoints: this.maxDataPoints,
                dataPointCount: this.frameTimeData.length
            }
        };
    }

    // Import data (useful for loading saved sessions)
    importData(data) {
        if (data.frameTime) {
            this.frameTimeData = data.frameTime.slice(-this.maxDataPoints);
            if (this.frameTimeChart) {
                this.frameTimeChart.data = [...this.frameTimeData];
                this.frameTimeChart.draw();
            }
        }
        
        if (data.memory) {
            this.memoryData = data.memory.slice(-this.maxDataPoints);
            if (this.memoryChart) {
                this.memoryChart.data = [...this.memoryData];
                this.memoryChart.draw();
            }
        }
    }
}

// =============================================================================
// PERFORMANCE MONITOR CLASS
// =============================================================================

class PerformanceMonitor {
    constructor() {
        this.charts = new PerformanceCharts();
        this.updateInterval = null;
        this.isMonitoring = false;
        
        this.metrics = {
            fps: 0,
            frameTime: 0,
            memoryUsage: 0,
            entityCount: 0,
            systemTime: 0,
            queryTime: 0
        };
        
        this.callbacks = {
            onUpdate: null,
            onBudgetExceeded: null,
            onMemoryPressure: null
        };
    }

    initialize() {
        this.charts.initialize();
        
        // Update UI elements
        this.updateUIElements();
        
        console.log('Performance monitor initialized');
    }

    start(updateIntervalMs = 100) {
        if (this.isMonitoring) return;
        
        this.isMonitoring = true;
        
        this.updateInterval = setInterval(() => {
            this.update();
        }, updateIntervalMs);
        
        console.log('Performance monitoring started');
    }

    stop() {
        if (!this.isMonitoring) return;
        
        this.isMonitoring = false;
        
        if (this.updateInterval) {
            clearInterval(this.updateInterval);
            this.updateInterval = null;
        }
        
        console.log('Performance monitoring stopped');
    }

    update() {
        if (!window.ECScopeLoader || !window.ECScopeLoader.isInitialized) {
            return;
        }
        
        try {
            // Get performance stats from WebAssembly
            const stats = window.ECScopeLoader.getPerformanceStats();
            
            // Update metrics
            this.metrics.fps = stats.fps;
            this.metrics.frameTime = stats.frameTime;
            this.metrics.memoryUsage = stats.memoryUsage;
            this.metrics.entityCount = window.ECScopeLoader.getEntityCount();
            
            // Update charts
            this.charts.updateFrameTime(this.metrics.frameTime);
            this.charts.updateMemoryUsage(this.metrics.memoryUsage);
            
            // Update UI
            this.updateUIElements();
            
            // Check performance budget
            this.checkPerformanceBudget();
            
            // Check memory pressure
            this.checkMemoryPressure();
            
            // Call update callback
            if (this.callbacks.onUpdate) {
                this.callbacks.onUpdate(this.metrics);
            }
            
        } catch (error) {
            console.error('Performance monitor update error:', error);
        }
    }

    updateUIElements() {
        // Update performance metrics in UI
        const elements = {
            'avg-fps': this.metrics.fps.toFixed(0),
            'frame-time-p95': this.charts.calculateFrameTimeStats().p95?.toFixed(1) + 'ms' || '0.0ms',
            'frame-time-p99': this.charts.calculateFrameTimeStats().p99?.toFixed(1) + 'ms' || '0.0ms',
            'total-allocated': (this.metrics.memoryUsage / 1024).toFixed(0) + ' KB',
            'peak-usage': (this.charts.calculateMemoryStats().max / 1024).toFixed(0) + ' KB',
            'allocation-count-metric': this.metrics.entityCount,
            'query-time': this.metrics.queryTime.toFixed(1) + 'ms',
            'system-time': this.metrics.systemTime.toFixed(1) + 'ms',
            'physics-time': '0.0ms' // Would be updated by physics system
        };
        
        Object.entries(elements).forEach(([id, value]) => {
            const element = document.getElementById(id);
            if (element) {
                element.textContent = value;
            }
        });
    }

    checkPerformanceBudget() {
        const budget = this.charts.checkPerformanceBudget();
        
        if (!budget.withinBudget && this.callbacks.onBudgetExceeded) {
            this.callbacks.onBudgetExceeded(budget);
        }
    }

    checkMemoryPressure() {
        const memoryStats = this.charts.calculateMemoryStats();
        const memoryPressureThreshold = 100 * 1024 * 1024; // 100MB threshold
        
        if (memoryStats.current > memoryPressureThreshold && this.callbacks.onMemoryPressure) {
            this.callbacks.onMemoryPressure({
                current: memoryStats.current,
                threshold: memoryPressureThreshold,
                utilizationPercent: (memoryStats.current / memoryPressureThreshold) * 100
            });
        }
    }

    reset() {
        this.charts.reset();
        
        this.metrics = {
            fps: 0,
            frameTime: 0,
            memoryUsage: 0,
            entityCount: 0,
            systemTime: 0,
            queryTime: 0
        };
        
        this.updateUIElements();
        
        console.log('Performance monitor reset');
    }

    exportData() {
        return {
            charts: this.charts.exportData(),
            metrics: { ...this.metrics },
            timestamp: Date.now()
        };
    }

    importData(data) {
        if (data.charts) {
            this.charts.importData(data.charts);
        }
        
        if (data.metrics) {
            this.metrics = { ...this.metrics, ...data.metrics };
            this.updateUIElements();
        }
    }

    // Event handlers
    onUpdate(callback) {
        this.callbacks.onUpdate = callback;
    }

    onBudgetExceeded(callback) {
        this.callbacks.onBudgetExceeded = callback;
    }

    onMemoryPressure(callback) {
        this.callbacks.onMemoryPressure = callback;
    }
}

// Global performance monitor instance
window.performanceMonitor = new PerformanceMonitor();

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.performanceMonitor.initialize();
});