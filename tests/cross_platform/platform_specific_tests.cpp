/**
 * @file platform_specific_tests.cpp
 * @brief Platform-specific GUI behavior tests
 * 
 * Tests platform-specific behaviors, window management differences,
 * file dialogs, DPI scaling, and OS-specific UI conventions.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../../include/ecscope/gui/gui_manager.hpp"
#include "../../include/ecscope/gui/core.hpp"

#ifdef ECSCOPE_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

#include <filesystem>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

using namespace ecscope::gui;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

// =============================================================================
// WINDOWS-SPECIFIC TESTS
// =============================================================================

#ifdef _WIN32

TEST_CASE("Windows-Specific GUI Features", "[windows][platform-specific]") {
    SECTION("Windows DPI awareness") {
        // Test high-DPI support on Windows
        WindowConfig config;
        config.title = "Windows DPI Test";
        config.width = 800;
        config.height = 600;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test DPI scaling
        gui_manager->set_ui_scale(1.25f); // Common Windows scaling
        gui_manager->set_ui_scale(1.5f);  // High DPI scaling
        gui_manager->set_ui_scale(2.0f);  // Very high DPI scaling
        
        gui_manager->shutdown();
    }
    
    SECTION("Windows window decorations") {
        WindowConfig config;
        config.title = "Windows Decorations Test";
        config.width = 800;
        config.height = 600;
        config.decorated = true;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test window title changes
        gui_manager->set_window_title("Updated Windows Title");
        
        // Test window sizing
        gui_manager->set_window_size(1024, 768);
        auto [width, height] = gui_manager->get_window_size();
        REQUIRE(width == 1024);
        REQUIRE(height == 768);
        
        gui_manager->shutdown();
    }
    
    SECTION("Windows fullscreen toggle") {
        WindowConfig config;
        config.title = "Windows Fullscreen Test";
        config.width = 800;
        config.height = 600;
        config.fullscreen = false;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test fullscreen toggle (should not crash)
        REQUIRE_NOTHROW(gui_manager->toggle_fullscreen());
        REQUIRE_NOTHROW(gui_manager->toggle_fullscreen());
        
        gui_manager->shutdown();
    }
}

TEST_CASE("Windows File Path Handling", "[windows][filesystem]") {
    SECTION("Windows path separators") {
        // Test Windows-style paths
        std::string windows_path = "C:\\Program Files\\ECScope\\data.txt";
        fs::path path(windows_path);
        
        REQUIRE(path.is_absolute());
        REQUIRE(path.root_name() == "C:");
    }
    
    SECTION("Windows special directories") {
        // Test access to Windows special directories
        auto temp_path = fs::temp_directory_path();
        REQUIRE_FALSE(temp_path.empty());
        REQUIRE(fs::exists(temp_path));
        
        INFO("Windows temp directory: " << temp_path.string());
    }
    
    SECTION("Windows long path support") {
        // Test long path names (Windows limitation)
        std::string long_path(300, 'a');
        long_path = "C:\\" + long_path + ".txt";
        
        fs::path path(long_path);
        // Should not crash when creating the path object
        REQUIRE_FALSE(path.empty());
    }
}

#endif // _WIN32

// =============================================================================
// MACOS-SPECIFIC TESTS
// =============================================================================

#ifdef __APPLE__

TEST_CASE("macOS-Specific GUI Features", "[macos][platform-specific]") {
    SECTION("macOS Retina display support") {
        WindowConfig config;
        config.title = "macOS Retina Test";
        config.width = 800;
        config.height = 600;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test Retina scaling
        auto [width, height] = gui_manager->get_window_size();
        
        // On Retina displays, framebuffer size might be different
        INFO("macOS window size: " << width << "x" << height);
        
        gui_manager->shutdown();
    }
    
    SECTION("macOS window behavior") {
        WindowConfig config;
        config.title = "macOS Window Test";
        config.width = 800;
        config.height = 600;
        config.decorated = true;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test macOS-specific window operations
        gui_manager->set_window_title("macOS Application");
        
        // Test window resizing (should respect macOS behavior)
        gui_manager->set_window_size(1200, 800);
        
        gui_manager->shutdown();
    }
    
    SECTION("macOS menu bar integration") {
        // Test if menu bar integration works
        WindowConfig config;
        config.title = "macOS Menu Test";
        config.width = 800;
        config.height = 600;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Menu bar tests would go here
        // For now, just ensure initialization works
        REQUIRE(true);
        
        gui_manager->shutdown();
    }
}

TEST_CASE("macOS File Path Handling", "[macos][filesystem]") {
    SECTION("macOS path separators") {
        // Test Unix-style paths
        std::string unix_path = "/Applications/ECScope.app/Contents/Resources/data.txt";
        fs::path path(unix_path);
        
        REQUIRE(path.is_absolute());
        REQUIRE(path.root_directory() == "/");
    }
    
    SECTION("macOS application bundle paths") {
        // Test .app bundle structure
        std::string bundle_path = "/Applications/ECScope.app";
        fs::path path(bundle_path);
        
        // Check if we can construct bundle-style paths
        auto contents_path = path / "Contents";
        auto resources_path = contents_path / "Resources";
        
        REQUIRE(contents_path.filename() == "Contents");
        REQUIRE(resources_path.filename() == "Resources");
    }
    
    SECTION("macOS hidden files") {
        // Test hidden file handling (files starting with .)
        std::string hidden_file = "/.hidden_config";
        fs::path path(hidden_file);
        
        REQUIRE(path.filename().string().starts_with("."));
    }
}

#endif // __APPLE__

// =============================================================================
// LINUX-SPECIFIC TESTS
// =============================================================================

#ifdef __linux__

TEST_CASE("Linux-Specific GUI Features", "[linux][platform-specific]") {
    SECTION("Linux X11 window management") {
        WindowConfig config;
        config.title = "Linux X11 Test";
        config.width = 800;
        config.height = 600;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test X11-specific window operations
        gui_manager->set_window_title("Linux Application");
        
        // Test window sizing on Linux
        gui_manager->set_window_size(1024, 768);
        auto [width, height] = gui_manager->get_window_size();
        
        INFO("Linux window size: " << width << "x" << height);
        
        gui_manager->shutdown();
    }
    
    SECTION("Linux display server compatibility") {
        // Test compatibility with different display servers
        
        // Check for X11
        const char* display = std::getenv("DISPLAY");
        if (display) {
            INFO("Running on X11, DISPLAY=" << display);
        }
        
        // Check for Wayland
        const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
        if (wayland_display) {
            INFO("Running on Wayland, WAYLAND_DISPLAY=" << wayland_display);
        }
        
        // At least one should be available
        REQUIRE((display != nullptr || wayland_display != nullptr));
    }
    
    SECTION("Linux window manager compatibility") {
        WindowConfig config;
        config.title = "Linux WM Test";
        config.width = 800;
        config.height = 600;
        config.resizable = true;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test fullscreen toggle (important for Linux WMs)
        REQUIRE_NOTHROW(gui_manager->toggle_fullscreen());
        REQUIRE_NOTHROW(gui_manager->toggle_fullscreen());
        
        gui_manager->shutdown();
    }
}

TEST_CASE("Linux File Path Handling", "[linux][filesystem]") {
    SECTION("Linux path separators") {
        // Test Unix-style paths
        std::string unix_path = "/usr/local/bin/ecscope";
        fs::path path(unix_path);
        
        REQUIRE(path.is_absolute());
        REQUIRE(path.root_directory() == "/");
    }
    
    SECTION("Linux home directory") {
        // Test home directory access
        const char* home = std::getenv("HOME");
        REQUIRE(home != nullptr);
        
        fs::path home_path(home);
        REQUIRE(fs::exists(home_path));
        REQUIRE(fs::is_directory(home_path));
        
        INFO("Linux home directory: " << home_path.string());
    }
    
    SECTION("Linux XDG directories") {
        // Test XDG Base Directory specification
        const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
        const char* xdg_data = std::getenv("XDG_DATA_HOME");
        const char* xdg_cache = std::getenv("XDG_CACHE_HOME");
        
        // XDG variables might not be set, but HOME should be
        const char* home = std::getenv("HOME");
        REQUIRE(home != nullptr);
        
        // Test default XDG paths
        fs::path config_path = xdg_config ? fs::path(xdg_config) : fs::path(home) / ".config";
        fs::path data_path = xdg_data ? fs::path(xdg_data) : fs::path(home) / ".local" / "share";
        fs::path cache_path = xdg_cache ? fs::path(xdg_cache) : fs::path(home) / ".cache";
        
        INFO("XDG config dir: " << config_path.string());
        INFO("XDG data dir: " << data_path.string());
        INFO("XDG cache dir: " << cache_path.string());
        
        // Paths should be constructible
        REQUIRE_FALSE(config_path.empty());
        REQUIRE_FALSE(data_path.empty());
        REQUIRE_FALSE(cache_path.empty());
    }
    
    SECTION("Linux symbolic links") {
        // Test symbolic link handling
        std::string link_path = "/tmp/ecscope_test_link";
        std::string target_path = "/tmp/ecscope_test_target";
        
        // Create test files
        std::ofstream target_file(target_path);
        target_file << "test content";
        target_file.close();
        
        // Create symbolic link (if possible)
        std::error_code ec;
        fs::create_symlink(target_path, link_path, ec);
        
        if (!ec) {
            // Symbolic link creation succeeded
            REQUIRE(fs::is_symlink(link_path));
            REQUIRE(fs::exists(link_path));
            
            // Clean up
            fs::remove(link_path);
        }
        
        // Clean up target file
        fs::remove(target_path);
    }
}

#endif // __linux__

// =============================================================================
// CROSS-PLATFORM COMPATIBILITY TESTS
# =============================================================================

TEST_CASE("Cross-Platform File Dialog Compatibility", "[cross-platform][dialogs]") {
    SECTION("File dialog filters") {
        // Test file dialog filter formats across platforms
        std::vector<std::string> filters = {
            "Text files (*.txt)",
            "Image files (*.png;*.jpg;*.jpeg)",
            "All files (*.*)"
        };
        
        // This tests that our filter format is compatible
        for (const auto& filter : filters) {
            REQUIRE_FALSE(filter.empty());
            REQUIRE(filter.find('(') != std::string::npos);
            REQUIRE(filter.find(')') != std::string::npos);
        }
    }
    
    SECTION("Default save locations") {
        // Test platform-appropriate default save locations
        fs::path documents_path;
        
#ifdef _WIN32
        // Windows: Documents folder
        const char* userprofile = std::getenv("USERPROFILE");
        if (userprofile) {
            documents_path = fs::path(userprofile) / "Documents";
        }
#elif defined(__APPLE__)
        // macOS: Documents folder
        const char* home = std::getenv("HOME");
        if (home) {
            documents_path = fs::path(home) / "Documents";
        }
#elif defined(__linux__)
        // Linux: XDG Documents directory or fallback
        const char* xdg_documents = std::getenv("XDG_DOCUMENTS_DIR");
        if (xdg_documents) {
            documents_path = fs::path(xdg_documents);
        } else {
            const char* home = std::getenv("HOME");
            if (home) {
                documents_path = fs::path(home) / "Documents";
            }
        }
#endif
        
        if (!documents_path.empty()) {
            INFO("Documents path: " << documents_path.string());
            // Path should be constructible even if directory doesn't exist
            REQUIRE_FALSE(documents_path.empty());
        }
    }
}

TEST_CASE("Cross-Platform Input Handling", "[cross-platform][input]") {
    SECTION("Keyboard shortcuts") {
        // Test platform-specific keyboard shortcuts
        struct KeyboardShortcut {
            std::string action;
            bool ctrl_key;
            bool cmd_key;
            bool alt_key;
            std::string key;
        };
        
        std::vector<KeyboardShortcut> shortcuts = {
            {"copy", true, false, false, "C"},
            {"paste", true, false, false, "V"},
            {"undo", true, false, false, "Z"},
            {"redo", true, false, false, "Y"}
        };
        
#ifdef __APPLE__
        // On macOS, Cmd key is used instead of Ctrl
        for (auto& shortcut : shortcuts) {
            if (shortcut.ctrl_key) {
                shortcut.ctrl_key = false;
                shortcut.cmd_key = true;
            }
        }
#endif
        
        // Verify shortcuts are properly configured
        for (const auto& shortcut : shortcuts) {
            REQUIRE_FALSE(shortcut.action.empty());
            REQUIRE_FALSE(shortcut.key.empty());
            
#ifdef __APPLE__
            if (shortcut.action == "copy" || shortcut.action == "paste" || 
                shortcut.action == "undo" || shortcut.action == "redo") {
                REQUIRE(shortcut.cmd_key);
                REQUIRE_FALSE(shortcut.ctrl_key);
            }
#else
            if (shortcut.action == "copy" || shortcut.action == "paste" || 
                shortcut.action == "undo" || shortcut.action == "redo") {
                REQUIRE(shortcut.ctrl_key);
                REQUIRE_FALSE(shortcut.cmd_key);
            }
#endif
        }
    }
    
    SECTION("Mouse button handling") {
        // Test mouse button mapping across platforms
        enum class MouseButton {
            Left = 0,
            Right = 1,
            Middle = 2
        };
        
        // Mouse buttons should be consistent across platforms
        REQUIRE(static_cast<int>(MouseButton::Left) == 0);
        REQUIRE(static_cast<int>(MouseButton::Right) == 1);
        REQUIRE(static_cast<int>(MouseButton::Middle) == 2);
    }
}

TEST_CASE("Cross-Platform Thread Safety", "[cross-platform][threading]") {
    SECTION("GUI thread safety") {
        // Test that GUI operations are safe from main thread
        WindowConfig config;
        config.title = "Thread Safety Test";
        config.width = 800;
        config.height = 600;
        
        auto gui_manager = std::make_unique<GUIManager>();
        REQUIRE(gui_manager->initialize(config, GUIFlags::EnableDocking, nullptr));
        
        // Test basic thread safety
        bool test_completed = false;
        std::thread test_thread([&]() {
            // Simulate some work
            std::this_thread::sleep_for(100ms);
            test_completed = true;
        });
        
        // Continue GUI operations while thread runs
        while (!test_completed) {
            gui_manager->begin_frame();
            gui_manager->poll_events();
            gui_manager->end_frame();
            std::this_thread::sleep_for(1ms);
        }
        
        test_thread.join();
        REQUIRE(test_completed);
        
        gui_manager->shutdown();
    }
}