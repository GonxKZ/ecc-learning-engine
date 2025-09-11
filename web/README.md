# ECScope WebAssembly Export System

Professional-grade WebAssembly integration for the ECScope engine, enabling deployment of high-performance applications to modern web browsers with full JavaScript interoperability.

## Features

### Core WebAssembly Integration
- **Complete Emscripten Toolchain**: Optimized build system with CMake integration
- **WASM Module Generation**: Optimized builds with size and performance options
- **JavaScript/WebAssembly Bindings**: Comprehensive Embind integration
- **Memory Management**: Efficient WASM/JS memory handling with SharedArrayBuffer support
- **Threading Support**: WebAssembly threads with SharedArrayBuffer and Atomics

### Browser API Integration
- **WebGL/WebGPU Rendering**: High-performance graphics with modern APIs
- **Web Audio API**: Spatial audio, effects processing, and real-time synthesis
- **Canvas and DOM**: Direct integration with browser rendering
- **File System Access**: Modern file handling with File System Access API
- **WebSockets**: Real-time networking capabilities
- **IndexedDB**: Persistent storage for assets and save data

### Performance Optimization
- **SIMD Optimizations**: WebAssembly SIMD for vector operations
- **Bulk Memory Operations**: Efficient memory copying and manipulation
- **Memory Allocation Strategies**: Optimized allocators for different use cases
- **Code Splitting**: Lazy loading and streaming compilation
- **Call Overhead Minimization**: Optimized JavaScript/WASM boundary

### Development Tools
- **Professional Build System**: Python-based build tool with caching
- **Source Maps**: Full debugging support for WebAssembly
- **Performance Profiling**: Real-time performance monitoring
- **Hot Reload**: Development server with automatic reloading
- **Browser Compatibility**: Comprehensive feature detection and polyfills

### TypeScript Integration
- **Complete Type Definitions**: Full TypeScript support with detailed interfaces
- **JavaScript Wrapper Classes**: High-level APIs for easy integration
- **Promise-based Async Operations**: Modern async/await patterns
- **Event Handling**: Type-safe event system
- **Error Handling**: Comprehensive error management

## Quick Start

### Prerequisites

- **Emscripten SDK** (latest version)
- **CMake** 3.20 or higher
- **Python** 3.8 or higher
- **Node.js** 14+ (for development tools)

### Installation

1. **Install Emscripten SDK**:
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```

2. **Clone ECScope**:
   ```bash
   git clone https://github.com/ecscope/ecscope.git
   cd ecscope/web
   ```

3. **Build the project**:
   ```bash
   # Debug build
   python3 tools/build.py build --target debug
   
   # Release build
   python3 tools/build.py build --target release
   
   # Optimized for size
   python3 tools/build.py build --target size
   ```

4. **Serve the application**:
   ```bash
   python3 tools/build.py serve
   ```

Visit `http://localhost:8080` to see your application running.

### Basic Usage

```html
<!DOCTYPE html>
<html>
<head>
    <title>ECScope Application</title>
    <script src="ecscope.js"></script>
</head>
<body>
    <canvas id="canvas" width="800" height="600"></canvas>
    
    <script>
        async function main() {
            // Wait for ECScope to load
            await new Promise(resolve => {
                if (window.ECScope && window.ECScope.ready) {
                    resolve();
                } else {
                    window.addEventListener('ecscopeready', resolve);
                }
            });
            
            // Create application
            const app = new ECScope.Application({
                canvas: 'canvas',
                width: 800,
                height: 600,
                webgl: {
                    alpha: false,
                    antialias: true
                },
                audio: {
                    enableSpatialAudio: true
                }
            });
            
            // Initialize
            await app.initialize();
            
            // Your application logic here
            console.log('ECScope application ready!');
        }
        
        main().catch(console.error);
    </script>
</body>
</html>
```

### TypeScript Usage

