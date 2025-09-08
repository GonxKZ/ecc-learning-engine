#!/bin/bash
##############################################################################
# ECScope WebAssembly Build Script - Clean and Simple
##############################################################################

set -euo pipefail

# Configuration
readonly PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly BUILD_DIR="$PROJECT_ROOT/build-wasm"
readonly DIST_DIR="$PROJECT_ROOT/dist"

# Colors
readonly GREEN='\033[0;32m'
readonly RED='\033[0;31m'
readonly YELLOW='\033[1;33m'
readonly NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_usage() {
    cat << EOF
ECScope WebAssembly Build Script

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -h, --help      Show this help message
    -d, --debug     Debug build
    -r, --release   Release build (default)
    --clean         Clean build directory
    --serve         Start development server after build

EXAMPLES:
    $0                      # Standard release build
    $0 --debug             # Debug build
    $0 --clean --serve     # Clean build then serve

EOF
}

# Parse arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
START_SERVER=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --serve)
            START_SERVER=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Check prerequisites
if ! command -v emcc &> /dev/null; then
    log_error "Emscripten not found. Please activate emsdk first:"
    log_error "source \$EMSDK/emsdk_env.sh"
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    log_error "CMake not found. Please install cmake."
    exit 1
fi

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    log_info "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create directories
mkdir -p "$BUILD_DIR"
mkdir -p "$DIST_DIR"

log_info "Building ECScope WebAssembly ($BUILD_TYPE)..."

cd "$BUILD_DIR"

# Configure with CMake
log_info "Configuring with CMake..."
emcmake cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DECSCOPE_BUILD_WASM=ON \
    "$PROJECT_ROOT"

if [[ $? -ne 0 ]]; then
    log_error "CMake configuration failed"
    exit 1
fi

# Build
log_info "Building..."
emmake make -j$(nproc)

if [[ $? -ne 0 ]]; then
    log_error "Build failed"
    exit 1
fi

# Copy output files
log_info "Copying output files to dist directory..."
for file in ecscope.js ecscope.wasm ecscope.data; do
    if [[ -f "$file" ]]; then
        cp "$file" "$DIST_DIR/"
        log_info "Copied $file"
    fi
done

# Copy web assets
if [[ -d "$PROJECT_ROOT/web" ]]; then
    cp -r "$PROJECT_ROOT/web"/* "$DIST_DIR/" 2>/dev/null || true
    log_info "Copied web assets"
fi

# Create simple HTML if it doesn't exist
if [[ ! -f "$DIST_DIR/index.html" ]]; then
    cat > "$DIST_DIR/index.html" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>ECScope WebAssembly</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        canvas { border: 1px solid #ccc; display: block; margin: 20px auto; }
        #output { background: #f0f0f0; padding: 10px; margin: 20px 0; font-family: monospace; }
    </style>
</head>
<body>
    <h1>ECScope WebAssembly Demo</h1>
    <canvas id="ecscope-canvas" width="800" height="600"></canvas>
    <div id="output"></div>
    
    <script>
        var Module = {
            preRun: [],
            postRun: [],
            print: function(text) {
                console.log(text);
                document.getElementById('output').innerHTML += text + '\n';
            },
            canvas: document.getElementById('ecscope-canvas')
        };
    </script>
    <script src="ecscope.js"></script>
</body>
</html>
EOF
    log_info "Created index.html"
fi

cd "$PROJECT_ROOT"

# Display results
log_info "Build completed successfully!"
if [[ -f "$DIST_DIR/ecscope.wasm" ]]; then
    WASM_SIZE=$(stat -c%s "$DIST_DIR/ecscope.wasm" 2>/dev/null || stat -f%z "$DIST_DIR/ecscope.wasm")
    log_info "WebAssembly module size: $(echo "$WASM_SIZE" | numfmt --to=iec)B"
fi

log_info "Output directory: $DIST_DIR"

# Start development server if requested
if [[ "$START_SERVER" == true ]]; then
    log_info "Starting development server..."
    cd "$DIST_DIR"
    python3 -m http.server 8080 &
    log_info "Server started at http://localhost:8080"
    log_info "Press Ctrl+C to stop"
    wait
fi