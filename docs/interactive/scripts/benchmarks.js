/**
 * ECScope Interactive Documentation - Performance Benchmarking Interface
 * Real-time performance analysis and visualization
 */

class ECScope_BenchmarkSystem {
    constructor() {
        this.isRunning = false;
        this.benchmarkResults = {};
        this.charts = {};
        this.realTimeData = {};
        this.websocket = null;
        
        this.initialize();
    }

    initialize() {
        this.setupBenchmarkInterface();
        this.initializeCharts();
        this.setupRealTimeConnection();
    }

    setupBenchmarkInterface() {
        // Add benchmark controls to performance section
        const performanceSection = document.getElementById('benchmarks');
        if (performanceSection && !performanceSection.dataset.initialized) {
            performanceSection.innerHTML = this.generateBenchmarkInterface();
            performanceSection.dataset.initialized = 'true';
            this.setupBenchmarkEventListeners();
        }
    }

    generateBenchmarkInterface() {
        return `
            <div class="benchmark-suite">
                <div class="benchmark-header">
                    <h1 class="section-title">Performance Laboratory</h1>
                    <div class="benchmark-controls">
                        <button id="start-benchmark" class="button primary">
                            <i class="fas fa-play"></i> Run Benchmarks
                        </button>
                        <button id="stop-benchmark" class="button error" disabled>
                            <i class="fas fa-stop"></i> Stop
                        </button>
                        <button id="export-results" class="button">
                            <i class="fas fa-download"></i> Export Results
                        </button>
                        <button id="compare-results" class="button">
                            <i class="fas fa-balance-scale"></i> Compare
                        </button>
                    </div>
                </div>

                <div class="benchmark-configuration">
                    <div class="config-section">
                        <h3>Test Configuration</h3>
                        <div class="config-grid">
                            <div class="config-group">
                                <label>Entity Counts:</label>
                                <div class="checkbox-group">
                                    <label><input type="checkbox" value="100" checked> 100</label>
                                    <label><input type="checkbox" value="1000" checked> 1K</label>
                                    <label><input type="checkbox" value="10000" checked> 10K</label>
                                    <label><input type="checkbox" value="50000"> 50K</label>
                                    <label><input type="checkbox" value="100000"> 100K</label>
                                </div>
                            </div>
                            
                            <div class="config-group">
                                <label>Architecture Types:</label>
                                <div class="checkbox-group">
                                    <label><input type="checkbox" value="archetype_soa" checked> Archetype (SoA)</label>
                                    <label><input type="checkbox" value="sparse_set" checked> Sparse Set</label>
                                    <label><input type="checkbox" value="component_array"> Component Arrays</label>
                                </div>
                            </div>
                            
                            <div class="config-group">
                                <label>Test Categories:</label>
                                <div class="checkbox-group">
                                    <label><input type="checkbox" value="entity_lifecycle" checked> Entity Lifecycle</label>
                                    <label><input type="checkbox" value="query_iteration" checked> Query Iteration</label>
                                    <label><input type="checkbox" value="memory_access" checked> Memory Access</label>
                                    <label><input type="checkbox" value="system_update" checked> System Updates</label>
                                </div>
                            </div>
                            
                            <div class="config-group">
                                <label>Iterations:</label>
                                <input type="number" id="benchmark-iterations" value="10" min="1" max="100">
                            </div>
                        </div>
                    </div>
                </div>

                <div class="benchmark-progress" style="display: none;">
                    <div class="progress-info">
                        <h3>Running Benchmarks...</h3>
                        <div id="current-test-info">Preparing tests...</div>
                    </div>
                    <div class="progress-bar">
                        <div class="progress-fill" id="benchmark-progress-fill"></div>
                    </div>
                    <div class="progress-text" id="benchmark-progress-text">0%</div>
                </div>

                <div class="benchmark-results">
                    <div class="results-summary" id="results-summary">
                        <div class="summary-placeholder">
                            <i class="fas fa-chart-line"></i>
                            <p>Run benchmarks to see performance analysis</p>
                        </div>
                    </div>
                    
                    <div class="results-charts">
                        <div class="chart-container">
                            <div class="chart-header">
                                <h3>Performance Comparison</h3>
                                <div class="chart-controls">
                                    <select id="chart-metric">
                                        <option value="time">Execution Time</option>
                                        <option value="throughput">Throughput</option>
                                        <option value="memory">Memory Usage</option>
                                        <option value="cache">Cache Performance</option>
                                    </select>
                                    <select id="chart-scale">
                                        <option value="linear">Linear Scale</option>
                                        <option value="log">Logarithmic Scale</option>
                                    </select>
                                </div>
                            </div>
                            <div id="performance-chart" class="chart-display"></div>
                        </div>
                        
                        <div class="chart-container">
                            <div class="chart-header">
                                <h3>Scaling Analysis</h3>
                            </div>
                            <div id="scaling-chart" class="chart-display"></div>
                        </div>
                    </div>
                    
                    <div class="results-details">
                        <div class="detail-tabs">
                            <button class="tab-button active" data-tab="detailed-results">Detailed Results</button>
                            <button class="tab-button" data-tab="memory-analysis">Memory Analysis</button>
                            <button class="tab-button" data-tab="recommendations">Recommendations</button>
                        </div>
                        
                        <div class="tab-content active" id="detailed-results">
                            <div id="detailed-results-table"></div>
                        </div>
                        
                        <div class="tab-content" id="memory-analysis">
                            <div id="memory-analysis-content"></div>
                        </div>
                        
                        <div class="tab-content" id="recommendations">
                            <div id="recommendations-content"></div>
                        </div>
                    </div>
                </div>

                <div class="real-time-monitoring">
                    <h3>Real-Time Performance Monitoring</h3>
                    <div class="monitoring-controls">
                        <button id="start-monitoring" class="button">
                            <i class="fas fa-play"></i> Start Monitoring
                        </button>
                        <button id="stop-monitoring" class="button" disabled>
                            <i class="fas fa-pause"></i> Stop Monitoring
                        </button>
                        <div class="monitoring-options">
                            <label>Update Rate:</label>
                            <select id="update-rate">
                                <option value="100">100ms</option>
                                <option value="500" selected>500ms</option>
                                <option value="1000">1s</option>
                            </select>
                        </div>
                    </div>
                    
                    <div class="live-metrics-grid">
                        <div class="metric-card">
                            <div class="metric-header">
                                <h4>Entities/Second</h4>
                                <i class="fas fa-cubes"></i>
                            </div>
                            <div class="metric-value" id="entities-per-second">0</div>
                            <div class="metric-trend" id="entities-trend"></div>
                        </div>
                        
                        <div class="metric-card">
                            <div class="metric-header">
                                <h4>Memory Usage</h4>
                                <i class="fas fa-memory"></i>
                            </div>
                            <div class="metric-value" id="memory-usage">0 MB</div>
                            <div class="metric-trend" id="memory-trend"></div>
                        </div>
                        
                        <div class="metric-card">
                            <div class="metric-header">
                                <h4>Cache Hit Rate</h4>
                                <i class="fas fa-tachometer-alt"></i>
                            </div>
                            <div class="metric-value" id="cache-hit-rate">0%</div>
                            <div class="metric-trend" id="cache-trend"></div>
                        </div>
                        
                        <div class="metric-card">
                            <div class="metric-header">
                                <h4>Frame Time</h4>
                                <i class="fas fa-clock"></i>
                            </div>
                            <div class="metric-value" id="frame-time">0.0ms</div>
                            <div class="metric-trend" id="frame-trend"></div>
                        </div>
                    </div>
                    
                    <div class="real-time-chart">
                        <div id="real-time-performance-chart" class="chart-display"></div>
                    </div>
                </div>

                <div class="benchmark-history">
                    <h3>Benchmark History</h3>
                    <div class="history-controls">
                        <button class="button" onclick="this.parentElement.nextElementSibling.style.display = this.parentElement.nextElementSibling.style.display === 'none' ? 'block' : 'none'">
                            <i class="fas fa-history"></i> View History
                        </button>
                        <button class="button" id="clear-history">
                            <i class="fas fa-trash"></i> Clear History
                        </button>
                    </div>
                    <div class="history-list" style="display: none;" id="benchmark-history-list">
                        <div class="history-placeholder">No benchmark history available</div>
                    </div>
                </div>
            </div>
        `;
    }

