#!/bin/bash

##############################################################################
# ECScope WebAssembly Build Script - Complete Production Build System
##############################################################################
# This script provides a complete WebAssembly build system for ECScope with:
# - Full Emscripten optimization and configuration
# - Complete asset pipeline and preprocessing
# - Advanced error handling and validation
# - Multiple build configurations and targets
# - Complete deployment package generation
# - Full integration with existing ECScope systems
##############################################################################

set -euo pipefail  # Strict error handling

# Script configuration
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$SCRIPT_DIR"
readonly BUILD_DIR="$PROJECT_ROOT/build-wasm"
readonly DIST_DIR="$PROJECT_ROOT/dist"
readonly WEB_DIR="$PROJECT_ROOT/web"
readonly SRC_DIR="$PROJECT_ROOT/src"

# Color output
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly PURPLE='\033[0;35m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m' # No Color

# Build configuration
BUILD_TYPE="Release"
ENABLE_DEBUG=false
ENABLE_PROFILING=false
ENABLE_OPTIMIZATION=true
ENABLE_CLOSURE=true
ENABLE_MINIFICATION=true
TARGET_PLATFORM="web"
MEMORY_SIZE="268435456"  # 256MB
MAX_MEMORY="1073741824"  # 1GB
STACK_SIZE="8388608"     # 8MB

# Feature flags
ENABLE_PHYSICS=true
ENABLE_GRAPHICS=true
ENABLE_AUDIO=true
ENABLE_NETWORKING=true
ENABLE_EDUCATION=true
ENABLE_PERFORMANCE_MONITORING=true

# Emscripten configuration
EMSCRIPTEN_OPTIMIZATION_LEVEL="-O3"
EMSCRIPTEN_LTO=true
EMSCRIPTEN_CLOSURE_LEVEL=1
WEBGL_VERSION=2
PTHREAD_POOL_SIZE=0  # Single-threaded for now

##############################################################################
# Utility Functions
##############################################################################

log_info() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_header() {
    echo -e "\n${PURPLE}=============================================================================${NC}"
    echo -e "${PURPLE} $1${NC}"
    echo -e "${PURPLE}=============================================================================${NC}\n"
}

check_command() {
    if ! command -v "$1" &> /dev/null; then
        log_error "Required command '$1' not found. Please install it and try again."
        exit 1
    fi
}

print_usage() {
    cat << EOF
ECScope WebAssembly Build Script

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -h, --help              Show this help message
    -d, --debug             Enable debug build
    -r, --release           Enable release build (default)
    -p, --profiling         Enable profiling build
    --no-optimization       Disable optimization
    --no-closure            Disable Closure compiler
    --no-minification       Disable code minification
    --memory-size SIZE      Set initial memory size in bytes (default: 268435456)
    --max-memory SIZE       Set maximum memory size in bytes (default: 1073741824)
    --clean                 Clean build directory before building
    --dist-only             Only create distribution package
    --serve                 Start development server after build

FEATURES:
    --disable-physics       Disable physics system
    --disable-graphics      Disable graphics system
    --disable-audio         Disable audio system
    --disable-networking    Disable networking system
    --disable-education     Disable educational features
    --disable-performance   Disable performance monitoring

EXAMPLES:
    $0                                  # Standard release build
    $0 --debug --profiling             # Debug build with profiling
    $0 --clean --no-closure            # Clean build without Closure
    $0 --memory-size 134217728         # Build with 128MB memory
    $0 --disable-physics --serve       # Build without physics, then serve

EOF
}

