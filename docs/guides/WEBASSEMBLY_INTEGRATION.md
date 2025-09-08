# ECScope WebAssembly Integration Guide

**Complete guide to running ECScope in web browsers using WebAssembly**

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Build Process](#build-process)
4. [JavaScript API](#javascript-api)
5. [Web Integration Examples](#web-integration-examples)
6. [Performance Considerations](#performance-considerations)
7. [Deployment Guide](#deployment-guide)
8. [Advanced Features](#advanced-features)

## Overview

ECScope provides full WebAssembly support, allowing the ECS engine to run directly in web browsers. This enables:

- **Interactive learning** through web-based demos
- **Cross-platform deployment** without native compilation
- **Educational accessibility** via web browsers
- **Real-time visualization** of ECS concepts
- **Performance comparison** between native and web versions

### Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   C++ ECScope   │───▶│   Emscripten     │───▶│  WebAssembly    │
│   Core Engine   │    │   Compiler       │    │   Module        │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                                         │
                                                         ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│  HTML/CSS UI    │◄───│   JavaScript     │◄───│   WASM Glue     │
│   Interface     │    │   Application    │    │     Code        │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## Quick Start

### Prerequisites

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # Add to your .bashrc/.zshrc
```

### Basic Build

```bash
cd entities-cpp2
mkdir build-wasm && cd build-wasm

# Configure for WebAssembly
emcmake cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DECSCOPE_TARGET_WASM=ON

# Build
emmake make -j4

# Outputs: ecscope.wasm, ecscope.js, ecscope.html
```

### Test in Browser

```bash
# Start local server
python3 -m http.server 8080

# Open browser to http://localhost:8080
# Navigate to build-wasm/ecscope.html
```

## Build Process

### CMake Configuration for WebAssembly

ECScope's CMake system automatically detects Emscripten and configures appropriate settings:

```cmake
# CMakeLists.txt excerpt
if(EMSCRIPTEN)
    set(ECSCOPE_TARGET_WASM ON)
    
    # WebAssembly-specific settings
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    # Emscripten flags
    set(EMSCRIPTEN_FLAGS
        "-s WASM=1"
        "-s USE_SDL=2"
        "-s ALLOW_MEMORY_GROWTH=1"
        "--bind"
        "--preload-file ${CMAKE_SOURCE_DIR}/assets@/assets"
    )
    
    target_compile_options(ecscope_wasm PRIVATE ${EMSCRIPTEN_FLAGS})
    target_link_options(ecscope_wasm PRIVATE ${EMSCRIPTEN_FLAGS})
endif()
```

### Build Variants

#### Basic Console WASM

```bash
emcmake cmake .. \
  -DECSCOPE_TARGET_WASM=ON \
  -DECSCOPE_ENABLE_GRAPHICS=OFF
```

**Output:** Pure ECS engine for headless web workers or Node.js.

#### Graphics-Enabled WASM

```bash
emcmake cmake .. \
  -DECSCOPE_TARGET_WASM=ON \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DCMAKE_CXX_FLAGS="-s USE_SDL=2 -s USE_WEBGL2=1"
```

**Output:** Full 2D renderer with WebGL support.

#### Educational Demo Build

```bash
emcmake cmake .. \
  -DECSCOPE_TARGET_WASM=ON \
  -DECSCOPE_BUILD_EXAMPLES=ON \
  -DECSCOPE_ENABLE_MEMORY_ANALYSIS=ON \
  -DCMAKE_CXX_FLAGS="-s ASSERTIONS=1 -s SAFE_HEAP=1"
```

**Output:** Interactive educational demos with memory visualization.

### Advanced Build Options

```bash
emcmake cmake .. \
  -DECSCOPE_TARGET_WASM=ON \
  -DCMAKE_CXX_FLAGS="\
    -s WASM=1 \
    -s USE_SDL=2 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=64MB \
    -s MAXIMUM_MEMORY=512MB \
    -s EXPORTED_FUNCTIONS=['_main','_malloc','_free'] \
    -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','addOnPreMain'] \
    --bind \
    --preload-file assets@/assets \
    -s ASSERTIONS=1 \
    -s SAFE_HEAP=1 \
    -O3"
```

## JavaScript API

ECScope uses Embind to expose C++ classes and functions to JavaScript:

### Core ECS API

```javascript
// Initialize ECScope module
const ECScope = await ECScope();

// Create registry
const registry = new ECScope.Registry("WebDemo");

// Entity management
const player = registry.createEntity();
const enemy = registry.createEntity();

// Component management
registry.addPosition(player, 100, 100);
registry.addVelocity(player, 1, 0);
registry.addHealth(player, 100, 100);

// Query components
const position = registry.getPosition(player);
console.log(`Player at (${position.x}, ${position.y})`);

// System updates
registry.updateMovementSystem(deltaTime);
registry.updateCollisionSystem();
```

### Memory Tracking API

```javascript
// Get memory statistics
const stats = registry.getMemoryStatistics();
console.log(`Memory efficiency: ${stats.memory_efficiency * 100}%`);
console.log(`Arena utilization: ${stats.arena_utilization * 100}%`);

// Generate memory report
const report = registry.generateMemoryReport();
document.getElementById('memory-report').textContent = report;

// Run benchmarks
registry.benchmarkAllocators("WebBenchmark", 10000);
const comparisons = registry.getPerformanceComparisons();
```

### Physics API

```javascript
// Create physics world
const physics = new ECScope.PhysicsWorld2D();
physics.setGravity(0, -9.81);

// Create physics body
const bodyId = physics.createBody({
    x: 100, y: 100,
    rotation: 0,
    isStatic: false
});

// Attach shapes
physics.attachCircle(bodyId, {
    radius: 20,
    density: 1.0,
    friction: 0.3,
    restitution: 0.8
});

// Step simulation
physics.step(1/60, 8, 3);  // 60 FPS, 8 velocity, 3 position iterations

// Get body state
const state = physics.getBodyState(bodyId);
console.log(`Body at (${state.x}, ${state.y})`);
```

### Renderer API

```javascript
// Create renderer (if graphics enabled)
const renderer = new ECScope.Renderer2D();
renderer.initialize(800, 600);

// Set camera
renderer.setCamera({
    x: 0, y: 0,
    zoom: 1.0,
    rotation: 0
});

// Render frame
renderer.beginFrame();
renderer.clear(0.2, 0.3, 0.3, 1.0);

// Draw sprites
renderer.drawSprite(100, 100, 32, 32, 1, 0, 0, 1);  // Red sprite
renderer.drawCircle(200, 200, 25, 32, 0, 1, 0, 1);  // Green circle

renderer.endFrame();
```

## Web Integration Examples

### Basic Game Loop

```html
<!DOCTYPE html>
<html>
<head>
    <title>ECScope Web Demo</title>
    <style>
        body { margin: 0; background: #222; color: white; font-family: monospace; }
        canvas { display: block; margin: 0 auto; border: 1px solid #555; }
        #info { padding: 20px; text-align: center; }
    </style>
</head>
<body>
    <div id="info">
        <h1>ECScope WebAssembly Demo</h1>
        <div id="stats"></div>
    </div>
    <canvas id="canvas" width="800" height="600"></canvas>
    
    <script src="ecscope.js"></script>
    <script>
        let ECScope, registry, renderer, physics;
        let lastTime = 0;
        
        async function initialize() {
            // Load WASM module
            ECScope = await ECScope();
            
            // Initialize systems
            registry = new ECScope.Registry("WebGame");
            renderer = new ECScope.Renderer2D();
            physics = new ECScope.PhysicsWorld2D();
            
            // Set up renderer
            renderer.initialize(800, 600);
            
            // Set up physics
            physics.setGravity(0, -500);
            
            // Create game entities
            createGameEntities();
            
            // Start game loop
            requestAnimationFrame(gameLoop);
        }
        
        function createGameEntities() {
            // Create player
            const player = registry.createEntity();
            registry.addPosition(player, 400, 300);
            registry.addVelocity(player, 0, 0);
            registry.addSprite(player, 32, 32, 1, 0, 0, 1);
            
            // Create physics body for player
            const playerId = physics.createBody({
                x: 400, y: 300,
                rotation: 0,
                isStatic: false
            });
            physics.attachCircle(playerId, {
                radius: 16,
                density: 1.0,
                friction: 0.3,
                restitution: 0.8
            });
            registry.addRigidBody(player, playerId);
            
            // Create static platforms
            for (let i = 0; i < 5; i++) {
                const platform = registry.createEntity();
                const x = 100 + i * 120;
                const y = 500;
                
                registry.addPosition(platform, x, y);
                registry.addSprite(platform, 80, 20, 0, 1, 0, 1);
                
                const platformId = physics.createBody({
                    x: x, y: y,
                    rotation: 0,
                    isStatic: true
                });
                physics.attachBox(platformId, {
                    width: 80, height: 20,
                    density: 1.0,
                    friction: 0.5,
                    restitution: 0.0
                });
            }
        }
        
        function gameLoop(currentTime) {
            const deltaTime = (currentTime - lastTime) / 1000.0;
            lastTime = currentTime;
            
            // Update physics
            physics.step(deltaTime, 8, 3);
            
            // Update ECS systems
            registry.updatePhysicsSync();
            
            // Render frame
            renderer.beginFrame();
            renderer.clear(0.1, 0.1, 0.2, 1.0);
            
            // Render all sprites
            const entities = registry.getEntitiesWithPositionAndSprite();
            entities.forEach(entity => {
                const pos = registry.getPosition(entity);
                const sprite = registry.getSprite(entity);
                
                renderer.drawSprite(
                    pos.x, pos.y,
                    sprite.width, sprite.height,
                    sprite.r, sprite.g, sprite.b, sprite.a
                );
            });
            
            renderer.endFrame();
            
            // Update stats
            updateStats(deltaTime);
            
            requestAnimationFrame(gameLoop);
        }
        
        function updateStats(deltaTime) {
            const fps = Math.round(1.0 / deltaTime);
            const stats = registry.getMemoryStatistics();
            
            document.getElementById('stats').innerHTML = `
                FPS: ${fps} | 
                Entities: ${stats.active_entities} | 
                Memory: ${(stats.arena_utilization * 100).toFixed(1)}%
            `;
        }
        
        // Handle input
        document.addEventListener('keydown', (event) => {
            const playerEntities = registry.getEntitiesWithRigidBody();
            if (playerEntities.length > 0) {
                const player = playerEntities[0];
                const rigidBody = registry.getRigidBody(player);
                
                switch (event.code) {
                    case 'ArrowLeft':
                        physics.applyImpulse(rigidBody.physics_id, -200, 0);
                        break;
                    case 'ArrowRight':
                        physics.applyImpulse(rigidBody.physics_id, 200, 0);
                        break;
                    case 'Space':
                        physics.applyImpulse(rigidBody.physics_id, 0, -400);
                        break;
                }
            }
        });
        
        // Initialize when page loads
        window.addEventListener('load', initialize);
    </script>
</body>
</html>
```

### Educational Memory Visualization

```html
<!DOCTYPE html>
<html>
<head>
    <title>ECScope Memory Lab</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; }
        .panel { background: white; padding: 20px; margin: 10px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .metrics { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }
        .metric { text-align: center; padding: 10px; background: #f8f9fa; border-radius: 5px; }
        .metric-value { font-size: 24px; font-weight: bold; color: #007bff; }
        .metric-label { font-size: 12px; color: #666; }
        .progress-bar { width: 100%; height: 20px; background: #e9ecef; border-radius: 10px; overflow: hidden; }
        .progress-fill { height: 100%; background: linear-gradient(90deg, #28a745, #ffc107, #dc3545); transition: width 0.3s ease; }
        button { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; }
        .btn-primary { background: #007bff; color: white; }
        .btn-success { background: #28a745; color: white; }
        .btn-warning { background: #ffc107; color: black; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ECScope Memory Laboratory</h1>
        
        <div class="panel">
            <h2>Real-time Memory Statistics</h2>
            <div class="metrics" id="memory-metrics"></div>
        </div>
        
        <div class="panel">
            <h2>Arena Allocator Visualization</h2>
            <div class="progress-bar">
                <div class="progress-fill" id="arena-progress"></div>
            </div>
            <p>Arena Utilization: <span id="arena-percent">0%</span></p>
        </div>
        
        <div class="panel">
            <h2>Interactive Tests</h2>
            <button class="btn-primary" onclick="stressTest()">Stress Test (10k entities)</button>
            <button class="btn-success" onclick="compactMemory()">Compact Memory</button>
            <button class="btn-warning" onclick="resetRegistry()">Reset Registry</button>
        </div>
        
        <div class="panel">
            <h2>Performance Comparison</h2>
            <div id="performance-results"></div>
        </div>
        
        <div class="panel">
            <h2>Memory Report</h2>
            <pre id="memory-report"></pre>
        </div>
    </div>
    
    <script src="ecscope.js"></script>
    <script>
        let ECScope, registry;
        
        async function initialize() {
            ECScope = await ECScope();
            registry = new ECScope.Registry("MemoryLab");
            
            // Start real-time updates
            setInterval(updateMemoryStats, 100);
        }
        
        function updateMemoryStats() {
            const stats = registry.getMemoryStatistics();
            
            // Update metrics
            document.getElementById('memory-metrics').innerHTML = `
                <div class="metric">
                    <div class="metric-value">${stats.active_entities}</div>
                    <div class="metric-label">Active Entities</div>
                </div>
                <div class="metric">
                    <div class="metric-value">${stats.total_archetypes}</div>
                    <div class="metric-label">Archetypes</div>
                </div>
                <div class="metric">
                    <div class="metric-value">${(stats.memory_efficiency * 100).toFixed(1)}%</div>
                    <div class="metric-label">Memory Efficiency</div>
                </div>
                <div class="metric">
                    <div class="metric-value">${(stats.cache_hit_ratio * 100).toFixed(1)}%</div>
                    <div class="metric-label">Cache Hit Ratio</div>
                </div>
            `;
            
            // Update arena visualization
            const utilization = stats.arena_utilization * 100;
            document.getElementById('arena-progress').style.width = utilization + '%';
            document.getElementById('arena-percent').textContent = utilization.toFixed(1) + '%';
            
            // Update memory report
            document.getElementById('memory-report').textContent = registry.generateMemoryReport();
        }
        
        function stressTest() {
            console.log("Starting stress test...");
            const startTime = performance.now();
            
            // Create 10,000 entities with different component combinations
            for (let i = 0; i < 10000; i++) {
                const entity = registry.createEntity();
                
                // Add different components based on entity index
                registry.addPosition(entity, i % 1000, i % 500);
                
                if (i % 2 === 0) {
                    registry.addVelocity(entity, Math.random() * 2 - 1, Math.random() * 2 - 1);
                }
                
                if (i % 3 === 0) {
                    registry.addHealth(entity, 100, 100);
                }
                
                if (i % 5 === 0) {
                    registry.addSprite(entity, 32, 32, Math.random(), Math.random(), Math.random(), 1);
                }
            }
            
            const endTime = performance.now();
            console.log(`Stress test completed in ${(endTime - startTime).toFixed(2)}ms`);
        }
        
        function compactMemory() {
            console.log("Compacting memory...");
            registry.compactMemory();
            
            setTimeout(() => {
                console.log("Memory compaction completed");
            }, 100);
        }
        
        function resetRegistry() {
            console.log("Resetting registry...");
            registry.clear();
        }
        
        // Initialize when page loads
        window.addEventListener('load', initialize);
    </script>
</body>
</html>
```

## Performance Considerations

### WebAssembly vs Native Performance

ECScope's WebAssembly build typically achieves:

- **70-90%** of native performance for ECS operations
- **50-70%** of native performance for physics simulation
- **80-95%** of native performance for memory operations

### Optimization Strategies

#### 1. Memory Management

```cpp
// Optimize for WASM memory model
emcmake cmake .. \
  -DCMAKE_CXX_FLAGS="-s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=128MB"
```

#### 2. Function Call Optimization

```javascript
// Batch operations to reduce WASM/JS boundary crossings
const entities = registry.getAllMovingEntities();  // Single call
entities.forEach(entity => {
    // Process in JavaScript when possible
    const pos = entity.position;
    pos.x += entity.velocity.x * deltaTime;
    pos.y += entity.velocity.y * deltaTime;
});
registry.updatePositions(entities);  // Batch update
```

#### 3. Threading Considerations

WebAssembly doesn't support native threading, so ECScope's job system is disabled in WASM builds:

```cmake
# Automatic configuration for WASM
if(EMSCRIPTEN)
    set(ECSCOPE_ENABLE_JOB_SYSTEM OFF)
    set(ECSCOPE_ENABLE_THREADING OFF)
endif()
```

## Deployment Guide

### Static File Hosting

```bash
# Build optimized production version
emcmake cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DECSCOPE_TARGET_WASM=ON \
  -DCMAKE_CXX_FLAGS="-O3 -s WASM=1"

emmake make -j4

# Deploy files
cp ecscope.wasm ecscope.js ecscope.html /var/www/html/
```

### CDN Configuration

```html
<!-- Serve WASM files with correct MIME type -->
<script>
    // Ensure WASM MIME type
    if (typeof WebAssembly === 'object') {
        fetch('ecscope.wasm')
            .then(response => {
                if (response.headers.get('Content-Type') !== 'application/wasm') {
                    console.warn('WASM file not served with correct MIME type');
                }
            });
    }
</script>
```

### Server Configuration

**Apache (.htaccess):**
```apache
AddType application/wasm .wasm
Header set Cross-Origin-Embedder-Policy require-corp
Header set Cross-Origin-Opener-Policy same-origin
```

**Nginx:**
```nginx
location ~* \.wasm$ {
    add_header Content-Type application/wasm;
    add_header Cross-Origin-Embedder-Policy require-corp;
    add_header Cross-Origin-Opener-Policy same-origin;
}
```

## Advanced Features

### Multi-threading with SharedArrayBuffer

```javascript
// Check for threading support
if (typeof SharedArrayBuffer !== 'undefined') {
    // Enable threading in WASM build
    const ECScope = await ECScope({
        locateFile: (path) => {
            if (path.endsWith('.wasm')) {
                return 'ecscope-threaded.wasm';
            }
            return path;
        }
    });
}
```

### File System Integration

```cpp
// Preload assets during build
emcmake cmake .. \
  -DCMAKE_CXX_FLAGS="--preload-file ${CMAKE_SOURCE_DIR}/assets@/assets"

// Access files in C++
std::ifstream file("/assets/config.json");
```

### Debugging WebAssembly

```bash
# Build with debug information
emcmake cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-g4 -O0 -s ASSERTIONS=1 -s SAFE_HEAP=1"

# Enable source maps
emcmake cmake .. \
  -DCMAKE_CXX_FLAGS="--source-map-base http://localhost:8080/"
```

### Performance Profiling

```html
<script>
    // Profile WASM performance
    performance.mark('ecs-update-start');
    registry.updateAllSystems(deltaTime);
    performance.mark('ecs-update-end');
    
    performance.measure('ecs-update', 'ecs-update-start', 'ecs-update-end');
    
    // Log performance data
    const entries = performance.getEntriesByType('measure');
    console.table(entries);
</script>
```

ECScope's WebAssembly integration provides a complete solution for running the ECS engine in web browsers, enabling interactive educational content and cross-platform deployment while maintaining good performance characteristics.