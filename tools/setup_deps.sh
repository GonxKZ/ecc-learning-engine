#!/bin/bash

# ECScope Dependencies Setup Script
# This script helps install dependencies for the full graphical version

echo "ECScope Dependencies Setup"
echo "=========================="
echo

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Detected Linux system"
    
    # Check for package managers and install SDL2
    if command -v apt-get &> /dev/null; then
        echo "Using apt package manager"
        echo "Installing SDL2 development libraries..."
        sudo apt-get update
        sudo apt-get install -y libsdl2-dev libgl1-mesa-dev
    elif command -v yum &> /dev/null; then
        echo "Using yum package manager"
        echo "Installing SDL2 development libraries..."
        sudo yum install -y SDL2-devel mesa-libGL-devel
    elif command -v pacman &> /dev/null; then
        echo "Using pacman package manager"
        echo "Installing SDL2 development libraries..."
        sudo pacman -S sdl2 mesa
    else
        echo "Package manager not detected. Please install SDL2 development libraries manually."
        echo "For Ubuntu/Debian: sudo apt-get install libsdl2-dev libgl1-mesa-dev"
        echo "For CentOS/RHEL: sudo yum install SDL2-devel mesa-libGL-devel"
        echo "For Arch: sudo pacman -S sdl2 mesa"
    fi
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS system"
    if command -v brew &> /dev/null; then
        echo "Using Homebrew package manager"
        echo "Installing SDL2..."
        brew install sdl2
    else
        echo "Homebrew not found. Please install SDL2 manually or install Homebrew first."
        echo "Install Homebrew: /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        echo "Then run: brew install sdl2"
    fi
    
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    echo "Detected Windows system"
    echo "For Windows, we recommend using vcpkg:"
    echo "1. Install vcpkg: git clone https://github.com/Microsoft/vcpkg.git"
    echo "2. Run: .\\vcpkg\\bootstrap-vcpkg.bat"
    echo "3. Install SDL2: .\\vcpkg\\vcpkg.exe install sdl2:x64-windows"
    echo "4. Integrate with CMake: .\\vcpkg\\vcpkg.exe integrate install"
fi

echo
echo "Installing ImGui..."
if [ ! -d "external/imgui" ]; then
    echo "Cloning ImGui (docking branch) to external/imgui/"
    git clone --branch docking https://github.com/ocornut/imgui.git external/imgui
    echo "ImGui installed successfully!"
else
    echo "ImGui already exists in external/imgui/"
    echo "To update: cd external/imgui && git pull"
fi

echo
echo "Setup complete!"
echo
echo "To build with graphics support:"
echo "  Using CMake: cmake -DECSCOPE_ENABLE_GRAPHICS=ON build && make -C build"
echo "  Using Make:  make GRAPHICS=1"
echo
echo "To test the setup:"
echo "  make GRAPHICS=1 test-compile"