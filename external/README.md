# External Dependencies

This directory contains external dependencies for ECScope.

## Required Dependencies

### SDL2 (Simple DirectMedia Layer)
- **Purpose**: Window creation, input handling, and basic graphics context
- **Version**: 2.0.20 or later
- **License**: zlib license (permissive)

#### Installation Options:

**Ubuntu/Debian:**
```bash
sudo apt-get install libsdl2-dev
```

**Windows (vcpkg):**
```bash
vcpkg install sdl2
```

**Manual Installation:**
- Download from https://libsdl.org/
- Extract to `external/SDL2/`

### Dear ImGui
- **Purpose**: Real-time UI for observability panels
- **Version**: 1.89 or later (docking branch recommended)
- **License**: MIT license

#### Installation:
```bash
git clone --branch docking https://github.com/ocornut/imgui.git external/imgui
```

## Optional Dependencies

### OpenGL
- Usually provided by system/drivers
- Headers may need separate installation on some systems

## Build Configuration

The build system is configured to work with or without these dependencies:

- **Without dependencies**: Basic console application for ECS core development
- **With dependencies**: Full graphical application with observability UI

To enable graphical features, ensure dependencies are installed and use:
```bash
cmake -DECSCOPE_ENABLE_GRAPHICS=ON ..
# or 
make GRAPHICS=1
```

## Status

Current dependency status will be shown during build configuration.