    setupBenchmarkEventListeners() {
        // Benchmark control buttons
        document.getElementById('start-benchmark')?.addEventListener('click', () => {
            this.startBenchmarkSuite();
        });

        document.getElementById('stop-benchmark')?.addEventListener('click', () => {
            this.stopBenchmarkSuite();
        });

        document.getElementById('export-results')?.addEventListener('click', () => {
            this.exportResults();
        });

        document.getElementById('compare-results')?.addEventListener('click', () => {
            this.showComparisonDialog();
        });

        // Real-time monitoring
        document.getElementById('start-monitoring')?.addEventListener('click', () => {
            this.startRealTimeMonitoring();
        });

        document.getElementById('stop-monitoring')?.addEventListener('click', () => {
            this.stopRealTimeMonitoring();
        });

        // Chart controls
        document.getElementById('chart-metric')?.addEventListener('change', (e) => {
            this.updateChart('performance-chart', e.target.value);
        });

        document.getElementById('chart-scale')?.addEventListener('change', (e) => {
            this.updateChartScale('performance-chart', e.target.value);
        });

        // History
        document.getElementById('clear-history')?.addEventListener('click', () => {
            this.clearBenchmarkHistory();
        });

        // Tab switching
        document.addEventListener('click', (e) => {
            if (e.target.matches('.tab-button')) {
                this.switchResultTab(e.target);
            }
        });
    }