##############################################################################
# Command Line Argument Parsing
##############################################################################

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                print_usage
                exit 0
                ;;
            -d|--debug)
                BUILD_TYPE="Debug"
                ENABLE_DEBUG=true
                ENABLE_OPTIMIZATION=false
                EMSCRIPTEN_OPTIMIZATION_LEVEL="-O1"
                log_info "Debug build enabled"
                shift
                ;;
            -r|--release)
                BUILD_TYPE="Release"
                ENABLE_DEBUG=false
                ENABLE_OPTIMIZATION=true
                EMSCRIPTEN_OPTIMIZATION_LEVEL="-O3"
                log_info "Release build enabled"
                shift
                ;;
            -p|--profiling)
                ENABLE_PROFILING=true
                log_info "Profiling enabled"
                shift
                ;;
            --no-optimization)
                ENABLE_OPTIMIZATION=false
                EMSCRIPTEN_OPTIMIZATION_LEVEL="-O0"
                log_info "Optimization disabled"
                shift
                ;;
            --no-closure)
                ENABLE_CLOSURE=false
                log_info "Closure compiler disabled"
                shift
                ;;
            --no-minification)
                ENABLE_MINIFICATION=false
                log_info "Code minification disabled"
                shift
                ;;
            --memory-size)
                MEMORY_SIZE="$2"
                log_info "Initial memory size set to $MEMORY_SIZE bytes"
                shift 2
                ;;
            --max-memory)
                MAX_MEMORY="$2"
                log_info "Maximum memory size set to $MAX_MEMORY bytes"
                shift 2
                ;;
            --clean)
                log_info "Clean build requested"
                if [[ -d "$BUILD_DIR" ]]; then
                    rm -rf "$BUILD_DIR"
                    log_success "Build directory cleaned"
                fi
                shift
                ;;
            --dist-only)
                DIST_ONLY=true
                log_info "Distribution package only"
                shift
                ;;
            --serve)
                START_SERVER=true
                log_info "Development server will start after build"
                shift
                ;;
            --disable-physics)
                ENABLE_PHYSICS=false
                log_info "Physics system disabled"
                shift
                ;;
            --disable-graphics)
                ENABLE_GRAPHICS=false
                log_info "Graphics system disabled"
                shift
                ;;
            --disable-audio)
                ENABLE_AUDIO=false
                log_info "Audio system disabled"
                shift
                ;;
            --disable-networking)
                ENABLE_NETWORKING=false
                log_info "Networking system disabled"
                shift
                ;;
            --disable-education)
                ENABLE_EDUCATION=false
                log_info "Educational features disabled"
                shift
                ;;
            --disable-performance)
                ENABLE_PERFORMANCE_MONITORING=false
                log_info "Performance monitoring disabled"
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                print_usage
                exit 1
                ;;
        esac
    done
}

##############################################################################
# Environment Validation
##############################################################################

validate_environment() {
    log_header "Environment Validation"
    
    # Check for required commands
    check_command "emcmake"
    check_command "emmake"
    check_command "cmake"
    check_command "node"
    check_command "python3"
    
    # Verify Emscripten installation
    if ! emcc --version &> /dev/null; then
        log_error "Emscripten not properly installed or activated"
        log_info "Please run: source \$EMSDK/emsdk_env.sh"
        exit 1
    fi
    
    # Check Emscripten version
    local emcc_version
    emcc_version=$(emcc --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
    log_info "Emscripten version: $emcc_version"
    
    # Verify Node.js version for development tools
    local node_version
    node_version=$(node --version)
    log_info "Node.js version: $node_version"
    
    # Check available memory
    if command -v free &> /dev/null; then
        local available_memory
        available_memory=$(free -m | awk 'NR==2{printf "%.1fGB", $7/1024}')
        log_info "Available memory: $available_memory"
    fi
    
    # Validate project structure
    if [[ ! -f "$PROJECT_ROOT/CMakeLists_WebAssembly.txt" ]]; then
        log_error "CMakeLists_WebAssembly.txt not found"
        exit 1
    fi
    
    if [[ ! -d "$WEB_DIR" ]]; then
        log_error "Web directory not found: $WEB_DIR"
        exit 1
    fi
    
    if [[ ! -d "$SRC_DIR" ]]; then
        log_error "Source directory not found: $SRC_DIR"
        exit 1
    fi
    
    log_success "Environment validation completed"
}

##############################################################################
# Asset Pipeline
##############################################################################

process_web_assets() {
    log_header "Processing Web Assets"
    
    local assets_dir="$WEB_DIR/assets"
    local shaders_dir="$WEB_DIR/shaders"
    local tutorials_dir="$WEB_DIR/tutorials"
    
    # Create asset directories if they don't exist
    mkdir -p "$assets_dir" "$shaders_dir" "$tutorials_dir"
    
    # Generate placeholder assets for demo
    log_info "Generating demo assets..."
    
    # Create demo textures (using ImageMagick if available)
    if command -v convert &> /dev/null; then
        convert -size 64x64 xc:white "$assets_dir/white_texture.png"
        convert -size 64x64 gradient:blue-cyan "$assets_dir/gradient_texture.png"
        log_info "Generated demo textures"
    else
        log_warning "ImageMagick not found, skipping texture generation"
    fi
    
    # Copy shader files
    cat > "$shaders_dir/basic_vertex.glsl" << 'EOF'
#version 300 es
precision highp float;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texcoord;
layout(location = 2) in vec4 a_color;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

out vec2 v_texcoord;
out vec4 v_color;

void main() {
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 0.0, 1.0);
    v_texcoord = a_texcoord;
    v_color = a_color;
}
EOF
    
    cat > "$shaders_dir/basic_fragment.glsl" << 'EOF'
#version 300 es
precision highp float;

in vec2 v_texcoord;
in vec4 v_color;

uniform sampler2D u_texture;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(u_texture, v_texcoord);
    fragColor = texColor * v_color;
}
EOF
    
    cat > "$shaders_dir/particle_vertex.glsl" << 'EOF'
