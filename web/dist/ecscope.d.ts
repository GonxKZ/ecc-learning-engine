/**
 * ECScope WebAssembly Engine - TypeScript Definitions
 * 
 * Professional-grade WebAssembly integration for modern web browsers
 * with comprehensive TypeScript support for the ECScope engine.
 */

declare global {
    interface Window {
        ECScope: typeof ECScope;
        Module: EmscriptenModule;
    }
}

// Core ECScope namespace
declare namespace ECScope {
    // Version information
    const version: string;
    const buildDate: string;
    
    // Initialization state
    let ready: boolean;
    let error?: string;
    
    // Browser capabilities
    interface BrowserCapabilities {
        webgl2Support: boolean;
        webgpuSupport: boolean;
        simdSupport: boolean;
        threadsSupport: boolean;
        sharedArrayBuffer: boolean;
        wasmBulkMemory: boolean;
        fileSystemAccess: boolean;
        webAudioWorklet: boolean;
        offscreenCanvas: boolean;
        userAgent: string;
        webglRenderer: string;
        webglVendor: string;
    }
    
    // Memory information
    interface MemoryInfo {
        heapSize: number;
        heapUsed: number;
        heapLimit: number;
        stackSize: number;
        stackUsed: number;
        memoryPressure: number;
    }
    
    // Performance metrics
    interface PerformanceMetrics {
        frameTimeMs: number;
        updateTimeMs: number;
        renderTimeMs: number;
        fps: number;
        drawCalls: number;
        triangles: number;
        memory: MemoryInfo;
    }
    
    // Canvas configuration
    interface CanvasInfo {
        canvasId: string;
        width: number;
        height: number;
        hasWebgl2: boolean;
        hasWebgpu: boolean;
    }
    
    // WebGL configuration
    interface WebGLConfig {
        alpha?: boolean;
        depth?: boolean;
        stencil?: boolean;
        antialias?: boolean;
        premultipliedAlpha?: boolean;
        preserveDrawingBuffer?: boolean;
        powerPreferenceHighPerformance?: boolean;
        failIfMajorPerformanceCaveat?: boolean;
        majorVersion?: number;
        minorVersion?: number;
    }
    
    // Audio configuration
    interface WebAudioConfig {
        sampleRate?: number;
        bufferSize?: number;
        channels?: number;
        enableSpatialAudio?: boolean;
        enableEffects?: boolean;
    }
    
    // Application configuration
    interface ApplicationConfig {
        canvas?: string | HTMLCanvasElement;
        title?: string;
        width?: number;
        height?: number;
        webgl?: WebGLConfig;
        audio?: WebAudioConfig;
        enableInput?: boolean;
        enableNetworking?: boolean;
        enableFilesystem?: boolean;
        enablePerformanceMonitoring?: boolean;
        enableErrorReporting?: boolean;
    }
    
    // Event types
    type ErrorHandler = (error: Error) => void;
    type InputHandler = (event: InputEvent) => void;
    type PerformanceHandler = (metrics: PerformanceMetrics) => void;
    type ReadyHandler = () => void;
    
    // Input system types
    enum InputEventType {
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseMove,
        MouseWheel,
        TouchStart,
        TouchMove,
        TouchEnd,
        GamepadConnected,
        GamepadDisconnected
    }
    
    enum MouseButton {
        Left = 0,
        Middle = 1,
        Right = 2,
        Back = 3,
        Forward = 4
    }
    
    enum KeyCode {
        A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72, I = 73, J = 74,
        K = 75, L = 76, M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84,
        U = 85, V = 86, W = 87, X = 88, Y = 89, Z = 90,
        Num0 = 48, Num1 = 49, Num2 = 50, Num3 = 51, Num4 = 52,
        Num5 = 53, Num6 = 54, Num7 = 55, Num8 = 56, Num9 = 57,
        Space = 32, Enter = 13, Escape = 27, Tab = 9, Backspace = 8, Delete = 46,
        Shift = 16, Ctrl = 17, Alt = 18, Meta = 91,
        Left = 37, Up = 38, Right = 39, Down = 40,
        F1 = 112, F2 = 113, F3 = 114, F4 = 115, F5 = 116, F6 = 117,
        F7 = 118, F8 = 119, F9 = 120, F10 = 121, F11 = 122, F12 = 123
    }
    
