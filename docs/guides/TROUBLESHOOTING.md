# ECScope Troubleshooting Guide

**Comprehensive troubleshooting guide for common issues, debugging techniques, and optimization problems**

## Table of Contents

1. [Quick Diagnostic Checklist](#quick-diagnostic-checklist)
2. [Build and Compilation Issues](#build-and-compilation-issues)
3. [Runtime Problems](#runtime-problems)
4. [Performance Issues](#performance-issues)
5. [Graphics and Rendering Problems](#graphics-and-rendering-problems)
6. [Memory-Related Issues](#memory-related-issues)
7. [Physics System Problems](#physics-system-problems)
8. [Platform-Specific Issues](#platform-specific-issues)
9. [Educational Feature Problems](#educational-feature-problems)

## Quick Diagnostic Checklist

### Before Diving Deep

When encountering issues with ECScope, start with this quick checklist:

1. **Check Build Configuration**:
   ```bash
   # Verify configuration
   cd build && cmake -L .. | grep ECSCOPE
   
   # Expected output:
   # ECSCOPE_BUILD_EXAMPLES:BOOL=ON
   # ECSCOPE_ENABLE_GRAPHICS:BOOL=ON
   # ECSCOPE_ENABLE_INSTRUMENTATION:BOOL=ON
   ```

2. **Verify Dependencies**:
   ```bash
   # Check SDL2 (for graphics builds)
   pkg-config --modversion sdl2
   
   # Check OpenGL support
   glxinfo | grep "OpenGL version"  # Linux
   ```

3. **Test Console Version First**:
   ```bash
   # Always test console version to isolate graphics issues
   ./ecscope_performance_lab_console
   ```

4. **Check Log Output**:
   ```bash
   # Run with verbose logging
   ECSCOPE_LOG_LEVEL=DEBUG ./ecscope_ui
   ```

5. **Verify File Permissions**:
   ```bash
   # Check if executables have proper permissions
   ls -la ecscope_*
   ```

## Build and Compilation Issues

### C++20 Compiler Support Issues

**Problem**: Compiler doesn't support C++20 features used in ECScope.

**Symptoms**:
```
error: 'concept' does not name a type
error: 'requires' was not declared in this scope
error: 'std::format' is not a member of 'std'
```

**Solutions**:

1. **Update Compiler**:
   ```bash
   # Ubuntu/Debian
   sudo apt install gcc-11 g++-11
   export CXX=g++-11
   
   # macOS
   brew install llvm
   export CXX=/usr/local/opt/llvm/bin/clang++
   
   # Check compiler version
   $CXX --version
   ```

2. **Force C++20 Standard**:
   ```bash
   cmake -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON ..
   ```

3. **Alternative Compiler Configuration**:
   ```cmake
   # In CMakeLists.txt, add version checks
   if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
       if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
           message(FATAL_ERROR "GCC 10.0 or higher required for C++20 support")
       endif()
   elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
       if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0")
           message(FATAL_ERROR "Clang 12.0 or higher required for C++20 support")
       endif()
   endif()
   ```

### CMake Configuration Problems

**Problem**: CMake can't find dependencies or configure properly.

**Common Error Messages**:
```
CMake Error: Could not find SDL2
CMake Error: OpenGL not found
Target "ecscope" links to item "SDL2::SDL2" which has missing files
```

**Solutions**:

1. **SDL2 Detection Issues**:
   ```bash
   # Method 1: Use pkg-config
   export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH
   
   # Method 2: Specify SDL2 path manually
   cmake -DSDL2_DIR=/usr/lib/x86_64-linux-gnu/cmake/SDL2 ..
   
   # Method 3: Install development packages
   sudo apt install libsdl2-dev  # Ubuntu/Debian
   brew install sdl2             # macOS
   vcpkg install sdl2:x64-windows # Windows
   ```

2. **OpenGL Issues**:
   ```bash
   # Install OpenGL development headers
   sudo apt install libgl1-mesa-dev libglu1-mesa-dev  # Ubuntu/Debian
   
   # Verify OpenGL drivers
   glxinfo | grep "direct rendering"  # Should say "yes"
   ```

3. **ImGui Issues**:
   ```bash
   # Check if ImGui submodule is initialized
   git submodule update --init --recursive
   
   # Verify ImGui directory
   ls -la external/imgui/imgui.h
   ```

### Link-Time Errors

**Problem**: Undefined symbols during linking.

**Common Errors**:
```
undefined reference to `SDL_CreateWindow'
undefined reference to `glGenBuffers'
undefined reference to `ecscope::Registry::create_entity()'
```

**Solutions**:

1. **Missing Library Links**:
   ```cmake
   # Ensure proper library linking in CMakeLists.txt
   target_link_libraries(ecscope PUBLIC
       SDL2::SDL2
       ${OPENGL_LIBRARIES}
       pthread  # For Linux
   )
   ```

2. **Static vs Dynamic Linking**:
   ```bash
   # Check what libraries are linked
   ldd ./ecscope_ui  # Linux
   otool -L ./ecscope_ui  # macOS
   
   # For static linking issues
   cmake -DBUILD_SHARED_LIBS=OFF ..
   ```

3. **Template Instantiation Issues**:
   ```cpp
   // Ensure template specializations are properly instantiated
   // Add explicit instantiations in .cpp files if needed
   template class ecscope::Registry;
   template class ecscope::BasicView<Transform, Velocity>;
   ```

## Runtime Problems

### Application Crashes on Startup

**Problem**: ECScope crashes immediately when launched.

**Diagnostic Steps**:

1. **Run with Debugger**:
   ```bash
   gdb ./ecscope_ui
   (gdb) run
   (gdb) bt  # Print backtrace when crashed
   ```

2. **Check Dependencies**:
   ```bash
   # Linux: Check shared library dependencies
   ldd ./ecscope_ui
   
   # Look for "not found" entries
   # Install missing libraries
   ```

3. **Verify Graphics Support**:
   ```bash
   # Test OpenGL context creation
   glxgears  # Should display spinning gears
   
   # Check graphics drivers
   glxinfo | grep "OpenGL vendor"
   ```

**Common Solutions**:

1. **Missing Graphics Drivers**:
   ```bash
   # NVIDIA
   sudo apt install nvidia-driver-470
   
   # AMD
   sudo apt install mesa-vulkan-drivers
   
   # Intel
   sudo apt install intel-media-va-driver
   ```

2. **Virtual Machine Issues**:
   ```bash
   # Enable 3D acceleration in VM settings
   # Or build console-only version
   cmake -DECSCOPE_ENABLE_GRAPHICS=OFF ..
   ```

### Memory Access Violations

**Problem**: Segmentation faults or access violations.

**Diagnostic Approach**:

1. **Use AddressSanitizer**:
   ```bash
   # Build with ASan
   cmake -DECSCOPE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug ..
   make
   
   # Run and get detailed error report
   ./ecscope_ui
   ```

2. **Use Valgrind** (Linux):
   ```bash
   # Memory error detection
   valgrind --tool=memcheck --leak-check=full ./ecscope_ui
   
   # Look for:
   # - Invalid read/write operations
   # - Use of uninitialized values
   # - Memory leaks
   ```

3. **Check EntityID Validity**:
   ```cpp
   // Always validate entities before use
   if (!registry.is_entity_valid(entity)) {
       log_error("Invalid entity: {}", entity.value());
       return;
   }
   
   auto* component = registry.get_component<Transform>(entity);
   if (!component) {
       log_error("Entity {} doesn't have Transform component", entity.value());
       return;
   }
   ```

### Performance Degradation

**Problem**: ECScope runs slower than expected performance benchmarks.

**Diagnostic Steps**:

1. **Verify Build Configuration**:
   ```bash
   # Check if built in debug mode
   cmake -L .. | grep CMAKE_BUILD_TYPE
   
   # Should be "Release" for performance testing
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```

2. **Check Instrumentation Overhead**:
   ```bash
   # Disable instrumentation for pure performance
   cmake -DECSCOPE_ENABLE_INSTRUMENTATION=OFF ..
   ```

3. **Profile the Application**:
   ```bash
   # Linux profiling
   perf record ./ecscope_performance_lab_console
   perf report
   
   # Look for hot functions and optimization opportunities
   ```

**Performance Optimization Steps**:

1. **Disable Educational Features**:
   ```cpp
   // In production/benchmarking builds
   registry.enable_detailed_logging(false);
   registry.enable_performance_tracking(false);
   physics_world.enable_step_by_step_debugging(false);
   ```

2. **Optimize Allocator Configuration**:
   ```cpp
   // Use performance-optimized configuration
   auto config = AllocatorConfig::create_performance_optimized();
   Registry registry{config};
   ```

3. **Check Hardware Limits**:
   ```bash
   # Check CPU thermal throttling
   sensors  # Linux
   
   # Check memory pressure
   free -h
   
   # Check GPU utilization
   nvidia-smi  # NVIDIA
   radeontop   # AMD
   ```

## Graphics and Rendering Problems

### OpenGL Context Creation Fails

**Problem**: Cannot create OpenGL context or graphics initialization fails.

**Error Messages**:
```
Failed to create OpenGL context: [error message]
OpenGL version 3.3 not supported
GLAD failed to initialize
```

**Solutions**:

1. **Check OpenGL Support**:
   ```bash
   # Verify OpenGL version
   glxinfo | grep "OpenGL core profile version"
   
   # Should show 3.3 or higher
   ```

2. **Update Graphics Drivers**:
   ```bash
   # Ubuntu/Debian - NVIDIA
   sudo apt install nvidia-driver-latest
   
   # Ubuntu/Debian - AMD
   sudo apt install mesa-vulkan-drivers
   
   # Check driver version
   nvidia-smi  # NVIDIA
   glxinfo | grep "OpenGL vendor"  # All vendors
   ```

3. **Virtual Machine Configuration**:
   ```bash
   # Enable 3D acceleration in VM settings
   # Or use software rendering (slower)
   export LIBGL_ALWAYS_SOFTWARE=1
   ./ecscope_ui
   ```

4. **Headless Server Setup**:
   ```bash
   # Use Xvfb for headless testing
   Xvfb :99 -screen 0 1024x768x24 &
   export DISPLAY=:99
   ./ecscope_ui
   ```

### Rendering Performance Issues

**Problem**: Low frame rates or stuttering graphics.

**Diagnostic Approach**:

1. **Check GPU Utilization**:
   ```bash
   # Monitor GPU usage while running
   nvidia-smi -l 1  # NVIDIA (update every second)
   radeontop        # AMD
   ```

2. **Analyze Render Statistics**:
   ```cpp
   // In your application
   const auto render_stats = renderer.get_frame_statistics();
   log_info("Draw calls: {}, Batches: {}, GPU time: {:.2f}ms", 
           render_stats.total_draw_calls, render_stats.total_batches, render_stats.gpu_time_ms);
   
   if (render_stats.total_draw_calls > 100) {
       log_warning("High draw call count - enable batching optimization");
   }
   ```

3. **Check Batch Efficiency**:
   ```cpp
   const auto batch_info = renderer.get_batch_information();
   for (const auto& batch : batch_info) {
       if (batch.utilization_percentage < 50.0f) {
           log_warning("Low batch utilization: {:.1f}%", batch.utilization_percentage);
       }
   }
   ```

**Optimization Solutions**:

1. **Enable Automatic Batching**:
   ```cpp
   Renderer2D::Config config{};
   config.enable_batching = true;
   config.max_quads_per_batch = 1000;
   Renderer2D renderer{config};
   ```

2. **Optimize Texture Usage**:
   ```cpp
   // Use texture atlasing
   TextureAtlas atlas{1024, 1024};
   atlas.add_texture("sprite1", sprite1_data, 64, 64);
   atlas.add_texture("sprite2", sprite2_data, 32, 32);
   ```

3. **Reduce Overdraw**:
   ```cpp
   // Sort sprites by depth
   sprite_component.sort_order = static_cast<i32>(transform.position.y * -100.0f);
   ```

### Shader Compilation Errors

**Problem**: Shaders fail to compile or link.

**Error Analysis**:
```cpp
// Check shader compilation in debug mode
if (!shader.compile_from_source(vertex_src, fragment_src)) {
    // Detailed error logging is automatically enabled in debug builds
    log_error("Shader compilation failed - check console for details");
}
```

**Common Shader Issues**:

1. **GLSL Version Mismatch**:
   ```glsl
   // Ensure correct version for OpenGL 3.3
   #version 330 core
   
   // Not supported in 3.3:
   // #version 460 core  // Too new
   // #version 120       // Too old
   ```

2. **Attribute Location Conflicts**:
   ```glsl
   // Use explicit attribute locations
   layout (location = 0) in vec2 a_position;
   layout (location = 1) in vec2 a_tex_coords;
   layout (location = 2) in vec4 a_color;
   ```

3. **Uniform Name Mismatches**:
   ```cpp
   // Ensure uniform names match between C++ and GLSL
   shader.set_matrix4("u_view_projection", view_projection); // C++
   // uniform mat4 u_view_projection;  // GLSL - must match exactly
   ```

## Performance Issues

### Slow Physics Simulation

**Problem**: Physics system running slower than expected benchmarks.

**Diagnostic Steps**:

1. **Check Physics Configuration**:
   ```cpp
   auto physics_stats = physics_world.get_statistics();
   log_info("Physics performance analysis:");
   log_info("  Bodies: {}", physics_stats.active_rigidbodies);
   log_info("  Collision pairs: {}", physics_stats.collision_pairs_tested);
   log_info("  Constraints: {}", physics_stats.active_constraints);
   log_info("  Physics time: {:.3f}ms", physics_stats.total_physics_time_ms);
   
   // Target: < 5ms for 1000 bodies at 60 FPS
   if (physics_stats.total_physics_time_ms > 16.67f) {
       log_warning("Physics exceeding frame budget!");
   }
   ```

2. **Analyze Bottlenecks**:
   ```cpp
   if (physics_stats.broad_phase_time_ms > physics_stats.narrow_phase_time_ms) {
       log_info("Bottleneck: Broad-phase collision detection");
       log_info("Solution: Optimize spatial partitioning or reduce collision pairs");
   } else {
       log_info("Bottleneck: Narrow-phase collision detection or constraint solving");
       log_info("Solution: Optimize collision algorithms or reduce constraint iterations");
   }
   ```

**Optimization Solutions**:

1. **Spatial Partitioning Tuning**:
   ```cpp
   // Optimize spatial hash grid cell size
   f32 optimal_cell_size = calculate_optimal_cell_size(average_object_size);
   SpatialHashGrid spatial_grid{optimal_cell_size};
   
   // Rule of thumb: cell_size = 2 * average_object_radius
   ```

2. **Constraint Solver Tuning**:
   ```cpp
   PhysicsWorld::Config config{};
   config.velocity_iterations = 6;  // Reduce from 8 for performance
   config.position_iterations = 2;  // Reduce from 3
   
   // Monitor constraint convergence
   if (physics_stats.solver_convergence_ratio < 0.9f) {
       log_warning("Poor constraint convergence - consider increasing iterations");
   }
   ```

3. **Body Sleeping Optimization**:
   ```cpp
   // Enable sleeping for inactive bodies
   rigidbody.enable_sleeping = true;
   
   // Adjust sleep thresholds
   physics_world.set_sleep_linear_threshold(0.1f);   // m/s
   physics_world.set_sleep_angular_threshold(0.05f); // rad/s
   ```

### Memory Performance Issues

**Problem**: High memory usage or slow allocation performance.

**Diagnostic Tools**:

1. **Memory Usage Analysis**:
   ```cpp
   auto memory_stats = registry.get_allocator_statistics();
   for (const auto& [allocator_name, stats] : memory_stats) {
       log_info("{} allocator:", allocator_name);
       log_info("  Allocated: {:.2f}MB", stats.bytes_allocated / (1024.0f * 1024.0f));
       log_info("  Used: {:.2f}MB", stats.bytes_used / (1024.0f * 1024.0f));
       log_info("  Efficiency: {:.1f}%", (stats.bytes_used / stats.bytes_allocated) * 100.0f);
   }
   ```

2. **Allocation Pattern Analysis**:
   ```cpp
   MemoryTracker& tracker = get_global_memory_tracker();
   auto allocation_pattern = tracker.get_allocation_size_distribution();
   
   for (const auto& [size, count] : allocation_pattern) {
       log_info("Size {} bytes: {} allocations", size, count);
   }
   ```

**Optimization Solutions**:

1. **Configure Appropriate Allocators**:
   ```cpp
   AllocatorConfig config{};
   
   // For many small components
   config.enable_pool_allocator = true;
   config.pool_block_sizes = {16, 32, 64, 128};
   
   // For large component arrays
   config.enable_archetype_arena = true;
   config.arena_size_mb = 32;
   
   // For dynamic containers
   config.enable_pmr_containers = true;
   ```

2. **Reduce Memory Fragmentation**:
   ```cpp
   // Use arena allocators for short-lived allocations
   ArenaAllocator frame_arena{1 * MB};
   
   // Reset at end of frame
   frame_arena.reset();
   ```

3. **Optimize Component Sizes**:
   ```cpp
   // Pack components efficiently
   struct OptimizedComponent {
       f32 position_x, position_y;  // 8 bytes
       u16 flags;                   // 2 bytes (instead of multiple bools)
       u8 layer;                    // 1 byte
       u8 padding;                  // 1 byte (explicit padding)
   };                               // Total: 12 bytes (vs potential 16+ with poor packing)
   ```

### ECS Performance Problems

**Problem**: ECS operations slower than expected.

**Common Issues and Solutions**:

1. **Slow Component Queries**:
   ```cpp
   // Problem: Random entity access
   for (EntityID entity : random_entity_list) {
       auto* transform = registry.get_component<Transform>(entity);
       // This pattern causes cache misses
   }
   
   // Solution: Use archetype-aware queries
   auto view = registry.view<Transform, Velocity>();
   for (auto [entity, transform, velocity] : view.each()) {
       // This pattern is cache-friendly
   }
   ```

2. **Excessive Archetype Transitions**:
   ```cpp
   // Problem: Frequent component add/remove
   if (should_add_physics) {
       registry.add_component<RigidBody>(entity, RigidBody{});  // Archetype transition
   }
   if (should_remove_physics) {
       registry.remove_component<RigidBody>(entity);           // Another transition
   }
   
   // Solution: Use component flags instead
   auto* rigidbody = registry.get_component<RigidBody>(entity);
   rigidbody->is_static = !should_add_physics;  // No archetype change
   ```

3. **Inefficient System Ordering**:
   ```cpp
   // Problem: Systems with poor cache locality
   registry.add_system<RenderSystem>();    // Accesses Transform, Sprite
   registry.add_system<PhysicsSystem>();   // Accesses Transform, RigidBody
   registry.add_system<MovementSystem>();  // Accesses Transform, Velocity
   
   // Solution: Group systems by component access patterns
   registry.add_system<MovementSystem>();  // Transform + Velocity
   registry.add_system<PhysicsSystem>();   // Transform + RigidBody (after movement)
   registry.add_system<RenderSystem>();    // Transform + Sprite (after physics)
   ```

## Memory-Related Issues

### Memory Leaks

**Problem**: Memory usage continuously increases over time.

**Detection**:

1. **Built-in Leak Detection**:
   ```cpp
   // Enable leak detection in educational builds
   registry.configure_allocators(AllocatorConfig::create_educational());
   
   // Check for leaks periodically
   MemoryTracker& tracker = get_global_memory_tracker();
   auto potential_leaks = tracker.find_potential_leaks(30.0f); // 30 second threshold
   
   if (!potential_leaks.empty()) {
       log_warning("Detected {} potential memory leaks", potential_leaks.size());
       for (const auto& leak : potential_leaks) {
           log_warning("  Leak: {} bytes at {} (age: {:.1f}s)", 
                      leak.size, leak.address, leak.lifetime_seconds);
       }
   }
   ```

2. **Valgrind Leak Check**:
   ```bash
   valgrind --leak-check=full --show-leak-kinds=all ./ecscope_ui
   
   # Look for:
   # "definitely lost" - definite leaks
   # "possibly lost" - potential leaks
   # "still reachable" - allocated but not freed (usually OK)
   ```

**Common Leak Causes and Fixes**:

1. **Entity Cleanup Issues**:
   ```cpp
   // Problem: Forgetting to destroy entities
   std::vector<EntityID> temporary_entities;
   
   // Solution: RAII or explicit cleanup
   class EntityManager {
       std::vector<EntityID> managed_entities_;
       Registry& registry_;
       
   public:
       ~EntityManager() {
           for (EntityID entity : managed_entities_) {
               registry_.destroy_entity(entity);
           }
       }
   };
   ```

2. **System Resource Leaks**:
   ```cpp
   // Problem: Systems not cleaning up resources
   class LeakySystem : public System {
       std::vector<Resource> resources_;  // Never cleaned up
   };
   
   // Solution: Implement proper cleanup
   class ProperSystem : public System {
       std::vector<Resource> resources_;
       
   public:
       ~ProperSystem() override {
           cleanup_resources();
       }
       
       void cleanup_resources() {
           for (auto& resource : resources_) {
               resource.release();
           }
           resources_.clear();
       }
   };
   ```

### Memory Corruption

**Problem**: Random crashes or data corruption.

**Detection Techniques**:

1. **AddressSanitizer**:
   ```bash
   # Build with ASan for detailed corruption detection
   cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" ..
   make
   ./ecscope_ui
   
   # ASan will pinpoint exact location of corruption
   ```

2. **Memory Guards**:
   ```cpp
   // Enable memory guards in educational builds
   AllocatorConfig config{};
   config.enable_allocation_guards = true;  // Detect buffer overruns
   config.enable_canary_values = true;      // Detect memory stomping
   ```

**Common Corruption Causes**:

1. **Buffer Overruns**:
   ```cpp
   // Problem: Writing past array bounds
   std::array<f32, 4> data;
   data[4] = 1.0f;  // Out of bounds!
   
   // Solution: Use bounds-checked access
   data.at(4) = 1.0f;  // Throws exception instead of corrupting memory
   ```

2. **Use-After-Free**:
   ```cpp
   // Problem: Using deleted entity components
   EntityID entity = registry.create_entity();
   auto* transform = registry.get_component<Transform>(entity);
   registry.destroy_entity(entity);
   transform->position = Vec2{1, 2};  // Use after free!
   
   // Solution: Validate entity before component access
   if (registry.is_entity_valid(entity)) {
       auto* transform = registry.get_component<Transform>(entity);
       if (transform) {
           transform->position = Vec2{1, 2};
       }
   }
   ```

## Physics System Problems

### Physics Instability

**Problem**: Objects behave erratically or simulation explodes.

**Common Causes and Solutions**:

1. **Time Step Too Large**:
   ```cpp
   // Problem: Variable or large time steps
   physics_world.update(registry, large_delta_time);  // > 1/30 second
   
   // Solution: Fixed time step with accumulation
   PhysicsWorld::Config config{};
   config.time_step = 1.0f / 60.0f;  // Fixed 60 Hz
   
   // Use time accumulation for smooth rendering
   ```

2. **Insufficient Constraint Iterations**:
   ```cpp
   // Check constraint convergence
   auto physics_stats = physics_world.get_statistics();
   if (physics_stats.solver_convergence_ratio < 0.8f) {
       log_warning("Poor constraint convergence: {:.1f}%", 
                  physics_stats.solver_convergence_ratio * 100.0f);
       
       // Increase iterations
       PhysicsWorld::Config config{};
       config.velocity_iterations = 10;  // Increase from 8
       config.position_iterations = 4;   // Increase from 3
   }
   ```

3. **Inappropriate Mass Ratios**:
   ```cpp
   // Problem: Extreme mass differences
   RigidBody light_object{.mass = 0.1f};
   RigidBody heavy_object{.mass = 1000.0f};  // 10,000:1 ratio!
   
   // Solution: Keep mass ratios reasonable
   RigidBody light_object{.mass = 1.0f};
   RigidBody heavy_object{.mass = 10.0f};   // 10:1 ratio (much better)
   ```

### Collision Detection Issues

**Problem**: Collisions not detected or incorrectly detected.

**Debugging Steps**:

1. **Enable Visual Debugging**:
   ```cpp
   physics_world.enable_visual_debugging(true);
   
   // Visualize collision shapes, normals, and contact points
   debug_renderer.enable_collision_visualization(true);
   debug_renderer.enable_contact_point_visualization(true);
   ```

2. **Check Collision Layer Configuration**:
   ```cpp
   // Verify collision layers are set correctly
   collider.collision_layer = 0b0001;      // Layer 1
   collider.collision_mask = 0b0010;       // Only collide with layer 2
   
   // Debug layer interactions
   log_info("Collider layers: {} & {} = {}", 
           collider_a.collision_layer, collider_b.collision_mask,
           (collider_a.collision_layer & collider_b.collision_mask) != 0);
   ```

3. **Validate Collider Setup**:
   ```cpp
   // Check collider bounds
   auto aabb = collider.get_world_aabb(transform);
   log_info("Collider AABB: min({:.2f}, {:.2f}), max({:.2f}, {:.2f})", 
           aabb.min.x, aabb.min.y, aabb.max.x, aabb.max.y);
   
   // Verify collider isn't degenerate
   if (aabb.size().x <= 0.0f || aabb.size().y <= 0.0f) {
       log_error("Degenerate collider detected!");
   }
   ```

## Platform-Specific Issues

### Linux-Specific Problems

**X11/Wayland Display Issues**:
```bash
# Problem: Graphics not working on Wayland
export SDL_VIDEODRIVER=x11  # Force X11

# Problem: Missing display
export DISPLAY=:0  # Set display for X11

# Problem: Permission denied for GPU
sudo usermod -a -G video $USER
# Log out and back in
```

**Library Path Issues**:
```bash
# Problem: Library not found at runtime
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Or install system-wide
sudo ldconfig
```

### macOS-Specific Problems

**Framework Issues**:
```bash
# Problem: SDL2 framework not found
# Solution: Copy framework to system location
sudo cp -R SDL2.framework /Library/Frameworks/

# Or use Homebrew installation
brew install sdl2
```

**Code Signing Issues**:
```bash
# Problem: App won't run due to security
# Solution: Allow unsigned binaries
sudo spctl --master-disable

# Or sign the binary
codesign -s - ./ecscope_ui
```

### Windows-Specific Problems

**DLL Issues**:
```powershell
# Problem: DLL not found
# Copy required DLLs to executable directory
copy C:\vcpkg\installed\x64-windows\bin\SDL2.dll .

# Or add to PATH
set PATH=%PATH%;C:\vcpkg\installed\x64-windows\bin
```

**Visual Studio Debugging**:
```cpp
// Enable console output for debugging
#ifdef _WIN32
AllocConsole();
freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
#endif
```

## Educational Feature Problems

### UI Panels Not Displaying

**Problem**: ImGui panels don't appear or are corrupted.

**Solutions**:

1. **Check ImGui Initialization**:
   ```cpp
   // Verify ImGui context
   if (!ImGui::GetCurrentContext()) {
       log_error("ImGui context not initialized!");
       return;
   }
   
   // Check IO configuration
   ImGuiIO& io = ImGui::GetIO();
   log_info("ImGui configuration:");
   log_info("  Display size: {:.0f}x{:.0f}", io.DisplaySize.x, io.DisplaySize.y);
   log_info("  Font atlas built: {}", io.Fonts->IsBuilt());
   ```

2. **Reset Panel Layout**:
   ```cpp
   // Reset ImGui layout if corrupted
   ImGui::LoadIniSettingsFromDisk("imgui_default.ini");
   
   // Or delete settings file to reset completely
   // rm imgui.ini
   ```

3. **Check Panel Registration**:
   ```cpp
   // Ensure panels are properly registered
   PanelManager& panel_manager = get_panel_manager();
   panel_manager.add_panel<ECSInspectorPanel>();
   panel_manager.add_panel<MemoryAnalysisPanel>();
   panel_manager.add_panel<PhysicsDebugPanel>();
   ```

### Performance Data Not Collecting

**Problem**: Performance graphs are empty or show no data.

**Solutions**:

1. **Enable Performance Tracking**:
   ```cpp
   // Ensure instrumentation is enabled
   registry.enable_performance_tracking(true);
   
   // Check if build supports instrumentation
   #ifdef ECSCOPE_ENABLE_INSTRUMENTATION
   log_info("Performance instrumentation available");
   #else
   log_warning("Performance instrumentation disabled in this build");
   #endif
   ```

2. **Verify Profiler Setup**:
   ```cpp
   PerformanceProfiler& profiler = get_global_profiler();
   
   // Check if profiler is active
   if (!profiler.is_active()) {
       profiler.start_monitoring(registry);
   }
   
   // Verify scope timers are being used
   {
       PROFILE_SCOPE("Test Scope");
       // Some work here
   }
   
   auto scopes = profiler.get_frame_scopes();
   log_info("Captured {} profile scopes", scopes.size());
   ```

### Memory Laboratory Not Working

**Problem**: Memory experiments show incorrect or no results.

**Debugging Steps**:

1. **Check Memory Tracking**:
   ```cpp
   MemoryTracker& tracker = get_global_memory_tracker();
   
   // Verify tracking is enabled
   if (!tracker.is_tracking_enabled()) {
       log_error("Memory tracking is disabled!");
       enable_global_memory_tracking(true);
   }
   
   // Check tracking overhead
   f32 overhead = tracker.get_current_tracking_overhead();
   log_info("Memory tracking overhead: {:.2f}%", overhead);
   ```

2. **Verify Allocator Instrumentation**:
   ```cpp
   // Check if allocators are properly instrumented
   auto config = AllocatorConfig::create_educational();
   config.enable_detailed_instrumentation = true;
   registry.configure_allocators(config);
   ```

3. **Platform Memory API Issues**:
   ```bash
   # Linux: Check if process has permission for memory analysis
   echo 0 | sudo tee /proc/sys/kernel/kptr_restrict
   
   # Enable perf events for non-root
   echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid
   ```

## Advanced Troubleshooting

### Using System Profilers

**Linux (perf)**:
```bash
# CPU profiling
perf record -g ./ecscope_ui
perf report

# Memory profiling
perf record -e cache-misses,cache-references ./ecscope_ui
perf report

# System-wide analysis
sudo perf top
```

**macOS (Instruments)**:
```bash
# Launch with Instruments
instruments -t "Time Profiler" ./ecscope_ui

# Memory analysis
instruments -t "Allocations" ./ecscope_ui

# System trace
instruments -t "System Trace" ./ecscope_ui
```

**Windows (Visual Studio)**:
```cpp
// Enable Visual Studio diagnostics
#ifdef _WIN32
#include <crtdbg.h>

int main() {
    // Enable memory leak detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    
    // Your application code
    
    return 0;
}
#endif
```

### Debug Build Configuration

**Complete Debug Setup**:
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DECSCOPE_ENABLE_GRAPHICS=ON \
  -DECSCOPE_SANITIZERS=ON \
  -DECSCOPE_DETAILED_LOGGING=ON \
  -DECSCOPE_MEMORY_DEBUGGING=ON \
  -DECSCOPE_STATIC_ANALYSIS=ON \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer" \
  ..
```

### Logging Configuration

**Educational Logging Setup**:
```cpp
// Configure detailed logging for troubleshooting
LogConfig log_config{};
log_config.log_level = LogLevel::DEBUG;
log_config.enable_timestamps = true;
log_config.enable_thread_ids = true;
log_config.enable_source_location = true;
log_config.output_file = "ecscope_debug.log";

configure_logging(log_config);

// Use specific log categories
log_debug_ecs("Entity {} created with archetype {}", entity.value(), archetype_id);
log_debug_physics("Collision detected between entities {} and {}", entity_a.value(), entity_b.value());
log_debug_memory("Arena allocator used: {:.2f}MB of {:.2f}MB", used_mb, total_mb);
log_debug_renderer("Batch created with {} quads, {} textures", quad_count, texture_count);
```

### Performance Regression Detection

**Automated Performance Testing**:
```bash
#!/bin/bash
# regression_test.sh

echo "Running ECScope performance regression tests..."

# Build optimized version
cmake -DCMAKE_BUILD_TYPE=Release -DECSCOPE_ENABLE_GRAPHICS=OFF ..
make -j$(nproc)

# Run benchmark suite
./ecscope_performance_lab_console --benchmark --output=current_results.json

# Compare with baseline
python3 compare_performance.py baseline_results.json current_results.json

# Check for regressions
if [ $? -ne 0 ]; then
    echo "Performance regression detected!"
    exit 1
else
    echo "All performance tests passed"
    exit 0
fi
```

### Getting Help

**Community Resources**:
- **Documentation**: Check all files in `docs/` directory
- **Examples**: Review `examples/` for usage patterns
- **Issues**: Search existing GitHub issues
- **Discussions**: Use GitHub discussions for questions

**Reporting Issues**:

When reporting issues, include:

1. **Environment Information**:
   ```bash
   # System info
   uname -a
   lsb_release -a  # Linux
   
   # Compiler info
   $CXX --version
   
   # Build configuration
   cmake -L .. | grep ECSCOPE
   ```

2. **Minimal Reproduction Case**:
   ```cpp
   // Provide minimal code that reproduces the issue
   int main() {
       ecscope::Registry registry{};
       // Minimal steps to reproduce problem
       return 0;
   }
   ```

3. **Log Output**:
   ```bash
   # Include relevant log output
   ECSCOPE_LOG_LEVEL=DEBUG ./ecscope_ui 2>&1 | tee error_log.txt
   ```

4. **Performance Context**:
   ```bash
   # Include performance baseline
   ./ecscope_performance_lab_console --quick-benchmark
   ```

---

**ECScope Troubleshooting: Systematic problem solving, clear diagnostic steps, educational insight into system behavior.**

*Every problem is a learning opportunity. The troubleshooting guide helps understand not just how to fix issues, but why they occur and how to prevent them.*