    async startBenchmarkSuite() {
        if (this.isRunning) return;

        this.isRunning = true;
        this.updateBenchmarkControls(true);
        this.showProgress(true);

        const config = this.getBenchmarkConfiguration();
        const totalTests = this.calculateTotalTests(config);
        let completedTests = 0;

        try {
            this.benchmarkResults = {};

            for (const entityCount of config.entityCounts) {
                for (const architecture of config.architectures) {
                    for (const testCategory of config.testCategories) {
                        if (!this.isRunning) break;

                        await this.runSingleBenchmark(entityCount, architecture, testCategory, config.iterations);
                        
                        completedTests++;
                        const progress = (completedTests / totalTests) * 100;
                        this.updateProgress(progress, `Testing ${testCategory} with ${entityCount} entities (${architecture})`);
                    }
                }
            }

            this.finalizeBenchmarkResults();
            this.updateResultsDisplay();

        } catch (error) {
            console.error('Benchmark failed:', error);
            this.showBenchmarkError(error);
        } finally {
            this.isRunning = false;
            this.updateBenchmarkControls(false);
            this.showProgress(false);
        }
    }

    async runSingleBenchmark(entityCount, architecture, testCategory, iterations) {
        // Simulate running actual C++ benchmark
        const delay = 100 + Math.random() * 500; // Simulate variable execution time
        await new Promise(resolve => setTimeout(resolve, delay));

        const result = this.simulateBenchmarkResult(entityCount, architecture, testCategory, iterations);
        
        const key = `${architecture}-${testCategory}-${entityCount}`;
        if (!this.benchmarkResults[key]) {
            this.benchmarkResults[key] = [];
        }
        this.benchmarkResults[key].push(result);
    }