#version 300 es
precision highp float;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_velocity;
layout(location = 2) in float a_life;
layout(location = 3) in vec4 a_color;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform float u_time;
uniform float u_deltaTime;

out vec4 v_color;
out float v_life;

void main() {
    vec2 pos = a_position + a_velocity * u_deltaTime;
    gl_Position = u_projection * u_view * vec4(pos, 0.0, 1.0);
    v_color = a_color;
    v_life = a_life;
    gl_PointSize = max(1.0, 10.0 * v_life);
}
EOF
    
    cat > "$shaders_dir/particle_fragment.glsl" << 'EOF'
#version 300 es
precision highp float;

in vec4 v_color;
in float v_life;

out vec4 fragColor;

void main() {
    float alpha = v_life * (1.0 - length(gl_PointCoord - 0.5) * 2.0);
    fragColor = vec4(v_color.rgb, alpha * v_color.a);
}
EOF
    
    log_info "Generated shader files"
    
    # Create tutorial content
    cat > "$tutorials_dir/tutorial_manifest.json" << 'EOF'
{
  "tutorials": [
    {
      "id": "ecs-basics",
      "title": "ECS Basics",
      "category": "basics",
      "description": "Learn the fundamentals of Entity-Component-System architecture",
      "duration": 15,
      "difficulty": "beginner",
      "steps": [
        {
          "title": "What is ECS?",
          "content": "Entity-Component-System is an architectural pattern used in game development and simulation software.",
          "code": "// Create an entity\nauto entity = registry.create_entity();\n\n// Add components\nregistry.add_component(entity, Transform{0, 0});\nregistry.add_component(entity, Velocity{1, 0});"
        }
      ]
    },
    {
      "id": "memory-management",
      "title": "Memory Management",
      "category": "intermediate",
      "description": "Understand memory layout and cache efficiency in ECS",
      "duration": 20,
      "difficulty": "intermediate"
    },
    {
      "id": "performance-optimization",
      "title": "Performance Optimization",
      "category": "advanced",
      "description": "Advanced techniques for optimizing ECS performance",
      "duration": 30,
      "difficulty": "advanced"
    }
  ]
}
EOF
    
    log_success "Web assets processed successfully"
}

##############################################################################
# CMake Configuration
##############################################################################

configure_cmake() {
    log_header "CMake Configuration"
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Prepare CMake arguments
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
        "-DECSCOPE_WEB_ENABLE_PHYSICS=$ENABLE_PHYSICS"
        "-DECSCOPE_WEB_ENABLE_GRAPHICS=$ENABLE_GRAPHICS"
        "-DECSCOPE_WEB_ENABLE_AUDIO=$ENABLE_AUDIO"
        "-DECSCOPE_WEB_ENABLE_NETWORKING=$ENABLE_NETWORKING"
        "-DECSCOPE_WEB_ENABLE_EDUCATIONAL_FEATURES=$ENABLE_EDUCATION"
        "-DECSCOPE_WEB_ENABLE_PERFORMANCE_MONITORING=$ENABLE_PERFORMANCE_MONITORING"
    )
    
    # Add optimization flags
    if [[ "$ENABLE_OPTIMIZATION" == true ]]; then
        cmake_args+=("-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -flto")
    fi
    
    # Configure with emcmake
    log_info "Configuring CMake with Emscripten..."
    emcmake cmake "${cmake_args[@]}" -f "$PROJECT_ROOT/CMakeLists_WebAssembly.txt" "$PROJECT_ROOT"
    
    if [[ $? -ne 0 ]]; then
        log_error "CMake configuration failed"
        exit 1
    fi
    
    log_success "CMake configuration completed"
    cd "$PROJECT_ROOT"
}

