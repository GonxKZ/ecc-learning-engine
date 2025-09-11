// ECScope WebAssembly Module Post-JS
// This file is loaded after the WebAssembly module is initialized

(function() {
    'use strict';
    
    if (!window.ECScope) {
        console.error('[ECScope] ECScope namespace not found!');
        return;
    }
    
    // ECScope JavaScript API
    var ECScope = window.ECScope;
    
    // Application wrapper class
    ECScope.Application = function(config) {
        this.config = config || {};
        this.nativeApp = null;
        this.initialized = false;
        this.running = false;
        
        // Default configuration
        this.config = Object.assign({
            canvas: 'canvas',
            title: 'ECScope Application',
            width: 800,
            height: 600,
            webgl: {
                alpha: true,
                depth: true,
                stencil: false,
                antialias: true,
                premultipliedAlpha: true,
                preserveDrawingBuffer: false,
                powerPreferenceHighPerformance: true,
                failIfMajorPerformanceCaveat: false,
                majorVersion: 2,
                minorVersion: 0
            },
            audio: {
                sampleRate: 44100,
                bufferSize: 1024,
                channels: 2,
                enableSpatialAudio: true,
                enableEffects: true
            },
            enableInput: true,
            enableNetworking: true,
            enableFilesystem: true,
            enablePerformanceMonitoring: true,
            enableErrorReporting: true
        }, this.config);
        
        // Get canvas element
        if (typeof this.config.canvas === 'string') {
            this.canvas = document.getElementById(this.config.canvas);
            if (!this.canvas) {
                this.canvas = document.querySelector(this.config.canvas);
            }
        } else {
            this.canvas = this.config.canvas;
        }
        
        if (!this.canvas) {
            throw new Error('Canvas element not found: ' + this.config.canvas);
        }
        
        // Setup canvas
        this.canvas.width = this.config.width;
        this.canvas.height = this.config.height;
        
        // Setup auto-resize
        ECScope.setupCanvasAutoResize(this.canvas);
        
        // Event handlers
        this.eventHandlers = {
            error: [],
            input: [],
            performance: [],
            ready: []
        };
        
        // Performance metrics
        this.performance = {
            fps: 0,
            frameTime: 0,
            updateTime: 0,
            renderTime: 0,
            drawCalls: 0,
            triangles: 0,
            memory: {
                heapSize: 0,
                heapUsed: 0,
                heapLimit: 0,
                stackSize: 0,
                stackUsed: 0,
                memoryPressure: 0
            }
        };
    };
    
    ECScope.Application.prototype.initialize = function(callback) {
        var self = this;
        
        if (this.initialized) {
            if (callback) callback(true);
            return Promise.resolve(true);
        }
        
        return new Promise(function(resolve, reject) {
            try {
                // Create native application configuration
                var nativeConfig = new Module.WebApplicationConfig();
                nativeConfig.title = self.config.title;
                
                // Canvas info
                var canvasInfo = new Module.CanvasInfo();
                canvasInfo.canvas_id = self.canvas.id || 'canvas';
                canvasInfo.width = self.canvas.width;
                canvasInfo.height = self.canvas.height;
                canvasInfo.has_webgl2 = ECScope.capabilities.webgl2;
                canvasInfo.has_webgpu = ECScope.capabilities.webgpu;
                nativeConfig.canvas = canvasInfo;
                
                // WebGL config
                var webglConfig = new Module.WebGLConfig();
                Object.keys(self.config.webgl).forEach(function(key) {
                    if (webglConfig.hasOwnProperty(key)) {
                        webglConfig[key] = self.config.webgl[key];
                    }
                });
                nativeConfig.webgl = webglConfig;
                
                // Audio config
                var audioConfig = new Module.WebAudioConfig();
                Object.keys(self.config.audio).forEach(function(key) {
                    if (audioConfig.hasOwnProperty(key)) {
                        audioConfig[key] = self.config.audio[key];
                    }
                });
                nativeConfig.audio = audioConfig;
                
                // Other settings
                nativeConfig.enable_input = self.config.enableInput;
                nativeConfig.enable_networking = self.config.enableNetworking;
                nativeConfig.enable_filesystem = self.config.enableFilesystem;
                nativeConfig.enable_performance_monitoring = self.config.enablePerformanceMonitoring;
                nativeConfig.enable_error_reporting = self.config.enableErrorReporting;
                
                // Create native application
                self.nativeApp = new Module.WebApplication(nativeConfig);
                
                // Initialize
                var success = self.nativeApp.initialize();
                if (success) {
                    self.initialized = true;
                    self.running = true;
                    
                    // Start performance monitoring
                    if (self.config.enablePerformanceMonitoring) {
                        self.startPerformanceMonitoring();
                    }
                    
                    // Trigger ready event
                    self.triggerEvent('ready', {});
                    
                    if (callback) callback(true);
                    resolve(true);
                } else {
                    var error = new Error('Failed to initialize ECScope application');
                    self.triggerEvent('error', { error: error });
                    if (callback) callback(false);
                    reject(error);
                }
                
            } catch (e) {
                self.triggerEvent('error', { error: e });
                if (callback) callback(false);
                reject(e);
            }
        });
    };
    
    ECScope.Application.prototype.shutdown = function() {
        if (this.nativeApp) {
            this.nativeApp.shutdown();
            this.nativeApp = null;
        }
        this.initialized = false;
        this.running = false;
    };
    
    ECScope.Application.prototype.resize = function(width, height) {
        if (this.nativeApp) {
            this.nativeApp.resize(width, height);
        }
        
        this.canvas.width = width;
        this.canvas.height = height;
    };
    
    ECScope.Application.prototype.setVisibility = function(visible) {
        if (this.nativeApp) {
            this.nativeApp.set_visibility(visible);
        }
    };
    
    ECScope.Application.prototype.setFocus = function(focused) {
        if (this.nativeApp) {
            this.nativeApp.set_focus(focused);
        }
    };
    
    ECScope.Application.prototype.getRenderer = function() {
        if (!this.nativeApp) {
            throw new Error('Application not initialized');
        }
        return this.nativeApp.get_renderer();
    };
    
    ECScope.Application.prototype.getAudio = function() {
        if (!this.nativeApp) {
            throw new Error('Application not initialized');
        }
        return this.nativeApp.get_audio();
    };
    
    ECScope.Application.prototype.getInput = function() {
        if (!this.nativeApp) {
            throw new Error('Application not initialized');
        }
        return this.nativeApp.get_input();
    };
    
    ECScope.Application.prototype.getBrowserCapabilities = function() {
        if (this.nativeApp) {
            return this.nativeApp.get_browser_capabilities();
        }
        return ECScope.capabilities;
    };
    
    ECScope.Application.prototype.getPerformanceMetrics = function() {
        if (this.nativeApp) {
            return this.nativeApp.get_performance_metrics();
        }
        return this.performance;
    };
    
    ECScope.Application.prototype.loadAsset = function(url) {
        var self = this;
        return new Promise(function(resolve, reject) {
            if (!self.nativeApp) {
                reject(new Error('Application not initialized'));
                return;
            }
            
            self.nativeApp.load_asset(url, function(success, data) {
                if (success) {
                    resolve(data);
                } else {
                    reject(new Error('Failed to load asset: ' + url));
                }
            });
        });
    };
    
    ECScope.Application.prototype.executeJavaScript = function(code) {
        if (!this.nativeApp) {
            throw new Error('Application not initialized');
        }
        return this.nativeApp.execute_javascript(code);
    };
    
    // Event handling
    ECScope.Application.prototype.on = function(event, handler) {
        if (!this.eventHandlers[event]) {
            this.eventHandlers[event] = [];
        }
        this.eventHandlers[event].push(handler);
    };
    
    ECScope.Application.prototype.off = function(event, handler) {
        if (!this.eventHandlers[event]) return;
        
        var index = this.eventHandlers[event].indexOf(handler);
        if (index !== -1) {
            this.eventHandlers[event].splice(index, 1);
        }
    };
    
    ECScope.Application.prototype.triggerEvent = function(event, data) {
        if (!this.eventHandlers[event]) return;
        
        this.eventHandlers[event].forEach(function(handler) {
            try {
                handler(data);
            } catch (e) {
                console.error('[ECScope] Error in event handler:', e);
            }
        });
    };
    
    // Performance monitoring
    ECScope.Application.prototype.startPerformanceMonitoring = function() {
        var self = this;
        
        function updateMetrics() {
            if (!self.running || !self.nativeApp) return;
            
            try {
                var metrics = self.nativeApp.get_performance_metrics();
                self.performance = metrics;
                
                // Trigger performance event
                self.triggerEvent('performance', metrics);
                
                // Update global performance
                ECScope.updatePerformanceMetrics();
                
            } catch (e) {
                console.error('[ECScope] Error updating performance metrics:', e);
            }
            
            // Schedule next update
            if (self.running) {
                requestAnimationFrame(updateMetrics);
            }
        }
        
        requestAnimationFrame(updateMetrics);
    };
    
    // Utility functions
    ECScope.loadBinary = function(url) {
        return Module.AsyncLoader.load_binary(url);
    };
    
    ECScope.loadText = function(url) {
        return Module.AsyncLoader.load_text(url);
    };
    
    ECScope.loadJSON = function(url) {
        return Module.AsyncLoader.load_json(url);
    };
    
    ECScope.loadImage = function(url) {
        return Module.AsyncLoader.load_image(url);
    };
    
    ECScope.loadAudio = function(url) {
        return Module.AsyncLoader.load_audio(url);
    };
    
    // Version information
    ECScope.version = '1.0.0';
    ECScope.buildDate = new Date().toISOString();
    
    // Global error handling
    window.addEventListener('error', function(event) {
        console.error('[ECScope] Global JavaScript error:', event.error);
    });
    
    // Handle visibility changes
    document.addEventListener('visibilitychange', function() {
        if (ECScope.currentApplication) {
            ECScope.currentApplication.setVisibility(!document.hidden);
        }
    });
    
    // Handle focus changes
    window.addEventListener('focus', function() {
        if (ECScope.currentApplication) {
            ECScope.currentApplication.setFocus(true);
        }
    });
    
    window.addEventListener('blur', function() {
        if (ECScope.currentApplication) {
            ECScope.currentApplication.setFocus(false);
        }
    });
    
    // Handle resize
    window.addEventListener('resize', function() {
        if (ECScope.currentApplication && ECScope.currentApplication.canvas) {
            var canvas = ECScope.currentApplication.canvas;
            ECScope.currentApplication.resize(canvas.clientWidth, canvas.clientHeight);
        }
    });
    
    console.log('[ECScope] Post-JS initialization complete');
    console.log('[ECScope] ECScope v' + ECScope.version + ' ready for use');
    
})();