    simulateBenchmarkResult(entityCount, architecture, testCategory, iterations) {
        // Generate realistic benchmark data based on actual ECScope performance characteristics
        const basePerformance = {
            'archetype_soa': { multiplier: 1.0, memoryEfficiency: 0.95 },
            'sparse_set': { multiplier: 1.2, memoryEfficiency: 0.85 },
            'component_array': { multiplier: 1.8, memoryEfficiency: 0.70 }
        };

        const testComplexity = {
            'entity_lifecycle': 1.0,
            'query_iteration': 0.8,
            'memory_access': 1.2,
            'system_update': 1.5
        };

        const arch = basePerformance[architecture];
        const complexity = testComplexity[testCategory];
        
        // Calculate execution time (microseconds)
        const baseTime = Math.log2(entityCount) * complexity * arch.multiplier * (50 + Math.random() * 20);
        const variance = baseTime * (0.05 + Math.random() * 0.1);
        
        const times = Array.from({ length: iterations }, () => 
            Math.max(1, baseTime + (Math.random() - 0.5) * variance)
        );

        const avgTime = times.reduce((a, b) => a + b, 0) / times.length;
        const minTime = Math.min(...times);
        const maxTime = Math.max(...times);
        
        // Calculate statistics
        const median = times.sort((a, b) => a - b)[Math.floor(times.length / 2)];
        const variance_calc = times.reduce((sum, time) => sum + Math.pow(time - avgTime, 2), 0) / times.length;
        const stdDev = Math.sqrt(variance_calc);

        return {
            testName: testCategory,
            architecture: architecture,
            entityCount: entityCount,
            averageTime: avgTime,
            minTime: minTime,
            maxTime: maxTime,
            medianTime: median,
            standardDeviation: stdDev,
            rawTimings: times,
            
            // Derived metrics
            entitiesPerSecond: (entityCount * 1000000) / avgTime,
            memoryUsage: entityCount * (architecture === 'archetype_soa' ? 32 : 48) + Math.random() * 1000,
            memoryEfficiency: arch.memoryEfficiency + Math.random() * 0.05,
            cacheHitRate: Math.max(0.7, arch.memoryEfficiency - 0.1 + Math.random() * 0.2),
            
            // Metadata
            timestamp: new Date().toISOString(),
            iterations: iterations
        };
    }

    getBenchmarkConfiguration() {
        const entityCounts = Array.from(document.querySelectorAll('input[type="checkbox"][value]'))
            .filter(cb => cb.checked && !isNaN(parseInt(cb.value)))
            .map(cb => parseInt(cb.value));

        const architectures = Array.from(document.querySelectorAll('input[value*="_"]'))
            .filter(cb => cb.checked)
            .map(cb => cb.value);

        const testCategories = Array.from(document.querySelectorAll('input[value*="lifecycle"], input[value*="iteration"], input[value*="access"], input[value*="update"]'))
            .filter(cb => cb.checked)
            .map(cb => cb.value);

        const iterations = parseInt(document.getElementById('benchmark-iterations')?.value) || 10;

        return {
            entityCounts,
            architectures,
            testCategories,
            iterations
        };
    }

    calculateTotalTests(config) {
        return config.entityCounts.length * config.architectures.length * config.testCategories.length;
    }

    updateBenchmarkControls(running) {
        const startBtn = document.getElementById('start-benchmark');
        const stopBtn = document.getElementById('stop-benchmark');
        
        if (startBtn) startBtn.disabled = running;
        if (stopBtn) stopBtn.disabled = !running;
    }

    showProgress(show) {
        const progressDiv = document.querySelector('.benchmark-progress');
        if (progressDiv) {
            progressDiv.style.display = show ? 'block' : 'none';
        }
    }

    updateProgress(percentage, currentTest) {
        const fillEl = document.getElementById('benchmark-progress-fill');
        const textEl = document.getElementById('benchmark-progress-text');
        const infoEl = document.getElementById('current-test-info');

        if (fillEl) fillEl.style.width = `${percentage}%`;
        if (textEl) textEl.textContent = `${Math.round(percentage)}%`;
        if (infoEl) infoEl.textContent = currentTest;
    }

    finalizeBenchmarkResults() {
        // Calculate summary statistics
        this.calculatePerformanceGrades();
        this.generateRecommendations();
        this.saveToHistory();
    }

    updateResultsDisplay() {
        this.updateResultsSummary();
        this.updatePerformanceChart();
        this.updateScalingChart();
        this.updateDetailedResultsTable();
        this.updateMemoryAnalysis();
        this.updateRecommendations();
    }

