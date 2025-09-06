#!/bin/bash

# ECScope Interactive Documentation Launcher
# This script builds and serves the interactive documentation system

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCS_DIR="${PROJECT_ROOT}/docs/interactive"
BUILD_SCRIPT="${PROJECT_ROOT}/scripts/build_docs.py"

echo "🚀 ECScope Interactive Documentation Launcher"
echo "📁 Project root: ${PROJECT_ROOT}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}✅${NC} $1"
}

print_info() {
    echo -e "${BLUE}ℹ️${NC}  $1"
}

print_warning() {
    echo -e "${YELLOW}⚠️${NC}  $1"
}

print_error() {
    echo -e "${RED}❌${NC} $1"
}

# Check for required tools
check_dependencies() {
    print_info "Checking dependencies..."
    
    # Check Python
    if ! command -v python3 &> /dev/null; then
        print_error "Python 3 is required but not installed"
        exit 1
    fi
    
    # Check if we have a modern browser
    if ! command -v google-chrome &> /dev/null && ! command -v firefox &> /dev/null && ! command -v open &> /dev/null; then
        print_warning "No browser command found - you'll need to open the documentation manually"
    fi
    
    print_status "Dependencies check passed"
}

# Build documentation
build_docs() {
    print_info "Building interactive documentation..."
    
    # Check if build script exists
    if [ ! -f "$BUILD_SCRIPT" ]; then
        print_error "Build script not found at $BUILD_SCRIPT"
        exit 1
    fi
    
    # Run build script
    if python3 "$BUILD_SCRIPT" "$PROJECT_ROOT" "$@"; then
        print_status "Documentation built successfully"
    else
        print_error "Documentation build failed"
        exit 1
    fi
}

# Start development server
start_server() {
    local port="${1:-8080}"
    print_info "Starting documentation server on port $port..."
    
    cd "$DOCS_DIR"
    
    # Check if the port is available
    if lsof -Pi :$port -sTCP:LISTEN -t >/dev/null 2>&1; then
        print_warning "Port $port is already in use, trying next available port..."
        port=$((port + 1))
    fi
    
    # Start Python HTTP server
    print_status "Server starting at http://localhost:$port"
    print_info "Press Ctrl+C to stop the server"
    
    # Try to open browser automatically
    if command -v open &> /dev/null; then
        # macOS
        open "http://localhost:$port" 2>/dev/null &
    elif command -v xdg-open &> /dev/null; then
        # Linux
        xdg-open "http://localhost:$port" 2>/dev/null &
    elif command -v start &> /dev/null; then
        # Windows (Git Bash, etc.)
        start "http://localhost:$port" 2>/dev/null &
    fi
    
    # Start server (this will block)
    python3 -m http.server "$port" 2>/dev/null || {
        print_error "Failed to start server"
        exit 1
    }
}

# Show help
show_help() {
    cat << EOF
ECScope Interactive Documentation Launcher

USAGE:
    $0 [OPTIONS] [COMMAND]

COMMANDS:
    build       Build documentation only (don't start server)
    serve       Start server only (assume docs are built)
    clean       Clean all generated documentation files
    help        Show this help message

OPTIONS:
    --port PORT     Port for development server (default: 8080)
    --force         Force rebuild all documentation
    --api-only      Generate only API documentation
    --tutorials-only Generate only tutorials
    --no-browser    Don't open browser automatically

EXAMPLES:
    $0                          # Build docs and start server
    $0 build --force           # Force rebuild documentation
    $0 serve --port 9000       # Start server on port 9000
    $0 clean                   # Clean generated files
    
For more advanced options, use the build script directly:
    python3 scripts/build_docs.py --help

EOF
}

# Parse command line arguments
COMMAND=""
PORT="8080"
BUILD_ARGS=()
NO_BROWSER=false

while [[ $# -gt 0 ]]; do
    case $1 in
        build|serve|clean|help)
            COMMAND="$1"
            shift
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        --force|--api-only|--tutorials-only|--no-optimization|--no-validation|--development)
            BUILD_ARGS+=("$1")
            shift
            ;;
        --no-browser)
            NO_BROWSER=true
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
main() {
    case "$COMMAND" in
        "help")
            show_help
            ;;
        "clean")
            print_info "Cleaning generated documentation..."
            python3 "$BUILD_SCRIPT" "$PROJECT_ROOT" --clean
            print_status "Documentation cleaned"
            ;;
        "build")
            check_dependencies
            build_docs "${BUILD_ARGS[@]}"
            print_status "Documentation is ready!"
            print_info "To serve: cd $DOCS_DIR && python3 -m http.server $PORT"
            ;;
        "serve")
            if [ ! -d "$DOCS_DIR" ] || [ ! -f "$DOCS_DIR/index.html" ]; then
                print_error "Documentation not found. Please build first:"
                print_info "$0 build"
                exit 1
            fi
            start_server "$PORT"
            ;;
        *)
            # Default: build and serve
            check_dependencies
            build_docs "${BUILD_ARGS[@]}"
            print_status "Documentation built! Starting server..."
            sleep 1
            start_server "$PORT"
            ;;
    esac
}

# Handle Ctrl+C gracefully
trap 'echo -e "\n👋 Documentation server stopped"; exit 0' INT

# Run main function
main

echo "🎉 ECScope documentation session completed!"