/**
 * @file gui.hpp
 * @brief Complete ECScope GUI Framework - Single Header Include
 * 
 * Professional immediate-mode GUI framework with complete widget system,
 * flexible layouts, theming, and high-performance rendering integration.
 * 
 * This single header provides access to the entire GUI framework.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

// =============================================================================
// CORE GUI SYSTEM
// =============================================================================

#include "gui/gui_core.hpp"        // Core types, context, and fundamental classes
#include "gui/gui_memory.hpp"      // Memory management for GUI (allocators, pools)
#include "gui/gui_input.hpp"       // Input handling (mouse, keyboard, gamepad)
#include "gui/gui_text.hpp"        // Text rendering, fonts, and typography
#include "gui/gui_renderer.hpp"    // Rendering integration with Vulkan/OpenGL

// =============================================================================
// WIDGET SYSTEM
// =============================================================================

#include "gui/gui_widgets.hpp"     // Complete widget collection

// =============================================================================
// LAYOUT SYSTEM
// =============================================================================

#include "gui/gui_layout.hpp"      // Windows, containers, docking, tables

// =============================================================================
// THEME SYSTEM
// =============================================================================

#include "gui/gui_theme.hpp"       // Colors, styles, fonts, animations

// =============================================================================
// ADVANCED FEATURES
// =============================================================================

#include "gui/gui_advanced.hpp"    // Drag-drop, tooltips, modals, notifications

// =============================================================================
// CONVENIENCE NAMESPACE ALIAS
// =============================================================================

/**
 * @brief Main GUI namespace
 * 
 * All GUI functionality is contained within ecscope::gui namespace.
 * Use 'using namespace ecscope::gui;' for convenience if desired.
 */
namespace gui = ecscope::gui;

// =============================================================================
// QUICK START DOCUMENTATION
// =============================================================================

/*
 * QUICK START GUIDE
 * =================
 * 
 * 1. Initialize the GUI system:
 * 
 *    auto gui_context = ecscope::gui::create_context();
 *    ecscope::gui::set_current_context(gui_context.get());
 *    gui_context->initialize(renderer, width, height);
 * 
 * 2. Main loop:
 * 
 *    while (running) {
 *        // Update input
 *        gui_context->set_mouse_pos(mouse_x, mouse_y);
 *        gui_context->set_mouse_button(MouseButton::Left, pressed);
 *        
 *        // Begin frame
 *        gui_context->new_frame(delta_time);
 *        
 *        // GUI code
 *        if (ecscope::gui::begin("My Window")) {
 *            ecscope::gui::text("Hello, GUI!");
 *            if (ecscope::gui::button("Click me!")) {
 *                std::cout << "Button clicked!" << std::endl;
 *            }
 *        }
 *        ecscope::gui::end();
 *        
 *        // End frame and render
 *        gui_context->end_frame();
 *        gui_context->render();
 *    }
 * 
 * 3. Key features:
 * 
 *    - Complete widget system (buttons, sliders, inputs, tables, etc.)
 *    - Flexible layouts with docking and containers
 *    - Professional theming system
 *    - Advanced features (drag-drop, modals, notifications)
 *    - High-performance rendering
 *    - Memory management optimized for real-time applications
 *    - Full integration with ECScope rendering engine
 * 
 * 4. Examples:
 * 
 *    See examples/gui_comprehensive_demo.cpp for a complete demonstration
 *    of all GUI features and capabilities.
 */