    updateResultsSummary() {
        const summaryEl = document.getElementById('results-summary');
        if (!summaryEl) return;

        const bestResult = this.findBestResult();
        const worstResult = this.findWorstResult();
        
        summaryEl.innerHTML = `
            <div class="summary-grid">
                <div class="summary-card best">
                    <h4>Best Performance</h4>
                    <div class="result-info">
                        <div class="architecture">${bestResult.architecture.replace('_', ' ').toUpperCase()}</div>
                        <div class="metric">${bestResult.entitiesPerSecond.toFixed(0)} entities/sec</div>
                        <div class="test-name">${bestResult.testName.replace('_', ' ')}</div>
                    </div>
                </div>
                
                <div class="summary-card average">
                    <h4>Average Performance</h4>
                    <div class="result-info">
                        <div class="metric">${this.calculateAveragePerformance().toFixed(0)} entities/sec</div>
                        <div class="description">Across all tests</div>
                    </div>
                </div>
                
                <div class="summary-card improvement">
                    <h4>Potential Improvement</h4>
                    <div class="result-info">
                        <div class="metric">${((bestResult.entitiesPerSecond / worstResult.entitiesPerSecond) * 100 - 100).toFixed(0)}%</div>
                        <div class="description">Best vs Worst case</div>
                    </div>
                </div>
                
                <div class="summary-card memory">
                    <h4>Memory Efficiency</h4>
                    <div class="result-info">
                        <div class="metric">${(this.calculateAverageMemoryEfficiency() * 100).toFixed(1)}%</div>
                        <div class="description">Average across tests</div>
                    </div>
                </div>
            </div>
        `;
    }

    initializeCharts() {
        // Initialize Plotly charts
        this.charts['performance-chart'] = null;
        this.charts['scaling-chart'] = null;
        this.charts['real-time-chart'] = null;
    }

    updatePerformanceChart() {
        const chartEl = document.getElementById('performance-chart');
        if (!chartEl) return;

        const data = this.prepareChartData('performance');
        
        const layout = {
            title: 'Performance Comparison',
            xaxis: { title: 'Entity Count' },
            yaxis: { title: 'Entities per Second' },
            margin: { t: 50, b: 50, l: 80, r: 50 },
            paper_bgcolor: 'transparent',
            plot_bgcolor: 'transparent',
            font: { color: getComputedStyle(document.documentElement).getPropertyValue('--text-primary') }
        };

        if (typeof Plotly !== 'undefined') {
            Plotly.newPlot(chartEl, data, layout, { responsive: true });
        } else {
            chartEl.innerHTML = '<div class="chart-placeholder">Charts require Plotly.js to be loaded</div>';
        }
    }

    prepareChartData(chartType) {
        const traces = [];
        const architectures = [...new Set(Object.values(this.benchmarkResults).flat().map(r => r.architecture))];
        
        architectures.forEach(arch => {
            const archResults = Object.values(this.benchmarkResults)
                .flat()
                .filter(r => r.architecture === arch)
                .sort((a, b) => a.entityCount - b.entityCount);

            traces.push({
                x: archResults.map(r => r.entityCount),
                y: archResults.map(r => r.entitiesPerSecond),
                mode: 'lines+markers',
                name: arch.replace('_', ' ').toUpperCase(),
                line: { width: 3 },
                marker: { size: 8 }
            });
        });

        return traces;
    }

    // Real-time monitoring
    startRealTimeMonitoring() {
        if (this.monitoringInterval) return;

        const updateRate = parseInt(document.getElementById('update-rate')?.value) || 500;
        
        this.monitoringInterval = setInterval(() => {
            this.updateLiveMetrics();
        }, updateRate);

        document.getElementById('start-monitoring').disabled = true;
        document.getElementById('stop-monitoring').disabled = false;
    }

    stopRealTimeMonitoring() {
        if (this.monitoringInterval) {
            clearInterval(this.monitoringInterval);
            this.monitoringInterval = null;
        }

        document.getElementById('start-monitoring').disabled = false;
        document.getElementById('stop-monitoring').disabled = true;
    }