##############################################################################
# Build Process
##############################################################################

build_webassembly() {
    log_header "Building WebAssembly"
    
    cd "$BUILD_DIR"
    
    # Build with emmake
    log_info "Building ECScope WebAssembly modules..."
    emmake make -j$(nproc) VERBOSE=1
    
    if [[ $? -ne 0 ]]; then
        log_error "WebAssembly build failed"
        exit 1
    fi
    
    # Verify output files
    if [[ ! -f "ecscope.js" ]]; then
        log_error "ecscope.js not generated"
        exit 1
    fi
    
    if [[ ! -f "ecscope.wasm" ]]; then
        log_error "ecscope.wasm not generated"
        exit 1
    fi
    
    log_success "WebAssembly build completed"
    
    # Display file sizes
    log_info "Generated files:"
    ls -lh ecscope.js ecscope.wasm *.data 2>/dev/null | while read -r line; do
        log_info "  $line"
    done
    
    cd "$PROJECT_ROOT"
}

##############################################################################
# Post-processing and Optimization
##############################################################################

optimize_output() {
    log_header "Post-processing and Optimization"
    
    cd "$BUILD_DIR"
    
    # Apply additional optimizations if enabled
    if [[ "$ENABLE_MINIFICATION" == true && "$BUILD_TYPE" == "Release" ]]; then
        log_info "Applying JavaScript minification..."
        
        # Use terser if available for additional JS optimization
        if command -v terser &> /dev/null; then
            terser ecscope.js --compress --mangle -o ecscope.min.js
            if [[ -f "ecscope.min.js" ]]; then
                mv ecscope.min.js ecscope.js
                log_info "JavaScript minification completed"
            fi
        else
            log_warning "Terser not found, skipping additional minification"
        fi
    fi
    
    # Generate source maps for debugging
    if [[ "$ENABLE_DEBUG" == true ]]; then
        log_info "Debug mode: source maps should be included"
    fi
    
    # Validate WebAssembly module
    if command -v wasm-validate &> /dev/null; then
        log_info "Validating WebAssembly module..."
        if wasm-validate ecscope.wasm; then
            log_success "WebAssembly module validation passed"
        else
            log_warning "WebAssembly module validation failed"
        fi
    fi
    
    cd "$PROJECT_ROOT"
}

##############################################################################
# Distribution Package Creation
##############################################################################

