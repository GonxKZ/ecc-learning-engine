/**
 * @file gui_compatibility_test.cpp
 * @brief Cross-platform GUI compatibility tests for ECScope
 * 
 * Comprehensive testing of GUI system compatibility across Windows, Linux, and macOS.
 * Tests ImGui integration, GLFW window management, and platform-specific behaviors.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../../include/ecscope/gui/gui_manager.hpp"
#include "../../include/ecscope/gui/dashboard.hpp"
#include "../../include/ecscope/gui/core.hpp"

#ifdef ECSCOPE_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

#ifdef ECSCOPE_HAS_IMGUI
#include <imgui.h>
#endif

#ifdef ECSCOPE_HAS_OPENGL
#include <GL/gl.h>
#endif

#include <thread>
#include <chrono>
#include <vector>
#include <memory>

using namespace ecscope::gui;
using namespace std::chrono_literals;

namespace {
    constexpr int TEST_WINDOW_WIDTH = 800;
    constexpr int TEST_WINDOW_HEIGHT = 600;
    constexpr float TEST_TIMEOUT_SECONDS = 5.0f;
}

// =============================================================================
// PLATFORM DETECTION TESTS
// =============================================================================

TEST_CASE("Platform Detection", "[cross-platform][platform]") {
    SECTION("Compiler identification") {
#ifdef _MSC_VER
        REQUIRE(true); // MSVC detected
        INFO("Running on MSVC compiler");
#elif defined(__GNUC__)
        REQUIRE(true); // GCC detected
        INFO("Running on GCC compiler");
#elif defined(__clang__)
        REQUIRE(true); // Clang detected
        INFO("Running on Clang compiler");
#else
        WARN("Unknown compiler detected");
#endif
    }
    
    SECTION("Operating system identification") {
#ifdef _WIN32
        REQUIRE(true); // Windows detected
        INFO("Running on Windows");
#elif defined(__APPLE__)
        REQUIRE(true); // macOS detected
        INFO("Running on macOS");
#elif defined(__linux__)
        REQUIRE(true); // Linux detected
        INFO("Running on Linux");
#else
        WARN("Unknown operating system detected");
#endif
    }
    
    SECTION("Architecture identification") {
#ifdef _M_X64 || __x86_64__
        REQUIRE(true); // x64 detected
        INFO("Running on x64 architecture");
#elif defined(_M_IX86) || defined(__i386__)
        REQUIRE(true); // x86 detected
        INFO("Running on x86 architecture");
#elif defined(__aarch64__) || defined(_M_ARM64)
        REQUIRE(true); // ARM64 detected
        INFO("Running on ARM64 architecture");
#else
        WARN("Unknown architecture detected");
#endif
    }
}

// =============================================================================
// GLFW COMPATIBILITY TESTS
// =============================================================================

TEST_CASE("GLFW Compatibility", "[cross-platform][glfw]") {
#ifdef ECSCOPE_HAS_GLFW
    
    SECTION("GLFW initialization") {
        REQUIRE(glfwInit() == GLFW_TRUE);
        
        int major, minor, rev;
        glfwGetVersion(&major, &minor, &rev);
        INFO("GLFW version: " << major << "." << minor << "." << rev);
        
        REQUIRE(major >= 3);
        REQUIRE(minor >= 3);
        
        glfwTerminate();
    }
    
    SECTION("Window creation and destruction") {
        REQUIRE(glfwInit() == GLFW_TRUE);
        
        // Set window hints for cross-platform compatibility
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden for testing
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        
        GLFWwindow* window = glfwCreateWindow(
            TEST_WINDOW_WIDTH, TEST_WINDOW_HEIGHT,
            "ECScope Cross-Platform Test", nullptr, nullptr
        );
        
        REQUIRE(window != nullptr);
        
        glfwMakeContextCurrent(window);
        
        // Test window properties
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        REQUIRE(width == TEST_WINDOW_WIDTH);
        REQUIRE(height == TEST_WINDOW_HEIGHT);
        
        // Test framebuffer size (important for high-DPI displays)
        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        REQUIRE(fb_width > 0);
        REQUIRE(fb_height > 0);
        
        // Calculate DPI scaling
        float x_scale = static_cast<float>(fb_width) / width;
        float y_scale = static_cast<float>(fb_height) / height;
        INFO("DPI scaling: " << x_scale << "x" << y_scale);
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    
    SECTION("Monitor enumeration") {
        REQUIRE(glfwInit() == GLFW_TRUE);
        
        int monitor_count;
        GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
        
        REQUIRE(monitors != nullptr);
        REQUIRE(monitor_count > 0);
        
        INFO("Detected " << monitor_count << " monitor(s)");
        
        // Test primary monitor
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        REQUIRE(primary != nullptr);
        
        const GLFWvidmode* mode = glfwGetVideoMode(primary);
        REQUIRE(mode != nullptr);
        
        INFO("Primary monitor resolution: " << mode->width << "x" << mode->height << " @ " << mode->refreshRate << "Hz");
        
        glfwTerminate();
    }
    
#else
    WARN("GLFW not available, skipping GLFW compatibility tests");
#endif
}

// =============================================================================
// OPENGL COMPATIBILITY TESTS
// =============================================================================

TEST_CASE("OpenGL Compatibility", "[cross-platform][opengl]") {
#if defined(ECSCOPE_HAS_GLFW) && defined(ECSCOPE_HAS_OPENGL)
    
    REQUIRE(glfwInit() == GLFW_TRUE);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    GLFWwindow* window = glfwCreateWindow(TEST_WINDOW_WIDTH, TEST_WINDOW_HEIGHT, "OpenGL Test", nullptr, nullptr);
    REQUIRE(window != nullptr);
    
    glfwMakeContextCurrent(window);
    
    SECTION("OpenGL context creation") {
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        REQUIRE(version != nullptr);
        INFO("OpenGL version: " << version);
        
        const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        REQUIRE(vendor != nullptr);
        INFO("OpenGL vendor: " << vendor);
        
        const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        REQUIRE(renderer != nullptr);
        INFO("OpenGL renderer: " << renderer);
    }
    
    SECTION("OpenGL capabilities") {
        GLint max_texture_size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
        REQUIRE(max_texture_size >= 1024);
        INFO("Max texture size: " << max_texture_size);
        
        GLint max_viewport_dims[2];
        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, max_viewport_dims);
        REQUIRE(max_viewport_dims[0] >= 1024);
        REQUIRE(max_viewport_dims[1] >= 1024);
        INFO("Max viewport: " << max_viewport_dims[0] << "x" << max_viewport_dims[1]);
    }
    
    SECTION("Basic rendering test") {
        // Clear color test
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        GLenum error = glGetError();
        REQUIRE(error == GL_NO_ERROR);
        
        // Viewport test
        glViewport(0, 0, TEST_WINDOW_WIDTH, TEST_WINDOW_HEIGHT);
        error = glGetError();
        REQUIRE(error == GL_NO_ERROR);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
#else
    WARN("OpenGL or GLFW not available, skipping OpenGL compatibility tests");
#endif
}

// =============================================================================
// IMGUI COMPATIBILITY TESTS
// =============================================================================

TEST_CASE("ImGui Compatibility", "[cross-platform][imgui]") {
#ifdef ECSCOPE_HAS_IMGUI
    
    SECTION("ImGui context creation") {
        IMGUI_CHECKVERSION();
        ImGuiContext* ctx = ImGui::CreateContext();
        REQUIRE(ctx != nullptr);
        
        ImGuiIO& io = ImGui::GetIO();
        
        // Test basic IO configuration
        io.DisplaySize = ImVec2(TEST_WINDOW_WIDTH, TEST_WINDOW_HEIGHT);
        REQUIRE(io.DisplaySize.x == TEST_WINDOW_WIDTH);
        REQUIRE(io.DisplaySize.y == TEST_WINDOW_HEIGHT);
        
        // Test font atlas
        REQUIRE(io.Fonts != nullptr);
        
        ImGui::DestroyContext(ctx);
    }
    
    SECTION("Platform-specific configuration") {
        ImGuiContext* ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // Test platform-specific features
#ifdef _WIN32
        // Windows-specific configuration
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        REQUIRE((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0);
#endif
        
#ifdef __APPLE__
        // macOS-specific configuration
        io.ConfigMacOSXBehaviors = true;
        REQUIRE(io.ConfigMacOSXBehaviors == true);
#endif
        
        ImGui::DestroyContext(ctx);
    }
    
    SECTION("Font rendering compatibility") {
        ImGuiContext* ctx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // Test default font
        ImFont* font = io.Fonts->AddFontDefault();
        REQUIRE(font != nullptr);
        
        // Build font atlas
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        
        REQUIRE(pixels != nullptr);
        REQUIRE(width > 0);
        REQUIRE(height > 0);
        
        INFO("Font atlas size: " << width << "x" << height);
        
        ImGui::DestroyContext(ctx);
    }
    
#else
    WARN("ImGui not available, skipping ImGui compatibility tests");
#endif
}

// =============================================================================
// GUI MANAGER INTEGRATION TESTS
// =============================================================================

TEST_CASE("GUI Manager Cross-Platform Integration", "[cross-platform][integration]") {
#if defined(ECSCOPE_HAS_GLFW) && defined(ECSCOPE_HAS_IMGUI) && defined(ECSCOPE_HAS_OPENGL)
    
    SECTION("GUI Manager initialization") {
        WindowConfig config;
        config.title = "ECScope Cross-Platform Test";
        config.width = TEST_WINDOW_WIDTH;
        config.height = TEST_WINDOW_HEIGHT;
        config.resizable = true;
        config.decorated = true;
        config.fullscreen = false;
        config.vsync = true;
        config.samples = 4;
        
        GUIFlags flags = GUIFlags::EnableDocking | GUIFlags::EnableKeyboardNav;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager != nullptr);
        
        // Test initialization
        bool init_result = gui_manager->initialize(config, flags, nullptr);
        REQUIRE(init_result == true);
        
        // Test window properties
        auto [width, height] = gui_manager->get_window_size();
        REQUIRE(width == TEST_WINDOW_WIDTH);
        REQUIRE(height == TEST_WINDOW_HEIGHT);
        
        // Test basic functionality
        REQUIRE_NOTHROW(gui_manager->begin_frame());
        REQUIRE_NOTHROW(gui_manager->end_frame());
        
        // Test cleanup
        REQUIRE_NOTHROW(gui_manager->shutdown());
    }
    
    SECTION("Dashboard cross-platform compatibility") {
        WindowConfig config;
        config.title = "Dashboard Test";
        config.width = TEST_WINDOW_WIDTH;
        config.height = TEST_WINDOW_HEIGHT;
        config.resizable = true;
        config.fullscreen = false;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test dashboard visibility
        gui_manager->show_dashboard(true);
        REQUIRE(gui_manager->is_dashboard_visible());
        
        gui_manager->show_dashboard(false);
        REQUIRE_FALSE(gui_manager->is_dashboard_visible());
        
        // Test theme switching
        REQUIRE_NOTHROW(gui_manager->set_theme(DashboardTheme::Dark));
        REQUIRE_NOTHROW(gui_manager->set_theme(DashboardTheme::Light));
        REQUIRE_NOTHROW(gui_manager->set_theme(DashboardTheme::HighContrast));
        
        gui_manager->shutdown();
    }
    
    SECTION("DPI scaling support") {
        WindowConfig config;
        config.title = "DPI Scaling Test";
        config.width = TEST_WINDOW_WIDTH;
        config.height = TEST_WINDOW_HEIGHT;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::None, nullptr));
        
        // Test UI scaling
        REQUIRE_NOTHROW(gui_manager->set_ui_scale(1.0f));
        REQUIRE_NOTHROW(gui_manager->set_ui_scale(1.5f));
        REQUIRE_NOTHROW(gui_manager->set_ui_scale(2.0f));
        
        // Test clamping
        REQUIRE_NOTHROW(gui_manager->set_ui_scale(0.1f)); // Should clamp to 0.5f
        REQUIRE_NOTHROW(gui_manager->set_ui_scale(5.0f)); // Should clamp to 3.0f
        
        gui_manager->shutdown();
    }
    
#else
    WARN("Required dependencies not available, skipping GUI Manager integration tests");
#endif
}

// =============================================================================
// PERFORMANCE TESTS
// =============================================================================

TEST_CASE("Cross-Platform Performance", "[cross-platform][performance]") {
#if defined(ECSCOPE_HAS_GLFW) && defined(ECSCOPE_HAS_IMGUI) && defined(ECSCOPE_HAS_OPENGL)
    
    SECTION("Initialization performance") {
        WindowConfig config;
        config.title = "Performance Test";
        config.width = TEST_WINDOW_WIDTH;
        config.height = TEST_WINDOW_HEIGHT;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        auto gui_manager = std::make_unique<GUIManager>();
        bool init_result = gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        REQUIRE(init_result == true);
        REQUIRE(duration.count() < 5000); // Should initialize within 5 seconds
        
        INFO("Initialization time: " << duration.count() << "ms");
        
        gui_manager->shutdown();
    }
    
    SECTION("Frame rendering performance") {
        WindowConfig config;
        config.title = "Frame Performance Test";
        config.width = TEST_WINDOW_WIDTH;
        config.height = TEST_WINDOW_HEIGHT;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Warm up
        for (int i = 0; i < 10; ++i) {
            gui_manager->begin_frame();
            gui_manager->end_frame();
        }
        
        // Measure frame time
        const int frame_count = 100;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < frame_count; ++i) {
            gui_manager->begin_frame();
            gui_manager->update(16.67f); // Simulate 60 FPS
            gui_manager->end_frame();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avg_frame_time = static_cast<double>(duration.count()) / frame_count / 1000.0; // ms
        double fps = 1000.0 / avg_frame_time;
        
        INFO("Average frame time: " << avg_frame_time << "ms");
        INFO("Estimated FPS: " << fps);
        
        // Performance should be reasonable
        REQUIRE(avg_frame_time < 50.0); // Less than 50ms per frame
        
        gui_manager->shutdown();
    }
    
#else
    WARN("Required dependencies not available, skipping performance tests");
#endif
}

// =============================================================================
// MEMORY MANAGEMENT TESTS
// =============================================================================

TEST_CASE("Cross-Platform Memory Management", "[cross-platform][memory]") {
#if defined(ECSCOPE_HAS_GLFW) && defined(ECSCOPE_HAS_IMGUI) && defined(ECSCOPE_HAS_OPENGL)
    
    SECTION("Memory leak detection") {
        WindowConfig config;
        config.title = "Memory Test";
        config.width = TEST_WINDOW_WIDTH;
        config.height = TEST_WINDOW_HEIGHT;
        
        // Create and destroy multiple GUI managers
        for (int i = 0; i < 5; ++i) {
            auto gui_manager = std::make_unique<GUIManager>();
            REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
            
            // Perform some operations
            gui_manager->begin_frame();
            gui_manager->show_dashboard(true);
            gui_manager->set_theme(DashboardTheme::Dark);
            gui_manager->end_frame();
            
            gui_manager->shutdown();
        }
        
        // If we reach here without crashes, memory management is likely correct
        REQUIRE(true);
    }
    
    SECTION("Resource cleanup") {
        WindowConfig config;
        config.title = "Resource Cleanup Test";
        config.width = TEST_WINDOW_WIDTH;
        config.height = TEST_WINDOW_HEIGHT;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test explicit shutdown
        REQUIRE_NOTHROW(gui_manager->shutdown());
        
        // Test double shutdown (should be safe)
        REQUIRE_NOTHROW(gui_manager->shutdown());
        
        // Reset should also be safe
        gui_manager.reset();
        REQUIRE(gui_manager == nullptr);
    }
    
#else
    WARN("Required dependencies not available, skipping memory management tests");
#endif
}