```typescript
import * as ECScope from '@ecscope/web';

class MyApplication {
    private app: ECScope.Application;
    
    async initialize(): Promise<void> {
        // Wait for ECScope module
        await this.waitForECScope();
        
        // Configure application
        const config: ECScope.ApplicationConfig = {
            canvas: document.getElementById('canvas') as HTMLCanvasElement,
            title: 'My ECScope Application',
            width: 1920,
            height: 1080,
            webgl: {
                alpha: false,
                depth: true,
                antialias: true,
                powerPreferenceHighPerformance: true
            },
            audio: {
                enableSpatialAudio: true,
                enableEffects: true
            },
            enableInput: true,
            enableNetworking: true,
            enablePerformanceMonitoring: true
        };
        
        // Create and initialize application
        this.app = new ECScope.Application(config);
        
        // Set up event handlers
        this.app.on('error', this.handleError.bind(this));
        this.app.on('performance', this.handlePerformance.bind(this));
        this.app.on('ready', this.handleReady.bind(this));
        
        // Initialize
        await this.app.initialize();
    }
    
    private async waitForECScope(): Promise<void> {
        return new Promise((resolve) => {
            if (window.ECScope && window.ECScope.ready) {
                resolve();
            } else {
                window.addEventListener('ecscopeready', () => resolve());
            }
        });
    }
    
    private handleError(error: Error): void {
        console.error('ECScope error:', error);
    }
    
    private handlePerformance(metrics: ECScope.PerformanceMetrics): void {
        console.log(`FPS: ${metrics.fps}, Frame Time: ${metrics.frameTimeMs}ms`);
    }
    
    private handleReady(): void {
        console.log('Application ready!');
        this.startGameLoop();
    }
    
    private startGameLoop(): void {
        // Your game loop here
    }
}

// Start application
const myApp = new MyApplication();
myApp.initialize().catch(console.error);
```

## Build System

### Build Targets

- **debug**: Development build with debugging symbols and assertions
- **release**: Optimized production build with aggressive optimizations
- **profile**: Profiling build with performance monitoring
- **size**: Size-optimized build for minimal file size

### Build Configuration

Create a `build.json` file to customize build settings:

```json
{
    "project_name": "MyECScopeApp",
    "version": "1.0.0",
    "features": {
        "simd": true,
        "threads": true,
        "webgl2": true,
        "webaudio": true,
        "bulk_memory": true,
        "shared_memory": true
    },
    "deployment": {
        "gzip": true,
        "brotli": true,
        "generate_service_worker": true,
        "generate_manifest": true
    }
}
```

### Build Commands

```bash
# Build specific target
python3 tools/build.py build --target release

# Clean build artifacts
python3 tools/build.py clean

# Run tests
python3 tools/build.py test --target release

# Analyze build output
python3 tools/build.py analyze --target release

# Serve development server
python3 tools/build.py serve
```

## API Reference

### Application Class

The main entry point for ECScope applications.

```typescript
class Application {
    constructor(config: ApplicationConfig);
    
    // Lifecycle
    initialize(): Promise<boolean>;
    shutdown(): void;
    resize(width: number, height: number): void;
    
    // Subsystems
    getRenderer(): Renderer;
    getAudio(): Audio;
    getInput(): Input;
    
    // Events
    on(event: string, handler: Function): void;
    off(event: string, handler: Function): void;
    
    // Utilities
    loadAsset(url: string): Promise<Uint8Array>;
    executeJavaScript(code: string): any;
}
```

### Renderer Class

High-performance WebGL/WebGPU renderer.

```typescript
class Renderer {
    // Frame operations
    beginFrame(): void;
    endFrame(): void;
    clear(r: number, g: number, b: number, a?: number): void;
    
    // Resource management
    createShaderProgram(vs: string, fs: string): number;
    createVertexBuffer(data: ArrayBuffer): number;
    createTexture(width: number, height: number, format: number): number;
    
    // Drawing
    drawIndexed(mode: number, count: number, type: number): void;
    drawArrays(mode: number, first: number, count: number): void;
    
    // Statistics
    getRenderStats(): RenderStats;
}
```

### Audio Class

Comprehensive audio system with spatial audio support.

```typescript
class Audio {
    // Context management
    resumeContext(): boolean;
    suspendContext(): boolean;
    
    // Buffer management
    createAudioBuffer(sampleRate: number, channels: number, data: Float32Array): number;
    
    // Source management
    createAudioSource(bufferId: number): number;
    playSource(sourceId: number): void;
    setSourcePosition(sourceId: number, x: number, y: number, z: number): void;
    
    // Effects
    createEffectNode(type: AudioNodeType, params: EffectParameters): number;
    connectNodes(sourceId: number, destinationId: number): void;
    
    // Master controls
    setMasterVolume(volume: number): void;
}
```

