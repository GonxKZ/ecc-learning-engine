# WebAssembly Build Configuration for ECScope Engine
cmake_minimum_required(VERSION 3.20)

# Set Emscripten toolchain
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This CMakeLists.txt is specifically for Emscripten builds. Please use emcmake cmake.")
endif()

# Project configuration
project(ECScope-WebAssembly VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
option(ECSCOPE_WEB_DEBUG "Enable debug build for WebAssembly" OFF)
option(ECSCOPE_WEB_SIMD "Enable SIMD optimizations" ON)
option(ECSCOPE_WEB_THREADS "Enable threading support" ON)
option(ECSCOPE_WEB_WEBGL2 "Enable WebGL2 support" ON)
option(ECSCOPE_WEB_WEBAUDIO "Enable Web Audio API support" ON)

# Emscripten-specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".js")

# Include main ECScope project
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ecscope-core)

# WebAssembly-specific source files
set(ECSCOPE_WEB_SOURCES
    src/web_application.cpp
    src/web_renderer.cpp
    src/web_audio.cpp
    src/web_input.cpp
    src/web_filesystem.cpp
    src/web_networking.cpp
    src/web_memory.cpp
    src/bindings.cpp
)

# WebAssembly-specific headers
set(ECSCOPE_WEB_HEADERS
    include/web_application.hpp
    include/web_renderer.hpp
    include/web_audio.hpp
    include/web_input.hpp
    include/web_filesystem.hpp
    include/web_networking.hpp
    include/web_memory.hpp
    include/web_bindings.hpp
    include/web_types.hpp
)

# Create WebAssembly library
add_executable(ecscope-web ${ECSCOPE_WEB_SOURCES} ${ECSCOPE_WEB_HEADERS})

# Include directories
target_include_directories(ecscope-web PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link ECScope core libraries
target_link_libraries(ecscope-web PRIVATE
    ecscope-core
    ecscope-ecs
    ecscope-physics
    ecscope-rendering
    ecscope-memory
)

# Emscripten compiler flags
set(EMSCRIPTEN_COMMON_FLAGS
    "-sUSE_WEBGL2=1"
    "-sUSE_GLFW=3"
    "-sFULL_ES3=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=67108864"  # 64MB initial memory
    "-sMAXIMUM_MEMORY=2147483648"  # 2GB maximum memory
    "-sSTACK_SIZE=5242880"  # 5MB stack
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','addOnPreMain','addOnInit','addOnPreRun','addOnPostRun','FS','ENV']"
    "-sEXPORT_NAME='ECScope'"
    "-sMODULARIZE=1"
    "-sENVIRONMENT=web,worker"
)

# SIMD optimizations
if(ECSCOPE_WEB_SIMD)
    list(APPEND EMSCRIPTEN_COMMON_FLAGS
        "-msimd128"
        "-sSIMD=1"
    )
endif()

# Threading support
if(ECSCOPE_WEB_THREADS)
    list(APPEND EMSCRIPTEN_COMMON_FLAGS
        "-pthread"
        "-sPTHREAD_POOL_SIZE=4"
        "-sUSE_PTHREADS=1"
        "-sSHARED_MEMORY=1"
    )
endif()

# Debug vs Release configuration
if(ECSCOPE_WEB_DEBUG)
    list(APPEND EMSCRIPTEN_COMMON_FLAGS
        "-g4"
        "-gsource-map"
        "-sASSERTIONS=2"
        "-sSAFE_HEAP=1"
        "-sSTACK_OVERFLOW_CHECK=1"
        "-sDEMANGLE_SUPPORT=1"
        "-O1"
    )
else()
    list(APPEND EMSCRIPTEN_COMMON_FLAGS
        "-O3"
        "-sASSERTIONS=0"
        "-DNDEBUG"
        "--closure=1"
        "-flto"
    )
endif()

# Apply Emscripten flags
target_compile_options(ecscope-web PRIVATE ${EMSCRIPTEN_COMMON_FLAGS})
target_link_options(ecscope-web PRIVATE ${EMSCRIPTEN_COMMON_FLAGS})

# Embind bindings
target_link_options(ecscope-web PRIVATE
    "--bind"
    "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/src/pre.js"
    "--post-js=${CMAKE_CURRENT_SOURCE_DIR}/src/post.js"
    "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/assets/shell.html"
)

# Copy assets and generate additional files
add_custom_command(TARGET ecscope-web POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/ecscope-web.js
        ${CMAKE_CURRENT_SOURCE_DIR}/dist/ecscope.js
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/ecscope-web.wasm
        ${CMAKE_CURRENT_SOURCE_DIR}/dist/ecscope.wasm
    COMMENT "Copying WebAssembly build artifacts"
)

# Development server target
add_custom_target(serve
    COMMAND python3 -m http.server 8080
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist
    COMMENT "Starting development server on http://localhost:8080"
)

# Performance analysis target
add_custom_target(analyze
    COMMAND emrun --browser firefox --port 8080 ${CMAKE_CURRENT_BINARY_DIR}/ecscope-web.html
    DEPENDS ecscope-web
    COMMENT "Running performance analysis"
)