    interface InputEvent {
        type: InputEventType;
        timestamp: number;
        key?: string;
        keyCode?: number;
        ctrlKey?: boolean;
        altKey?: boolean;
        shiftKey?: boolean;
        metaKey?: boolean;
        mouseX?: number;
        mouseY?: number;
        deltaX?: number;
        deltaY?: number;
        mouseButton?: number;
        touchPoints?: Array<{x: number, y: number}>;
        gamepadIndex?: number;
    }
    
    interface TouchPoint {
        identifier: number;
        x: number;
        y: number;
        radiusX: number;
        radiusY: number;
        rotationAngle: number;
        force: number;
        active: boolean;
    }
    
    enum GestureType {
        None,
        Tap,
        DoubleTap,
        LongPress,
        Swipe,
        Pinch,
        Rotate,
        Pan
    }
    
    interface Gesture {
        type: GestureType;
        x: number;
        y: number;
        deltaX: number;
        deltaY: number;
        scale: number;
        rotation: number;
        velocityX: number;
        velocityY: number;
        duration: number;
    }
    
    // Audio system types
    enum AudioNodeType {
        Source,
        Gain,
        Filter,
        Delay,
        Reverb,
        Compressor,
        Analyzer,
        Panner,
        Destination
    }
    
    interface AudioListener {
        x: number;
        y: number;
        z: number;
        forwardX: number;
        forwardY: number;
        forwardZ: number;
        upX: number;
        upY: number;
        upZ: number;
    }
    
    interface EffectParameters {
        frequency?: number;
        q?: number;
        gain?: number;
        delayTime?: number;
        feedback?: number;
        mix?: number;
        roomSize?: number;
        decayTime?: number;
        damping?: number;
        wetMix?: number;
        threshold?: number;
        ratio?: number;
        attack?: number;
        release?: number;
        drive?: number;
        outputGain?: number;
    }
    
    // Renderer types
    enum RendererBackend {
        WebGL2,
        WebGPU,
        Auto
    }
    
    interface RenderTarget {
        canvasId: string;
        width: number;
        height: number;
        devicePixelRatio: number;
        isOffscreen: boolean;
    }
    
    interface RenderStats {
        drawCalls: number;
        triangles: number;
        vertices: number;
        textureSwitches: number;
        shaderSwitches: number;
        frameTimeMs: number;
    }
    
    // Main Application class
    class Application {
        constructor(config?: ApplicationConfig);
        
        // Lifecycle
        initialize(callback?: (success: boolean) => void): Promise<boolean>;
        shutdown(): void;
        resize(width: number, height: number): void;
        setVisibility(visible: boolean): void;
        setFocus(focused: boolean): void;
        
        // Getters
        getRenderer(): Renderer;
        getAudio(): Audio;
        getInput(): Input;
        getBrowserCapabilities(): BrowserCapabilities;
        getPerformanceMetrics(): PerformanceMetrics;
        
        // Asset loading
        loadAsset(url: string): Promise<Uint8Array>;
        
        // JavaScript integration
        executeJavaScript(code: string): any;
        
        // Event handling
        on(event: 'error', handler: ErrorHandler): void;
        on(event: 'input', handler: InputHandler): void;
        on(event: 'performance', handler: PerformanceHandler): void;
        on(event: 'ready', handler: ReadyHandler): void;
        off(event: string, handler: Function): void;
        
        // Properties
        readonly initialized: boolean;
        readonly running: boolean;
        readonly canvas: HTMLCanvasElement;
        readonly performance: PerformanceMetrics;
    }
    
    // Renderer class
    class Renderer {
        // Lifecycle
        initialize(): boolean;
        shutdown(): void;
        
        // Frame operations
        beginFrame(): void;
        endFrame(): void;
        resize(width: number, height: number): void;
        setViewport(x: number, y: number, width: number, height: number): void;
        clear(r: number, g: number, b: number, a?: number): void;
        