create_distribution() {
    log_header "Creating Distribution Package"
    
    # Clean and create distribution directory
    rm -rf "$DIST_DIR"
    mkdir -p "$DIST_DIR"
    
    # Copy WebAssembly files
    log_info "Copying WebAssembly files..."
    cp "$BUILD_DIR/ecscope.js" "$DIST_DIR/" 2>/dev/null || log_warning "ecscope.js not found"
    cp "$BUILD_DIR/ecscope.wasm" "$DIST_DIR/" 2>/dev/null || log_warning "ecscope.wasm not found"
    
    # Copy data files if they exist
    if [[ -f "$BUILD_DIR/ecscope.data" ]]; then
        cp "$BUILD_DIR/ecscope.data" "$DIST_DIR/"
    fi
    
    # Copy web assets
    log_info "Copying web assets..."
    cp -r "$WEB_DIR"/* "$DIST_DIR/"
    
    # Generate build info
    cat > "$DIST_DIR/build_info.json" << EOF
{
  "version": "1.0.0",
  "buildDate": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "buildType": "$BUILD_TYPE",
  "features": {
    "physics": $ENABLE_PHYSICS,
    "graphics": $ENABLE_GRAPHICS,
    "audio": $ENABLE_AUDIO,
    "networking": $ENABLE_NETWORKING,
    "education": $ENABLE_EDUCATION,
    "performanceMonitoring": $ENABLE_PERFORMANCE_MONITORING
  },
  "optimization": {
    "enabled": $ENABLE_OPTIMIZATION,
    "level": "$EMSCRIPTEN_OPTIMIZATION_LEVEL",
    "closure": $ENABLE_CLOSURE,
    "minification": $ENABLE_MINIFICATION
  },
  "memory": {
    "initialSize": $MEMORY_SIZE,
    "maximumSize": $MAX_MEMORY,
    "stackSize": $STACK_SIZE
  }
}
EOF
    
    # Generate deployment script
    cat > "$DIST_DIR/deploy.sh" << 'EOF'
#!/bin/bash
# ECScope WebAssembly Deployment Script

set -euo pipefail

DEPLOY_DIR="${1:-./deploy}"
PORT="${2:-8080}"

echo "Deploying ECScope WebAssembly to: $DEPLOY_DIR"

# Create deployment directory
mkdir -p "$DEPLOY_DIR"

# Copy all files
cp -r ./* "$DEPLOY_DIR/"

echo "Deployment completed successfully!"
echo "To serve locally, run: python3 -m http.server $PORT --directory $DEPLOY_DIR"
echo "Then open: http://localhost:$PORT"
EOF
    
    chmod +x "$DIST_DIR/deploy.sh"
    
    # Create README
    cat > "$DIST_DIR/README.md" << 'EOF'
# ECScope WebAssembly Distribution

This package contains the complete ECScope Educational ECS Engine compiled for WebAssembly.

## Quick Start

1. Serve the files with any HTTP server:
   ```bash
   python3 -m http.server 8080
   ```
   
2. Open your browser to `http://localhost:8080`

## Deployment

Use the included deployment script:
```bash
./deploy.sh /path/to/web/server/directory
```

## Features

- Complete ECS system with WebAssembly performance
- Interactive tutorials and educational content
- Real-time performance monitoring
- WebGL 2.0 graphics rendering
- Physics simulation (if enabled)
- Audio system (if enabled)
- Networking capabilities (if enabled)

## Browser Requirements

- Modern browser with WebAssembly support
- WebGL 2.0 support
- JavaScript ES6+ support

## File Overview

- `ecscope.js` - Main WebAssembly loader and JavaScript interface
- `ecscope.wasm` - Compiled WebAssembly module
- `ecscope.data` - Preloaded assets (if present)
- `index.html` - Main application entry point
- `css/` - Stylesheets and theming
- `js/` - JavaScript modules and utilities
- `assets/` - Images, textures, and other assets
- `shaders/` - WebGL shaders
- `tutorials/` - Educational content and tutorials

EOF
    
    log_success "Distribution package created: $DIST_DIR"
    
    # Display package info
    local total_size
    total_size=$(du -sh "$DIST_DIR" | cut -f1)
    log_info "Distribution package size: $total_size"
    
    # List main files
    log_info "Main files:"
    ls -lh "$DIST_DIR"/*.{js,wasm,html} 2>/dev/null | while read -r line; do
        log_info "  $line"
    done || log_info "No main files found yet"
}

##############################################################################
# Development Server
##############################################################################

start_development_server() {
    log_header "Starting Development Server"
    
    local port=8080
    local server_dir="$DIST_DIR"
    
    # Check if port is available
    if netstat -tuln 2>/dev/null | grep -q ":$port "; then
        log_warning "Port $port is already in use"
        port=8081
        log_info "Using alternative port: $port"
    fi
    
    log_info "Starting HTTP server on port $port"
    log_info "Server directory: $server_dir"
    log_info "URL: http://localhost:$port"
    
    # Start Python HTTP server
    cd "$server_dir"
    python3 -m http.server "$port" &
    local server_pid=$!
    
    # Wait a moment for server to start
    sleep 2
    
    # Try to open browser (if available)
    if command -v xdg-open &> /dev/null; then
        xdg-open "http://localhost:$port" &
    elif command -v open &> /dev/null; then
        open "http://localhost:$port" &
    fi
    
    log_success "Development server started (PID: $server_pid)"
    log_info "Press Ctrl+C to stop the server"
    
    # Wait for interrupt
    trap 'kill $server_pid 2>/dev/null; log_info "Development server stopped"; exit 0' INT
    wait "$server_pid"
}

##############################################################################
# Build Summary and Statistics
##############################################################################

show_build_summary() {
    log_header "Build Summary"
    
    local build_time_end
    build_time_end=$(date +%s)
    local build_duration=$((build_time_end - build_time_start))
    
    log_info "Build Configuration:"
    log_info "  Type: $BUILD_TYPE"
    log_info "  Optimization: $ENABLE_OPTIMIZATION ($EMSCRIPTEN_OPTIMIZATION_LEVEL)"
    log_info "  Closure: $ENABLE_CLOSURE"
    log_info "  Minification: $ENABLE_MINIFICATION"
    log_info "  Debug: $ENABLE_DEBUG"
    log_info "  Profiling: $ENABLE_PROFILING"
    
    log_info "Features Enabled:"
    log_info "  Physics: $ENABLE_PHYSICS"
    log_info "  Graphics: $ENABLE_GRAPHICS"
    log_info "  Audio: $ENABLE_AUDIO"
    log_info "  Networking: $ENABLE_NETWORKING"
    log_info "  Education: $ENABLE_EDUCATION"
    log_info "  Performance Monitoring: $ENABLE_PERFORMANCE_MONITORING"
    
    log_info "Memory Configuration:"
    log_info "  Initial: $(numfmt --to=iec $MEMORY_SIZE 2>/dev/null || echo "$MEMORY_SIZE bytes")"
    log_info "  Maximum: $(numfmt --to=iec $MAX_MEMORY 2>/dev/null || echo "$MAX_MEMORY bytes")"
    log_info "  Stack: $(numfmt --to=iec $STACK_SIZE 2>/dev/null || echo "$STACK_SIZE bytes")"
    
    if [[ -f "$BUILD_DIR/ecscope.wasm" ]]; then
        local wasm_size
        wasm_size=$(stat -f%z "$BUILD_DIR/ecscope.wasm" 2>/dev/null || stat -c%s "$BUILD_DIR/ecscope.wasm")
        log_info "WebAssembly module size: $(numfmt --to=iec $wasm_size 2>/dev/null || echo "$wasm_size bytes")"
    fi
    
    if [[ -f "$BUILD_DIR/ecscope.js" ]]; then
        local js_size
        js_size=$(stat -f%z "$BUILD_DIR/ecscope.js" 2>/dev/null || stat -c%s "$BUILD_DIR/ecscope.js")
        log_info "JavaScript module size: $(numfmt --to=iec $js_size 2>/dev/null || echo "$js_size bytes")"
    fi
    
    log_info "Build Duration: ${build_duration}s"
    
    if [[ -d "$DIST_DIR" ]]; then
        local dist_size
        dist_size=$(du -sh "$DIST_DIR" | cut -f1)
        log_info "Distribution package size: $dist_size"
        log_info "Distribution location: $DIST_DIR"
    fi
    
    log_success "ECScope WebAssembly build completed successfully!"
    
    if [[ "${START_SERVER:-false}" == true ]]; then
        log_info "Starting development server..."
        start_development_server
    else
        log_info "To test the build, run: python3 -m http.server 8080 --directory $DIST_DIR"
        log_info "Then open: http://localhost:8080"
    fi
}

##############################################################################
# Error Handling
##############################################################################

handle_error() {
    local exit_code=$?
    log_error "Build failed with exit code: $exit_code"
    log_error "Check the log output above for details"
    
    # Cleanup on failure
    if [[ -d "$BUILD_DIR" && "$BUILD_DIR" != "$PROJECT_ROOT" ]]; then
        log_info "Cleaning up build directory..."
        # Uncomment to auto-cleanup on failure
        # rm -rf "$BUILD_DIR"
    fi
    
    exit $exit_code
}

##############################################################################
# Main Execution
##############################################################################

main() {
    # Record build start time
    readonly build_time_start=$(date +%s)
    
    # Set up error handling
    trap 'handle_error' ERR
    
    log_header "ECScope WebAssembly Build System"
    log_info "Starting build process..."
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Skip validation and building if only creating distribution
    if [[ "${DIST_ONLY:-false}" != true ]]; then
        # Validate environment
        validate_environment
        
        # Process web assets
        process_web_assets
        
        # Configure CMake
        configure_cmake
        
        # Build WebAssembly
        build_webassembly
        
        # Optimize output
        optimize_output
    fi
    
    # Create distribution package
    create_distribution
    
    # Show build summary
    show_build_summary
}

# Execute main function with all arguments
main "$@"