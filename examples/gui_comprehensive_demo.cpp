/**
 * @file gui_comprehensive_demo.cpp
 * @brief Comprehensive GUI Framework Demo
 * 
 * Complete demonstration of all GUI framework features including widgets,
 * layouts, themes, advanced features, and performance showcases.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include <iostream>
#include <memory>
#include <chrono>
#include <vector>
#include <map>
#include <random>

// ECS and rendering system
#include "ecscope/rendering/renderer.hpp"
#include "ecscope/rendering/opengl_backend.hpp"
#include "ecscope/rendering/vulkan_backend.hpp"

// Complete GUI system
#include "ecscope/gui/gui_core.hpp"
#include "ecscope/gui/gui_memory.hpp"
#include "ecscope/gui/gui_input.hpp"
#include "ecscope/gui/gui_text.hpp"
#include "ecscope/gui/gui_widgets.hpp"
#include "ecscope/gui/gui_layout.hpp"
#include "ecscope/gui/gui_theme.hpp"
#include "ecscope/gui/gui_renderer.hpp"
#include "ecscope/gui/gui_advanced.hpp"

// Platform
#include <GLFW/glfw3.h>

using namespace ecscope;
using namespace ecscope::gui;
using namespace ecscope::rendering;

// =============================================================================
// DEMO APPLICATION CLASS
// =============================================================================

class GuiDemoApplication {
public:
    GuiDemoApplication() = default;
    ~GuiDemoApplication() = default;
    
    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Create window
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA
        
        window_ = glfwCreateWindow(1920, 1080, "ECScope GUI Framework - Comprehensive Demo", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1); // VSync
        
        // Set up callbacks
        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);
        glfwSetKeyCallback(window_, key_callback);
        glfwSetCharCallback(window_, char_callback);
        glfwSetMouseButtonCallback(window_, mouse_button_callback);
        glfwSetCursorPosCallback(window_, cursor_pos_callback);
        glfwSetScrollCallback(window_, scroll_callback);
        
        // Initialize renderer
        renderer_ = RendererFactory::create(RenderingAPI::OpenGL, window_);
        if (!renderer_ || !renderer_->initialize()) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return false;
        }
        
        // Get window size
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        
        // Initialize GUI system
        gui_context_ = create_context();
        set_current_context(gui_context_.get());
        
        if (!gui_context_->initialize(renderer_.get(), width, height)) {
            std::cerr << "Failed to initialize GUI context" << std::endl;
            return false;
        }
        
        // Initialize demo state
        initialize_demo_state();
        
        std::cout << "GUI Demo Application initialized successfully!" << std::endl;
        std::cout << "Renderer: " << RendererFactory::api_to_string(renderer_->get_api()) << std::endl;
        
        auto caps = renderer_->get_capabilities();
        std::cout << "Max Texture Size: " << caps.max_texture_size << std::endl;
        std::cout << "Max MSAA Samples: " << caps.max_msaa_samples << std::endl;
        
        return true;
    }
    
    void run() {
        auto last_time = std::chrono::high_resolution_clock::now();
        
        while (!glfwWindowShouldClose(window_)) {
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;
            
            glfwPollEvents();
            
            // Update GUI
            update_input();
            gui_context_->new_frame(delta_time);
            
            // Render demo
            render_demo();
            
            // End frame and render
            gui_context_->end_frame();
            
            // Present
            renderer_->begin_frame();
            renderer_->clear({0.1f, 0.1f, 0.1f, 1.0f});
            gui_context_->render();
            renderer_->end_frame();
            
            glfwSwapBuffers(window_);
            
            // Update FPS
            frame_count_++;
            fps_timer_ += delta_time;
            if (fps_timer_ >= 1.0f) {
                fps_ = frame_count_;
                frame_count_ = 0;
                fps_timer_ = 0.0f;
            }
        }
    }
    
    void shutdown() {
        if (gui_context_) {
            gui_context_->shutdown();
            gui_context_.reset();
        }
        
        if (renderer_) {
            renderer_->shutdown();
            renderer_.reset();
        }
        
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        
        glfwTerminate();
    }
    
private:
    // Core systems
    GLFWwindow* window_ = nullptr;
    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<GuiContext> gui_context_;
    
    // Demo state
    struct DemoState {
        // Basic widgets
        bool show_basic_widgets = true;
        bool show_layout_demo = true;
        bool show_text_demo = true;
        bool show_advanced_features = true;
        bool show_performance_monitor = true;
        bool show_theme_editor = false;
        bool show_style_editor = false;
        bool show_memory_monitor = false;
        
        // Widget values
        std::string text_input = "Hello, ECScope GUI!";
        std::string multiline_text = "This is a multiline\ntext input widget\nwith multiple lines.";
        float float_value = 0.5f;
        int int_value = 42;
        bool checkbox_value = true;
        int radio_value = 1;
        float slider_value = 0.7f;
        std::array<float, 3> color_value = {1.0f, 0.5f, 0.2f};
        
        // Advanced features
        int selected_item = 0;
        std::vector<std::string> combo_items = {"Apple", "Banana", "Cherry", "Date", "Elderberry"};
        std::array<float, 4> drag_values = {1.0f, 2.0f, 3.0f, 4.0f};
        
        // Layout demo
        float splitter_size1 = 200.0f;
        float splitter_size2 = 300.0f;
        int tab_selection = 0;
        
        // Performance data
        std::vector<float> frame_times;
        std::vector<float> memory_usage;
        
        // Theme selection
        std::vector<std::string> available_themes = {"Dark", "Light", "Classic", "High Contrast", "Modern"};
        int current_theme = 0;
        
    } demo_state_;
    
    // Performance monitoring
    int fps_ = 0;
    int frame_count_ = 0;
    float fps_timer_ = 0.0f;
    
    void initialize_demo_state() {
        // Initialize performance data
        demo_state_.frame_times.reserve(120);
        demo_state_.memory_usage.reserve(120);
        
        // Set up default theme
        auto* theme_manager = get_theme_manager();
        if (theme_manager) {
            theme_manager->register_builtin_themes();
            theme_manager->apply_theme("Dark");
        }
    }
    
    void update_input() {
        // Update mouse position
        double mouse_x, mouse_y;
        glfwGetCursorPos(window_, &mouse_x, &mouse_y);
        gui_context_->set_mouse_pos(static_cast<float>(mouse_x), static_cast<float>(mouse_y));
        
        // Update mouse buttons
        gui_context_->set_mouse_button(MouseButton::Left, 
                                      glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        gui_context_->set_mouse_button(MouseButton::Right, 
                                      glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
        gui_context_->set_mouse_button(MouseButton::Middle, 
                                      glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
        
        // Update keyboard modifiers
        KeyMod mods = KeyMod::None;
        if (glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
            mods = mods | KeyMod::Ctrl;
        }
        if (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
            mods = mods | KeyMod::Shift;
        }
        if (glfwGetKey(window_, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
            mods = mods | KeyMod::Alt;
        }
        gui_context_->set_key_mods(mods);
    }
    
    void render_demo() {
        render_main_menu_bar();
        render_dockspace();
        
        if (demo_state_.show_basic_widgets) {
            render_basic_widgets_demo();
        }
        
        if (demo_state_.show_layout_demo) {
            render_layout_demo();
        }
        
        if (demo_state_.show_text_demo) {
            render_text_demo();
        }
        
        if (demo_state_.show_advanced_features) {
            render_advanced_features_demo();
        }
        
        if (demo_state_.show_performance_monitor) {
            render_performance_monitor();
        }
        
        if (demo_state_.show_theme_editor) {
            render_theme_editor();
        }
        
        if (demo_state_.show_style_editor) {
            render_style_editor();
        }
        
        if (demo_state_.show_memory_monitor) {
            render_memory_monitor();
        }
        
        // Demo tooltips and context menus
        render_demo_overlays();
    }
    
    void render_main_menu_bar() {
        if (begin_main_menu_bar()) {
            if (begin_menu("Demo")) {
                menu_item("Basic Widgets", "", &demo_state_.show_basic_widgets);
                menu_item("Layout Demo", "", &demo_state_.show_layout_demo);
                menu_item("Text Demo", "", &demo_state_.show_text_demo);
                menu_item("Advanced Features", "", &demo_state_.show_advanced_features);
                separator();
                menu_item("Performance Monitor", "", &demo_state_.show_performance_monitor);
                menu_item("Memory Monitor", "", &demo_state_.show_memory_monitor);
                end_menu();
            }
            
            if (begin_menu("Tools")) {
                menu_item("Theme Editor", "", &demo_state_.show_theme_editor);
                menu_item("Style Editor", "", &demo_state_.show_style_editor);
                separator();
                if (menu_item("Save Layout")) {
                    // Save current layout
                }
                if (menu_item("Load Layout")) {
                    // Load layout
                }
                end_menu();
            }
            
            if (begin_menu("Help")) {
                if (menu_item("About")) {
                    show_message_box("About ECScope GUI", 
                                    "ECScope GUI Framework v1.0.0\n"
                                    "Professional immediate-mode GUI for C++\n\n"
                                    "Features:\n"
                                    "• Complete widget system\n"
                                    "• Flexible layouts and docking\n"
                                    "• Advanced theming\n"
                                    "• High-performance rendering\n"
                                    "• Memory management\n"
                                    "• And much more!",
                                    ModalType::Info);
                }
                if (menu_item("Documentation")) {
                    // Open documentation
                }
                end_menu();
            }
            
            // Right-aligned status
            float menu_bar_height = get_frame_height();
            set_cursor_pos_x(get_window_width() - 200);
            text("FPS: " + std::to_string(fps_));
            
            end_main_menu_bar();
        }
    }
    
    void render_dockspace() {
        static GuiID dockspace_id = 0;
        
        // Create dockspace over viewport
        dockspace_id = dockspace_over_viewport(DockNodeFlags::PassthruCentralNode);
        
        // Set up default layout on first run
        static bool first_time = true;
        if (first_time) {
            first_time = false;
            
            // Split the dockspace
            GuiID left_id, right_id, bottom_id;
            DockBuilder::dock_builder_remove_node(dockspace_id);
            GuiID main_id = DockBuilder::dock_builder_add_node(dockspace_id, DockNodeFlags::None);
            
            left_id = DockBuilder::dock_builder_split_node(main_id, LayoutDirection::Horizontal, 0.25f, nullptr, &main_id);
            bottom_id = DockBuilder::dock_builder_split_node(main_id, LayoutDirection::Vertical, 0.3f, nullptr, &main_id);
            right_id = DockBuilder::dock_builder_split_node(main_id, LayoutDirection::Horizontal, 0.7f, nullptr, &main_id);
            
            // Dock windows
            DockBuilder::dock_builder_dock_window("Basic Widgets", left_id);
            DockBuilder::dock_builder_dock_window("Layout Demo", main_id);
            DockBuilder::dock_builder_dock_window("Performance Monitor", bottom_id);
            DockBuilder::dock_builder_dock_window("Advanced Features", right_id);
            
            DockBuilder::dock_builder_finish(dockspace_id);
        }
    }
    
    void render_basic_widgets_demo() {
        if (begin("Basic Widgets", &demo_state_.show_basic_widgets)) {
            text("Basic Widget Demonstration");
            separator();
            
            // Buttons
            collapsing_header("Buttons");
            if (button("Regular Button")) {
                show_info_notification("Button", "Regular button clicked!");
            }
            same_line();
            if (button_colored("Colored Button", Color{0.2f, 0.7f, 0.3f, 1.0f})) {
                show_success_notification("Button", "Colored button clicked!");
            }
            if (button_small("Small")) {
                show_info_notification("Button", "Small button clicked!");
            }
            same_line();
            arrow_button("left_arrow", NavDirection::Left);
            same_line();
            arrow_button("right_arrow", NavDirection::Right);
            
            // Input widgets
            spacing();
            collapsing_header("Input Widgets");
            input_text("Text Input", demo_state_.text_input);
            input_text_multiline("Multiline", demo_state_.multiline_text, Vec2{0, 80});
            input_float("Float", &demo_state_.float_value, 0.01f, 1.0f);
            input_int("Integer", &demo_state_.int_value, 1, 10);
            
            // Sliders and drags
            spacing();
            collapsing_header("Sliders & Drags");
            slider_float("Slider", &demo_state_.slider_value, 0.0f, 1.0f);
            drag_float4("Drag Values", demo_state_.drag_values.data(), 0.1f, 0.0f, 10.0f);
            
            // Selection widgets
            spacing();
            collapsing_header("Selection");
            checkbox("Checkbox", &demo_state_.checkbox_value);
            radio_button("Option 1", &demo_state_.radio_value, 0); same_line();
            radio_button("Option 2", &demo_state_.radio_value, 1); same_line();
            radio_button("Option 3", &demo_state_.radio_value, 2);
            
            combo("Combo Box", &demo_state_.selected_item, demo_state_.combo_items);
            
            // Colors
            spacing();
            collapsing_header("Colors");
            color_edit3("Color", demo_state_.color_value.data());
            color_button("Color Button", Color{demo_state_.color_value[0], demo_state_.color_value[1], demo_state_.color_value[2], 1.0f});
            
            // Progress
            spacing();
            collapsing_header("Progress");
            static float progress = 0.0f;
            progress += get_delta_time() * 0.3f;
            if (progress > 1.0f) progress = 0.0f;
            progress_bar(progress, Vec2{-1, 0}, std::to_string(static_cast<int>(progress * 100)) + "%");
            
            loading_spinner("Loading", 15.0f, 3.0f);
        }
        end();
    }
    
    void render_layout_demo() {
        if (begin("Layout Demo", &demo_state_.show_layout_demo)) {
            text("Layout System Demonstration");
            separator();
            
            // Tabs
            if (begin_tab_bar("LayoutTabs")) {
                if (begin_tab_item("Containers")) {
                    text("Flexbox-like Layout Container");
                    separator();
                    
                    begin_vertical_layout(10.0f);
                    
                    layout_item(0.0f); // Fixed size
                    if (button("Fixed Size Button", Vec2{200, 30})) {}
                    
                    layout_item(1.0f); // Flexible
                    begin_child("FlexChild", Vec2{0, 0}, true);
                    text("This child takes remaining space");
                    for (int i = 0; i < 10; ++i) {
                        text("Flexible content line " + std::to_string(i));
                    }
                    end_child();
                    
                    layout_item(0.0f); // Fixed size
                    if (button("Another Fixed Button", Vec2{300, 25})) {}
                    
                    end_layout();
                    
                    end_tab_item();
                }
                
                if (begin_tab_item("Splitters")) {
                    text("Resizable Splitter Panels");
                    separator();
                    
                    splitter("demo_splitter", &demo_state_.splitter_size1, &demo_state_.splitter_size2, 50.0f, 50.0f, -1.0f);
                    
                    begin_child("Left Panel", Vec2{demo_state_.splitter_size1, 0}, true);
                    text("Left Panel Content");
                    text("Size: %.1f", demo_state_.splitter_size1);
                    for (int i = 0; i < 20; ++i) {
                        text("Item %d", i);
                    }
                    end_child();
                    
                    same_line();
                    
                    begin_child("Right Panel", Vec2{demo_state_.splitter_size2, 0}, true);
                    text("Right Panel Content");
                    text("Size: %.1f", demo_state_.splitter_size2);
                    
                    // Nested tabs in right panel
                    if (begin_tab_bar("NestedTabs", TabBarFlags::Reorderable)) {
                        if (begin_tab_item("Tab A")) {
                            text("Content of Tab A");
                            button("Button in Tab A");
                            end_tab_item();
                        }
                        if (begin_tab_item("Tab B")) {
                            text("Content of Tab B");
                            slider_float("Slider in Tab B", &demo_state_.slider_value, 0.0f, 1.0f);
                            end_tab_item();
                        }
                        end_tab_bar();
                    }
                    
                    end_child();
                    
                    end_tab_item();
                }
                
                if (begin_tab_item("Tables")) {
                    text("Advanced Table System");
                    separator();
                    
                    if (begin_table("demo_table", 4, TableFlags::Resizable | TableFlags::Reorderable | 
                                   TableFlags::Hideable | TableFlags::BordersOuter | TableFlags::BordersV)) {
                        table_setup_column("Name");
                        table_setup_column("Age", TableColumnFlags::WidthFixed, 80.0f);
                        table_setup_column("Score", TableColumnFlags::WidthFixed, 100.0f);
                        table_setup_column("Actions");
                        table_headers_row();
                        
                        for (int i = 0; i < 10; ++i) {
                            table_next_row();
                            
                            table_next_column();
                            text("Person %d", i + 1);
                            
                            table_next_column();
                            text("%d", 20 + i * 2);
                            
                            table_next_column();
                            progress_bar(static_cast<float>(i) / 10.0f, Vec2{-1, 0});
                            
                            table_next_column();
                            if (button(("Edit##" + std::to_string(i)).c_str())) {
                                show_info_notification("Table", "Edit person " + std::to_string(i + 1));
                            }
                            same_line();
                            if (button(("Delete##" + std::to_string(i)).c_str())) {
                                show_warning_notification("Table", "Delete person " + std::to_string(i + 1));
                            }
                        }
                        
                        end_table();
                    }
                    
                    end_tab_item();
                }
                
                end_tab_bar();
            }
        }
        end();
    }
    
    void render_text_demo() {
        if (begin("Text Demo", &demo_state_.show_text_demo)) {
            text("Text Rendering Demonstration");
            separator();
            
            // Different text styles
            text("Regular text");
            
            push_color(GuiColor::Text, Color{1.0f, 0.5f, 0.2f, 1.0f});
            text("Colored text");
            pop_color();
            
            text("Text with different sizes:");
            
            // Would need font system integration for different sizes
            text("• Normal size text");
            text("• This would be larger with font scaling");
            text("• This would be smaller with font scaling");
            
            separator();
            
            text("Text wrapping and alignment:");
            text("This is a very long line of text that should wrap around when it reaches the edge of the available space. The text wrapping system handles this automatically based on the available width.");
            
            separator();
            
            // Rich text formatting (would need implementation)
            text("Rich text features (concept):");
            text("• Bold text");
            text("• Italic text");  
            text("• Underlined text");
            text("• Links and hypertext");
            
            separator();
            
            // Text input showcase
            text("Text Input Variants:");
            static std::string password = "secret";
            input_text("Password", password, InputTextFlags::Password);
            
            static std::string readonly = "Read-only text";
            input_text("Read Only", readonly, InputTextFlags::ReadOnly);
            
            static std::string numbers = "123.456";
            input_text("Numbers Only", numbers, InputTextFlags::CharsDecimal);
        }
        end();
    }
    
    void render_advanced_features_demo() {
        if (begin("Advanced Features", &demo_state_.show_advanced_features)) {
            text("Advanced Features Demonstration");
            separator();
            
            if (collapsing_header("Drag and Drop", TreeNodeFlags::DefaultOpen)) {
                text("Drag items between boxes:");
                
                static std::vector<std::string> box1 = {"Item A", "Item B", "Item C"};
                static std::vector<std::string> box2 = {"Item 1", "Item 2"};
                
                // Box 1
                text("Box 1:");
                begin_child("DragDropBox1", Vec2{200, 100}, true);
                for (size_t i = 0; i < box1.size(); ++i) {
                    push_id(static_cast<int>(i));
                    if (selectable(box1[i])) {
                        if (begin_drag_drop_source()) {
                            set_drag_drop_payload("DEMO_ITEM", &i, sizeof(size_t));
                            text("Moving: %s", box1[i].c_str());
                            end_drag_drop_source();
                        }
                    }
                    pop_id();
                }
                
                if (begin_drag_drop_target()) {
                    if (auto* payload = accept_drag_drop_payload("DEMO_ITEM")) {
                        size_t* item_index = static_cast<size_t*>(payload->data);
                        // Handle drop
                    }
                    end_drag_drop_target();
                }
                end_child();
                
                same_line();
                
                // Box 2
                text("Box 2:");
                begin_child("DragDropBox2", Vec2{200, 100}, true);
                for (size_t i = 0; i < box2.size(); ++i) {
                    push_id(static_cast<int>(i + 1000));
                    if (selectable(box2[i])) {
                        if (begin_drag_drop_source()) {
                            set_drag_drop_payload("DEMO_ITEM", &i, sizeof(size_t));
                            text("Moving: %s", box2[i].c_str());
                            end_drag_drop_source();
                        }
                    }
                    pop_id();
                }
                
                if (begin_drag_drop_target()) {
                    if (auto* payload = accept_drag_drop_payload("DEMO_ITEM")) {
                        size_t* item_index = static_cast<size_t*>(payload->data);
                        // Handle drop
                    }
                    end_drag_drop_target();
                }
                end_child();
            }
            
            if (collapsing_header("Context Menus")) {
                text("Right-click for context menu:");
                
                if (button("Right-click me")) {
                    // Button action
                }
                
                if (begin_popup_context_item("item_context")) {
                    if (menu_item("Action 1")) {
                        show_info_notification("Context Menu", "Action 1 selected");
                    }
                    if (menu_item("Action 2")) {
                        show_info_notification("Context Menu", "Action 2 selected");
                    }
                    separator();
                    if (menu_item("Delete", "", false, false)) {
                        // Disabled item
                    }
                    end_popup();
                }
            }
            
            if (collapsing_header("Modals and Dialogs")) {
                if (button("Show Info Dialog")) {
                    show_message_box("Information", "This is an information dialog.", ModalType::Info);
                }
                same_line();
                if (button("Show Warning")) {
                    show_message_box("Warning", "This is a warning dialog.", ModalType::Warning);
                }
                same_line();
                if (button("Show Error")) {
                    show_error_dialog("Error", "This is an error dialog.");
                }
                
                if (button("Show Confirmation")) {
                    auto result = show_confirmation_dialog("Confirm Action", "Are you sure you want to proceed?");
                    if (result == ModalResult::Yes) {
                        show_success_notification("Confirmed", "Action confirmed!");
                    }
                }
            }
            
            if (collapsing_header("Tooltips")) {
                button("Hover me");
                if (is_item_hovered()) {
                    set_tooltip("This is a tooltip!");
                }
                
                same_line();
                
                button("Rich tooltip");
                if (is_item_hovered()) {
                    begin_tooltip();
                    text("This is a rich tooltip");
                    separator();
                    text("With multiple lines");
                    text("And different colors:");
                    push_color(GuiColor::Text, Color{1.0f, 0.0f, 0.0f, 1.0f});
                    text("Red text");
                    pop_color();
                    end_tooltip();
                }
            }
        }
        end();
    }
    
    void render_performance_monitor() {
        if (begin("Performance Monitor", &demo_state_.show_performance_monitor)) {
            text("Performance Monitoring");
            separator();
            
            // Update performance data
            auto& frame_times = demo_state_.frame_times;
            if (frame_times.size() >= 120) {
                frame_times.erase(frame_times.begin());
            }
            frame_times.push_back(get_delta_time() * 1000.0f); // Convert to ms
            
            // FPS display
            text("FPS: %d (%.2f ms)", fps_, get_delta_time() * 1000.0f);
            
            // Frame time graph
            if (!frame_times.empty()) {
                float min_time = *std::min_element(frame_times.begin(), frame_times.end());
                float max_time = *std::max_element(frame_times.begin(), frame_times.end());
                
                text("Frame Time: %.2f ms (min: %.2f, max: %.2f)", 
                     frame_times.back(), min_time, max_time);
                
                // Simple graph visualization (would need custom plotting widget)
                progress_bar(frame_times.back() / 33.33f, Vec2{-1, 0}, "Frame Time");
            }
            
            separator();
            
            // Renderer statistics
            if (renderer_) {
                auto stats = renderer_->get_frame_stats();
                text("Renderer Statistics:");
                text("  Draw Calls: %u", stats.draw_calls);
                text("  Vertices: %u", stats.vertices_rendered);
                text("  GPU Time: %.2f ms", stats.gpu_time_ms);
                text("  Memory Used: %llu MB", stats.memory_used / (1024 * 1024));
            }
            
            separator();
            
            // Memory statistics
            auto& memory_manager = MemoryManager::instance();
            auto mem_stats = memory_manager.get_stats();
            text("Memory Statistics:");
            text("  Frame Memory: %zu / %zu bytes", mem_stats.frame_allocated, mem_stats.frame_capacity);
            text("  Persistent Memory: %zu / %zu bytes", mem_stats.persistent_allocated, mem_stats.persistent_capacity);
            text("  Total Allocations: %zu", mem_stats.total_allocations);
            text("  Peak Frame Usage: %zu bytes", mem_stats.peak_frame_usage);
            
            float frame_utilization = mem_stats.frame_capacity > 0 ? 
                static_cast<float>(mem_stats.frame_allocated) / mem_stats.frame_capacity : 0.0f;
            progress_bar(frame_utilization, Vec2{-1, 0}, "Frame Memory");
            
            float persistent_utilization = mem_stats.persistent_capacity > 0 ? 
                static_cast<float>(mem_stats.persistent_allocated) / mem_stats.persistent_capacity : 0.0f;
            progress_bar(persistent_utilization, Vec2{-1, 0}, "Persistent Memory");
        }
        end();
    }
    
    void render_theme_editor() {
        if (begin("Theme Editor", &demo_state_.show_theme_editor)) {
            text("Theme Editor");
            separator();
            
            // Theme selection
            combo("Current Theme", &demo_state_.current_theme, demo_state_.available_themes);
            
            if (button("Apply Theme")) {
                auto* theme_manager = get_theme_manager();
                if (theme_manager) {
                    theme_manager->apply_theme(demo_state_.available_themes[demo_state_.current_theme]);
                }
            }
            
            separator();
            
            // Color editing
            text("Colors:");
            static std::array<float, 3> window_bg = {0.1f, 0.1f, 0.1f};
            static std::array<float, 3> button_bg = {0.2f, 0.2f, 0.2f};
            static std::array<float, 3> text_color = {1.0f, 1.0f, 1.0f};
            
            if (color_edit3("Window Background", window_bg.data())) {
                // Apply color change
            }
            if (color_edit3("Button Background", button_bg.data())) {
                // Apply color change  
            }
            if (color_edit3("Text Color", text_color.data())) {
                // Apply color change
            }
            
            separator();
            
            text("Style Variables:");
            static float window_rounding = 5.0f;
            static float frame_padding = 4.0f;
            static float item_spacing = 8.0f;
            
            if (slider_float("Window Rounding", &window_rounding, 0.0f, 12.0f)) {
                push_style_var(GuiStyleVar::WindowRounding, window_rounding);
            }
            if (slider_float("Frame Padding", &frame_padding, 0.0f, 20.0f)) {
                push_style_var(GuiStyleVar::FramePadding, Vec2{frame_padding, frame_padding});
            }
            if (slider_float("Item Spacing", &item_spacing, 0.0f, 20.0f)) {
                push_style_var(GuiStyleVar::ItemSpacing, Vec2{item_spacing, item_spacing});
            }
            
            separator();
            
            if (button("Save Theme")) {
                // Save current theme
            }
            same_line();
            if (button("Reset to Default")) {
                // Reset theme
            }
        }
        end();
    }
    
    void render_style_editor() {
        if (begin("Style Editor", &demo_state_.show_style_editor)) {
            text("Style Editor - Fine-tune visual appearance");
            separator();
            
            // This would contain detailed style editing controls
            text("Detailed style editing controls would be here");
            text("Including spacing, sizing, colors, fonts, etc.");
            
            // Sample style controls
            static float alpha = 1.0f;
            slider_float("Global Alpha", &alpha, 0.0f, 1.0f);
            
            static float disabled_alpha = 0.6f;
            slider_float("Disabled Alpha", &disabled_alpha, 0.0f, 1.0f);
        }
        end();
    }
    
    void render_memory_monitor() {
        if (begin("Memory Monitor", &demo_state_.show_memory_monitor)) {
            text("Memory Monitoring and Profiling");
            separator();
            
            // Detailed memory information
            text("Memory pools and allocation tracking would be here");
            text("Including allocation patterns, leak detection, etc.");
        }
        end();
    }
    
    void render_demo_overlays() {
        // Demo notifications
        static float notification_timer = 0.0f;
        notification_timer += get_delta_time();
        
        if (notification_timer > 10.0f) {
            show_info_notification("Demo", "Periodic demo notification", 2.0f);
            notification_timer = 0.0f;
        }
    }
    
    // GLFW callbacks
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        auto* app = static_cast<GuiDemoApplication*>(glfwGetWindowUserPointer(window));
        if (app && app->gui_context_) {
            app->gui_context_->set_display_size(width, height);
        }
    }
    
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* app = static_cast<GuiDemoApplication*>(glfwGetWindowUserPointer(window));
        if (app && app->gui_context_) {
            // Convert GLFW key to GUI key
            Key gui_key = static_cast<Key>(key);
            bool pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
            app->gui_context_->set_key(gui_key, pressed);
        }
    }
    
    static void char_callback(GLFWwindow* window, unsigned int codepoint) {
        auto* app = static_cast<GuiDemoApplication*>(glfwGetWindowUserPointer(window));
        if (app && app->gui_context_) {
            app->gui_context_->add_input_character(codepoint);
        }
    }
    
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        auto* app = static_cast<GuiDemoApplication*>(glfwGetWindowUserPointer(window));
        if (app && app->gui_context_) {
            MouseButton gui_button = static_cast<MouseButton>(button);
            bool pressed = (action == GLFW_PRESS);
            app->gui_context_->set_mouse_button(gui_button, pressed);
        }
    }
    
    static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
        auto* app = static_cast<GuiDemoApplication*>(glfwGetWindowUserPointer(window));
        if (app && app->gui_context_) {
            app->gui_context_->set_mouse_pos(static_cast<float>(xpos), static_cast<float>(ypos));
        }
    }
    
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
        auto* app = static_cast<GuiDemoApplication*>(glfwGetWindowUserPointer(window));
        if (app && app->gui_context_) {
            app->gui_context_->set_mouse_wheel(static_cast<float>(yoffset));
        }
    }
};

// =============================================================================
// MAIN FUNCTION
// =============================================================================

int main() {
    std::cout << "ECScope GUI Framework - Comprehensive Demo" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    GuiDemoApplication app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize demo application!" << std::endl;
        return -1;
    }
    
    std::cout << "Demo application initialized. Starting main loop..." << std::endl;
    
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception in main loop: " << e.what() << std::endl;
        app.shutdown();
        return -1;
    }
    
    std::cout << "Demo application shutting down..." << std::endl;
    app.shutdown();
    
    std::cout << "Demo application terminated successfully." << std::endl;
    return 0;
}