        // Resource management
        createShaderProgram(vertexSource: string, fragmentSource: string): number;
        deleteShaderProgram(program: number): void;
        createVertexBuffer(data: ArrayBuffer | ArrayBufferView, usage?: number): number;
        createIndexBuffer(data: ArrayBuffer | ArrayBufferView, usage?: number): number;
        updateBuffer(buffer: number, offset: number, data: ArrayBuffer | ArrayBufferView): void;
        deleteBuffer(buffer: number): void;
        createTexture(width: number, height: number, format: number, type: number, data?: ArrayBuffer | ArrayBufferView): number;
        deleteTexture(texture: number): void;
        createFramebuffer(width: number, height: number, colorAttachments?: number, hasDepth?: boolean, hasStencil?: boolean): number;
        bindFramebuffer(framebuffer: number): void;
        deleteFramebuffer(framebuffer: number): void;
        
        // Drawing
        drawIndexed(mode: number, count: number, type: number, offset?: number): void;
        drawArrays(mode: number, first: number, count: number): void;
        
        // State management
        setFeature(feature: number, enable: boolean): void;
        setBlendMode(srcFactor: number, dstFactor: number): void;
        setDepthTest(enable: boolean, func?: number): void;
        setCulling(enable: boolean, face?: number): void;
        
        // Statistics
        getRenderStats(): RenderStats;
        resetRenderStats(): void;
        
        // Properties
        readonly backend: RendererBackend;
        readonly target: RenderTarget;
        readonly initialized: boolean;
    }
    
    // Audio class
    class Audio {
        // Lifecycle
        initialize(): boolean;
        shutdown(): void;
        update(deltaTime: number): void;
        
        // Context management
        resumeContext(): boolean;
        suspendContext(): boolean;
        readonly contextRunning: boolean;
        
        // Buffer management
        createAudioBuffer(sampleRate: number, channels: number, data: Float32Array): number;
        deleteAudioBuffer(bufferId: number): void;
        
        // Source management
        createAudioSource(bufferId: number): number;
        deleteAudioSource(sourceId: number): void;
        playSource(sourceId: number, startTime?: number): void;
        stopSource(sourceId: number, stopTime?: number): void;
        pauseSource(sourceId: number): void;
        resumeSource(sourceId: number): void;
        
        // Source properties
        setSourceVolume(sourceId: number, volume: number): void;
        setSourcePitch(sourceId: number, pitch: number): void;
        setSourceLooping(sourceId: number, looping: boolean): void;
        setSourcePosition(sourceId: number, x: number, y: number, z: number): void;
        setSourceVelocity(sourceId: number, vx: number, vy: number, vz: number): void;
        
        // Listener
        setListener(listener: AudioListener): void;
        
        // Master controls
        setMasterVolume(volume: number): void;
        getMasterVolume(): number;
        
        // Effects
        createEffectNode(type: AudioNodeType, params: EffectParameters): number;
        updateEffectParameters(nodeId: number, params: EffectParameters): void;
        connectNodes(sourceId: number, destinationId: number): void;
        disconnectNodes(sourceId: number, destinationId?: number): void;
        deleteEffectNode(nodeId: number): void;
        
        // Analysis
        getAnalysisData(analyzerId: number): {frequencyData: Float32Array, timeData: Float32Array};
        
        // Recording
        startRecording(callback: (data: Float32Array) => void): boolean;
        stopRecording(): void;
        readonly recording: boolean;
        
        // Synthesis
        generateTone(frequency: number, duration: number, volume: number, waveType?: string): number;
        
        // Properties
        getCurrentTime(): number;
        getSampleRate(): number;
        readonly initialized: boolean;
    }
    
    // Input class
    class Input {
        // Lifecycle
        initialize(): boolean;
        shutdown(): void;
        update(deltaTime: number): void;
        
        // Keyboard
        isKeyDown(keyCode: KeyCode): boolean;
        isKeyPressed(keyCode: KeyCode): boolean;
        isKeyReleased(keyCode: KeyCode): boolean;
        getTypedText(): string;
        
