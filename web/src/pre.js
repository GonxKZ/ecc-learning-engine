// ECScope WebAssembly Module Pre-JS
// This file is loaded before the WebAssembly module is initialized

(function() {
    'use strict';
    
    // Global ECScope namespace
    window.ECScope = window.ECScope || {};
    
    // Module configuration
    var ModuleConfig = {
        // Memory configuration
        INITIAL_MEMORY: 64 * 1024 * 1024,  // 64MB
        MAXIMUM_MEMORY: 2 * 1024 * 1024 * 1024,  // 2GB
        
        // Threading configuration
        PTHREAD_POOL_SIZE: navigator.hardwareConcurrency || 4,
        
        // Canvas configuration
        canvas: null,
        
        // Print functions
        print: function(text) {
            console.log('[ECScope] ' + text);
        },
        
        printErr: function(text) {
            console.error('[ECScope Error] ' + text);
        },
        
        // Progress callbacks
        onRuntimeInitialized: function() {
            console.log('[ECScope] Runtime initialized');
            window.ECScope.ready = true;
            
            // Trigger ready event
            var event = new CustomEvent('ecscopeready', {
                detail: { module: Module }
            });
            window.dispatchEvent(event);
        },
        
        onAbort: function(what) {
            console.error('[ECScope] Aborted: ' + what);
            window.ECScope.error = what;
            
            // Trigger error event
            var event = new CustomEvent('ecscopeerror', {
                detail: { error: what }
            });
            window.dispatchEvent(event);
        },
        
        // WebGL context configuration
        webglContextAttributes: {
            alpha: true,
            depth: true,
            stencil: false,
            antialias: true,
            premultipliedAlpha: true,
            preserveDrawingBuffer: false,
            powerPreference: 'high-performance',
            failIfMajorPerformanceCaveat: false,
            majorVersion: 2,
            minorVersion: 0
        },
        
        // Audio configuration
        audioContext: null,
        
        // Performance monitoring
        performance: {
            enabled: true,
            frameCount: 0,
            startTime: Date.now(),
            lastFrameTime: 0,
            frameTimeHistory: [],
            maxHistorySize: 60
        }
    };
    
    // Browser capability detection
    function detectCapabilities() {
        var capabilities = {
            webgl2: false,
            webgpu: false,
            simd: false,
            threads: false,
            sharedArrayBuffer: false,
            wasmBulkMemory: false,
            fileSystemAccess: false,
            webAudioWorklet: false,
            offscreenCanvas: false
        };
        
        // WebGL2 detection
        try {
            var canvas = document.createElement('canvas');
            var gl = canvas.getContext('webgl2');
            capabilities.webgl2 = !!gl;
            if (gl) {
                canvas = null;
            }
        } catch (e) {
            capabilities.webgl2 = false;
        }
        
        // WebGPU detection
        capabilities.webgpu = typeof navigator.gpu !== 'undefined';
        
        // SIMD detection
        capabilities.simd = typeof WebAssembly.SIMD !== 'undefined';
        
        // SharedArrayBuffer detection
        capabilities.sharedArrayBuffer = typeof SharedArrayBuffer !== 'undefined';
        
        // Threading support (requires SharedArrayBuffer and proper headers)
        capabilities.threads = capabilities.sharedArrayBuffer && 
                             typeof Worker !== 'undefined';
        
        // File System Access API detection
        capabilities.fileSystemAccess = typeof window.showOpenFilePicker !== 'undefined';
        
        // Web Audio Worklet detection
        try {
            capabilities.webAudioWorklet = typeof AudioWorkletNode !== 'undefined';
        } catch (e) {
            capabilities.webAudioWorklet = false;
        }
        
        // OffscreenCanvas detection
        capabilities.offscreenCanvas = typeof OffscreenCanvas !== 'undefined';
        
        return capabilities;
    }
    
    // Initialize audio context (must be done after user gesture)
    function initializeAudioContext() {
        if (ModuleConfig.audioContext) {
            return ModuleConfig.audioContext;
        }
        
        try {
            var AudioContext = window.AudioContext || window.webkitAudioContext;
            ModuleConfig.audioContext = new AudioContext({
                sampleRate: 44100,
                latencyHint: 'interactive'
            });
            
            // Handle suspended context
            if (ModuleConfig.audioContext.state === 'suspended') {
                console.log('[ECScope] Audio context suspended, will resume on user interaction');
                
                var resumeAudio = function() {
                    ModuleConfig.audioContext.resume().then(function() {
                        console.log('[ECScope] Audio context resumed');
                        document.removeEventListener('click', resumeAudio);
                        document.removeEventListener('touchstart', resumeAudio);
                        document.removeEventListener('keydown', resumeAudio);
                    });
                };
                
                document.addEventListener('click', resumeAudio);
                document.addEventListener('touchstart', resumeAudio);
                document.addEventListener('keydown', resumeAudio);
            }
            
            return ModuleConfig.audioContext;
        } catch (e) {
            console.error('[ECScope] Failed to create audio context:', e);
            return null;
        }
    }
    
    // Performance monitoring utilities
    function updatePerformanceMetrics() {
        if (!ModuleConfig.performance.enabled) return;
        
        var now = performance.now();
        var frameTime = now - ModuleConfig.performance.lastFrameTime;
        
        ModuleConfig.performance.frameCount++;
        ModuleConfig.performance.lastFrameTime = now;
        
        // Update frame time history
        ModuleConfig.performance.frameTimeHistory.push(frameTime);
        if (ModuleConfig.performance.frameTimeHistory.length > ModuleConfig.performance.maxHistorySize) {
            ModuleConfig.performance.frameTimeHistory.shift();
        }
        
        // Calculate average FPS over the last second
        if (ModuleConfig.performance.frameCount % 60 === 0) {
            var avgFrameTime = ModuleConfig.performance.frameTimeHistory.reduce(function(a, b) {
                return a + b;
            }, 0) / ModuleConfig.performance.frameTimeHistory.length;
            
            var fps = 1000 / avgFrameTime;
            
            // Dispatch performance event
            var event = new CustomEvent('ecscopeperformance', {
                detail: {
                    fps: fps,
                    frameTime: avgFrameTime,
                    frameCount: ModuleConfig.performance.frameCount,
                    uptime: now - ModuleConfig.performance.startTime
                }
            });
            window.dispatchEvent(event);
        }
    }
    
    // Canvas auto-resize functionality
    function setupCanvasAutoResize(canvas) {
        if (!canvas) return;
        
        function resizeCanvas() {
            var displayWidth = canvas.clientWidth;
            var displayHeight = canvas.clientHeight;
            
            if (canvas.width !== displayWidth || canvas.height !== displayHeight) {
                canvas.width = displayWidth;
                canvas.height = displayHeight;
                
                // Notify ECScope of resize
                if (window.ECScope.ready && Module && Module._ecscope_resize) {
                    Module._ecscope_resize(displayWidth, displayHeight);
                }
                
                console.log('[ECScope] Canvas resized to', displayWidth, 'x', displayHeight);
            }
        }
        
        // Initial resize
        resizeCanvas();
        
        // Listen for window resize
        window.addEventListener('resize', resizeCanvas);
        
        // Use ResizeObserver if available
        if (typeof ResizeObserver !== 'undefined') {
            var resizeObserver = new ResizeObserver(resizeCanvas);
            resizeObserver.observe(canvas);
        }
    }
    
    // Error handling
    window.addEventListener('error', function(event) {
        console.error('[ECScope] Global error:', event.error);
    });
    
    window.addEventListener('unhandledrejection', function(event) {
        console.error('[ECScope] Unhandled promise rejection:', event.reason);
    });
    
    // Detect capabilities on load
    var capabilities = detectCapabilities();
    console.log('[ECScope] Browser capabilities:', capabilities);
    
    // Store capabilities for access from C++
    window.ECScope.capabilities = capabilities;
    
    // Initialize audio context
    window.ECScope.getAudioContext = initializeAudioContext;
    
    // Performance monitoring
    window.ECScope.updatePerformanceMetrics = updatePerformanceMetrics;
    
    // Canvas utilities
    window.ECScope.setupCanvasAutoResize = setupCanvasAutoResize;
    
    // Module configuration
    window.ECScope.ModuleConfig = ModuleConfig;
    
    // Global Module variable for Emscripten
    window.Module = ModuleConfig;
    
    console.log('[ECScope] Pre-JS initialization complete');
    
})();