    updateLiveMetrics() {
        // Simulate real-time data from ECScope
        const metrics = this.generateLiveMetrics();
        
        document.getElementById('entities-per-second').textContent = metrics.entitiesPerSecond.toFixed(0);
        document.getElementById('memory-usage').textContent = `${metrics.memoryUsage.toFixed(1)} MB`;
        document.getElementById('cache-hit-rate').textContent = `${(metrics.cacheHitRate * 100).toFixed(1)}%`;
        document.getElementById('frame-time').textContent = `${metrics.frameTime.toFixed(2)}ms`;

        this.updateTrendIndicators(metrics);
        this.updateRealTimeChart(metrics);
    }

    generateLiveMetrics() {
        const now = Date.now();
        const baseMetrics = {
            entitiesPerSecond: 15000 + Math.sin(now / 10000) * 5000 + Math.random() * 1000,
            memoryUsage: 45.5 + Math.sin(now / 15000) * 10 + Math.random() * 2,
            cacheHitRate: 0.85 + Math.sin(now / 8000) * 0.1 + Math.random() * 0.05,
            frameTime: 16.67 + Math.sin(now / 5000) * 5 + Math.random() * 2
        };

        return baseMetrics;
    }

    updateTrendIndicators(metrics) {
        // Update trend arrows based on recent history
        // Implementation would track history and show ↑↓ indicators
    }

    // Utility methods
    findBestResult() {
        const allResults = Object.values(this.benchmarkResults).flat();
        return allResults.reduce((best, current) => 
            current.entitiesPerSecond > best.entitiesPerSecond ? current : best
        );
    }

    findWorstResult() {
        const allResults = Object.values(this.benchmarkResults).flat();
        return allResults.reduce((worst, current) => 
            current.entitiesPerSecond < worst.entitiesPerSecond ? current : worst
        );
    }

    calculateAveragePerformance() {
        const allResults = Object.values(this.benchmarkResults).flat();
        return allResults.reduce((sum, r) => sum + r.entitiesPerSecond, 0) / allResults.length;
    }

    calculateAverageMemoryEfficiency() {
        const allResults = Object.values(this.benchmarkResults).flat();
        return allResults.reduce((sum, r) => sum + r.memoryEfficiency, 0) / allResults.length;
    }

    exportResults() {
        const data = {
            timestamp: new Date().toISOString(),
            results: this.benchmarkResults,
            summary: {
                totalTests: Object.keys(this.benchmarkResults).length,
                bestPerformance: this.findBestResult(),
                averagePerformance: this.calculateAveragePerformance()
            }
        };

        const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        
        const a = document.createElement('a');
        a.href = url;
        a.download = `ecscope-benchmark-${Date.now()}.json`;
        a.click();
        
        URL.revokeObjectURL(url);
    }

    clearBenchmarkHistory() {
        localStorage.removeItem('ecscope-benchmark-history');
        document.getElementById('benchmark-history-list').innerHTML = 
            '<div class="history-placeholder">No benchmark history available</div>';
    }

    saveToHistory() {
        const history = JSON.parse(localStorage.getItem('ecscope-benchmark-history') || '[]');
        history.unshift({
            timestamp: new Date().toISOString(),
            results: this.benchmarkResults,
            summary: {
                totalTests: Object.keys(this.benchmarkResults).length,
                bestPerformance: this.findBestResult()?.entitiesPerSecond || 0
            }
        });
        
        // Keep only last 10 runs
        history.splice(10);
        
        localStorage.setItem('ecscope-benchmark-history', JSON.stringify(history));
    }

    stopBenchmarkSuite() {
        this.isRunning = false;
    }

    showBenchmarkError(error) {
        console.error('Benchmark error:', error);
        // Show error UI
    }
}

// Initialize benchmark system when document is ready
document.addEventListener('DOMContentLoaded', () => {
    if (window.ecscopeDoc) {
        window.ecscopeDoc.benchmarkSystem = new ECScope_BenchmarkSystem();
    }
});

// Export for module systems
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ECScope_BenchmarkSystem;
}