        // Mouse
        isMouseButtonDown(button: MouseButton): boolean;
        isMouseButtonPressed(button: MouseButton): boolean;
        isMouseButtonReleased(button: MouseButton): boolean;
        getMousePosition(): {x: number, y: number};
        getMouseDelta(): {x: number, y: number};
        getMouseWheelDelta(): {x: number, y: number};
        setCursorVisible(visible: boolean): void;
        lockPointer(): boolean;
        unlockPointer(): void;
        readonly pointerLocked: boolean;
        
        // Touch
        getTouchPoints(): TouchPoint[];
        getTouchPoint(identifier: number): TouchPoint | null;
        getCurrentGesture(): Gesture;
        setGestureRecognition(enable: boolean): void;
        
        // Gamepad
        getGamepadState(index: number): any; // Gamepad state interface would be defined
        isGamepadButtonDown(gamepadIndex: number, buttonIndex: number): boolean;
        getGamepadAxis(gamepadIndex: number, axisIndex: number): number;
        setGamepadVibration(gamepadIndex: number, lowFreq: number, highFreq: number, duration: number): void;
        
        // State management
        clearState(): void;
        setFocus(focused: boolean): void;
        readonly hasFocus: boolean;
        readonly initialized: boolean;
        
        // Event handling
        setInputCallback(callback: InputHandler): void;
    }
    
    // Utility functions
    function loadBinary(url: string): Promise<Uint8Array>;
    function loadText(url: string): Promise<string>;
    function loadJSON(url: string): Promise<any>;
    function loadImage(url: string): Promise<ImageBitmap>;
    function loadAudio(url: string): Promise<AudioBuffer>;
    
    // Global application instance
    let currentApplication: Application | null;
    
    // Browser capabilities
    const capabilities: BrowserCapabilities;
    
    // Audio context getter
    function getAudioContext(): AudioContext | null;
    
    // Performance utilities
    function updatePerformanceMetrics(): void;
    
    // Canvas utilities
    function setupCanvasAutoResize(canvas: HTMLCanvasElement): void;
    
    // Module configuration
    const ModuleConfig: {
        INITIAL_MEMORY: number;
        MAXIMUM_MEMORY: number;
        PTHREAD_POOL_SIZE: number;
        canvas: HTMLCanvasElement | null;
        print: (text: string) => void;
        printErr: (text: string) => void;
        onRuntimeInitialized: () => void;
        onAbort: (what: string) => void;
        webglContextAttributes: WebGLContextAttributes;
        audioContext: AudioContext | null;
        performance: {
            enabled: boolean;
            frameCount: number;
            startTime: number;
            lastFrameTime: number;
            frameTimeHistory: number[];
            maxHistorySize: number;
        };
    };
}

// Global events
declare global {
    interface WindowEventMap {
        'ecscopeready': CustomEvent<{module: EmscriptenModule}>;
        'ecscopeerror': CustomEvent<{error: string}>;
        'ecscopeperformance': CustomEvent<{
            fps: number;
            frameTime: number;
            frameCount: number;
            uptime: number;
        }>;
    }
}

// Emscripten module interface (basic definitions)
interface EmscriptenModule {
    ready: Promise<EmscriptenModule>;
    onRuntimeInitialized?: () => void;
    print?: (str: string) => void;
    printErr?: (str: string) => void;
    canvas?: HTMLCanvasElement;
    ccall?: (ident: string, returnType: string | null, argTypes: string[], args: any[]) => any;
    cwrap?: (ident: string, returnType: string | null, argTypes: string[]) => (...args: any[]) => any;
    addOnPreMain?: (cb: () => void) => void;
    addOnInit?: (cb: () => void) => void;
    addOnPreRun?: (cb: () => void) => void;
    addOnPostRun?: (cb: () => void) => void;
    FS?: any;
    ENV?: any;
    getTotalMemory?: () => number;
}

// Export the ECScope namespace as the default export
export = ECScope;
export as namespace ECScope;