### Input Class

Comprehensive input handling for all input devices.

```typescript
class Input {
    // Keyboard
    isKeyDown(keyCode: KeyCode): boolean;
    isKeyPressed(keyCode: KeyCode): boolean;
    
    // Mouse
    isMouseButtonDown(button: MouseButton): boolean;
    getMousePosition(): {x: number, y: number};
    lockPointer(): boolean;
    
    // Touch
    getTouchPoints(): TouchPoint[];
    getCurrentGesture(): Gesture;
    
    // Gamepad
    getGamepadState(index: number): GamepadState;
}
```

## Examples

### Basic Demo
- Simple application demonstrating core features
- Renderer, audio, and input integration
- Performance monitoring
- Error handling

### Performance Benchmark
- Comprehensive performance testing suite
- Renderer stress tests
- Memory allocation benchmarks
- Audio system performance
- SIMD operation benchmarks

### Advanced Rendering Demo
- Modern rendering techniques
- Deferred rendering pipeline
- Post-processing effects
- Shadow mapping
- PBR materials

## Performance Optimization

### Memory Management
- Use pool allocators for frequent allocations
- Minimize JavaScript/WASM boundary crossings
- Leverage SharedArrayBuffer for large data transfers
- Implement object pooling for game objects

### Rendering Optimization
- Batch draw calls to minimize state changes
- Use texture atlases to reduce texture switches
- Implement frustum culling
- Use instanced rendering for repeated objects

### Audio Optimization
- Use audio worklets for low-latency processing
- Implement audio streaming for large files
- Pool audio sources for dynamic sounds
- Use spatial audio efficiently

### SIMD Optimization
- Vectorize math operations
- Use SIMD for image processing
- Optimize physics calculations
- Batch vector operations

## Browser Compatibility

### Minimum Requirements
- **Chrome**: 91+ (full support)
- **Firefox**: 89+ (full support)
- **Safari**: 14.1+ (limited threading)
- **Edge**: 91+ (full support)

### Feature Detection
```javascript
const capabilities = app.getBrowserCapabilities();
console.log('WebGL2 Support:', capabilities.webgl2Support);
console.log('SIMD Support:', capabilities.simdSupport);
console.log('Threading Support:', capabilities.threadsSupport);
```

## Deployment

### Production Deployment

1. **Build for production**:
   ```bash
   python3 tools/build.py build --target release
   ```

2. **Enable compression**:
   - Serve `.wasm.gz` files with `Content-Encoding: gzip`
   - Use Brotli compression for even better compression ratios

3. **Configure headers**:
   ```
   Cross-Origin-Embedder-Policy: require-corp
   Cross-Origin-Opener-Policy: same-origin
   ```

4. **Service Worker**:
   - Cache WASM and JS files for offline usage
   - Implement update mechanisms

### CDN Deployment
```html
<!-- Use CDN for faster loading -->
<script src="https://cdn.ecscope.dev/v1.0.0/ecscope.min.js"></script>
```

## Troubleshooting

### Common Issues

**WASM module fails to load**:
- Check MIME type configuration on server
- Ensure proper CORS headers
- Verify file integrity

**Audio context suspended**:
- Audio requires user interaction to start
- Call `audio.resumeContext()` after user gesture

**Threading not working**:
- Requires proper headers for SharedArrayBuffer
- Check browser support

**Performance issues**:
- Use performance profiler to identify bottlenecks
- Check memory allocation patterns
- Optimize draw calls and state changes

### Debug Mode
```bash
# Build with debug symbols
python3 tools/build.py build --target debug

# Enable verbose logging
python3 tools/build.py build --target debug --verbose
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests: `python3 tools/build.py test`
5. Submit a pull request

## License

MIT License - see LICENSE file for details.

## Support

- **Documentation**: https://docs.ecscope.dev
- **Issues**: https://github.com/ecscope/ecscope/issues
- **Discord**: https://discord.gg/ecscope
- **Email**: support@ecscope.dev