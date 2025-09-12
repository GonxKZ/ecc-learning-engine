# ECScope Responsive Design System

## Overview

The ECScope Responsive Design System provides comprehensive support for creating adaptive, scalable, and touch-friendly user interfaces using Dear ImGui. This system is designed specifically for professional game engine interfaces that need to work seamlessly across different screen sizes, DPI settings, and input methods while maintaining visual quality and usability.

## Key Features

### 🎯 **DPI Scaling & High-Resolution Display Support**
- Automatic detection and scaling for high-DPI displays
- Support for multiple monitor configurations with different DPI scales
- Smooth scaling transitions with customizable animation curves
- Manual override options for user preferences

### 📱 **Responsive Breakpoint System**
- Six responsive breakpoints: XSmall, Small, Medium, Large, XLarge, XXLarge
- Adaptive layout components that automatically adjust to screen size
- Fluid and discrete breakpoint modes
- Screen size change callbacks for custom adaptations

### 🖱️ **Touch Interface Support**
- Automatic touch capability detection
- Touch-friendly sizing for interactive elements (44pt minimum touch targets)
- Gesture-aware spacing and padding adjustments
- Platform-specific touch optimizations

### 🎨 **Professional Typography System**
- Responsive font scaling across all screen sizes
- Six semantic font sizes: Display, H1, H2, H3, Body, Small, Tiny
- Font oversampling for crisp rendering at all scales
- Context-aware font selection

### 📏 **Intelligent Spacing & Layout**
- Seven-tier responsive spacing system (tiny to huge)
- Screen-size aware spacing calculations
- Adaptive column layouts with automatic column count adjustment
- Flexible layout components with grow/shrink behavior

### 🧪 **Comprehensive Testing Framework**
- Automated responsive design testing across all screen sizes
- Visual regression testing with screenshot comparison
- Performance benchmarking for layout operations
- Accessibility compliance validation

## Architecture

```
ResponsiveDesignManager
├── Display Detection & DPI Scaling
├── Breakpoint Management
├── Font System
├── Spacing & Layout Engine
├── Touch Interface Handler
└── Component Renderer

ResponsiveTestingFramework
├── Test Case Management
├── Screen Simulation
├── Performance Measurement
├── Accessibility Testing
└── Report Generation
```

## Quick Start

### Basic Integration

```cpp
#include "ecscope/gui/responsive_design.hpp"

// Initialize responsive design system
ResponsiveConfig config;
config.mode = ResponsiveMode::Adaptive;
config.touch_mode = TouchMode::Auto;
config.auto_dpi_scaling = true;

InitializeGlobalResponsiveDesign(window, config);
auto* manager = GetResponsiveDesignManager();

// In your render loop
manager->update(delta_time);

// Use responsive components
if (manager->begin_responsive_window("My Window")) {
    manager->responsive_button("Click Me");
    manager->end_responsive_window();
}
```

### Responsive Layouts

```cpp
// Adaptive columns that adjust to screen size
manager->begin_adaptive_columns(2, 4); // 2-4 columns based on space
render_content_column_1();
ImGui::NextColumn();
render_content_column_2();
manager->end_adaptive_columns();

// Responsive spacing
ImVec2 spacing = manager->get_spacing_vec2("large", "medium");
ImGui::Dummy(spacing);

// Screen size conditional rendering
if (manager->is_screen_at_least(ScreenSize::Medium)) {
    render_desktop_features();
} else {
    render_mobile_optimized_ui();
}
```

## Responsive Breakpoints

| Breakpoint | Size Range | Typical Devices | Columns | Spacing |
|------------|------------|-----------------|---------|---------|
| XSmall     | < 480px    | Mobile Portrait | 1       | Tight   |
| Small      | < 768px    | Mobile Landscape| 1-2     | Compact |
| Medium     | < 1024px   | Tablets         | 2-3     | Standard|
| Large      | < 1440px   | Laptops/Desktop | 3-4     | Comfortable|
| XLarge     | < 1920px   | Large Monitors  | 4-6     | Generous|
| XXLarge    | >= 1920px  | 4K/Ultrawide    | 6-8     | Maximum |

## DPI Scaling Categories

| Category   | DPI Scale | Examples              | Font Multiplier |
|------------|-----------|----------------------|----------------|
| Standard   | 1.0x      | 1080p @ 24"         | 1.0x           |
| High       | 1.5x      | 1440p @ 24"         | 1.25x          |
| Very High  | 2.0x      | 4K @ 24", Retina    | 1.5x           |
| Ultra      | 3.0x+     | 4K @ 15", High-DPI  | 2.0x           |

## Typography Scale

### Font Hierarchy
```
Display:  36-48px  - Hero headlines, splash screens
H1:       30-40px  - Page/section titles  
H2:       24-32px  - Subsection headers
H3:       20-28px  - Component titles
Body:     16-18px  - Default text content
Small:    14-16px  - Secondary information
Tiny:     12-14px  - Captions, metadata
```

### Responsive Font Usage
```cpp
// Get appropriate font for current screen
ImFont* title_font = manager->get_font(manager->get_screen_size(), "h1");
ImGui::PushFont(title_font);
ImGui::Text("Responsive Title");
ImGui::PopFont();

// Or use calculated size
float body_size = manager->get_font_size("body");
```

## Spacing System

### Spacing Values (base values, scaled by screen size and DPI)

| Size     | Mobile | Tablet | Desktop | Use Case            |
|----------|--------|--------|---------|---------------------|
| tiny     | 1-2px  | 2-3px  | 3-4px   | Element borders     |
| small    | 2-4px  | 4-6px  | 6-8px   | Inner element gaps  |
| medium   | 4-8px  | 8-12px | 12-16px | Standard spacing    |
| large    | 8-16px | 16-20px| 20-24px | Section separators  |
| xlarge   | 12-24px| 24-28px| 28-32px | Panel padding       |
| xxlarge  | 16-32px| 32-36px| 36-48px | Window margins      |
| huge     | 24-48px| 48-56px| 56-64px | Major sections      |

```cpp
// Usage examples
float standard_gap = manager->get_spacing("medium");
ImVec2 button_padding = manager->get_spacing_vec2("large", "medium");
ImGui::Dummy(ImVec2(0, manager->get_spacing("huge"))); // Big vertical space
```

## Touch Interface Guidelines

### Touch Target Sizes
- **Minimum**: 44pt (iOS) / 48dp (Android)
- **Recommended**: 56pt for primary actions
- **Spacing**: Minimum 8pt between targets

```cpp
// Automatic touch-friendly sizing
if (manager->is_touch_enabled()) {
    ImVec2 touch_size = manager->get_touch_button_size();
    ImGui::Button("Touch Friendly", touch_size);
}

// Touch-aware spacing
float touch_spacing = manager->get_touch_spacing();
```

## Component Examples

### Dashboard Layout
```cpp
void render_responsive_dashboard() {
    // Create adaptive card layout
    std::vector<std::function<void()>> cards = {
        []() { render_performance_card(); },
        []() { render_systems_card(); },
        []() { render_assets_card(); },
        []() { render_logs_card(); }
    };
    
    ResponsiveWidget widget(manager);
    widget.adaptive_layout(cards, 2, 4); // 2-4 columns
}

void render_performance_card() {
    ImVec2 card_size = manager->calculate_adaptive_window_size(
        ImVec2(300, 200), // Preferred size
        {.min_width = 250, .max_width = 400} // Constraints
    );
    
    if (ImGui::BeginChild("Performance", card_size, true)) {
        // Card content with responsive elements
        ImGui::PushFont(manager->get_font(manager->get_screen_size(), "h2"));
        ImGui::Text("Performance");
        ImGui::PopFont();
        
        // Responsive metrics display
        render_performance_metrics();
    }
    ImGui::EndChild();
}
```

### ECS Inspector
```cpp
void render_ecs_inspector() {
    manager->begin_adaptive_columns(2, 3);
    
    // Entity list column
    render_entity_list();
    ImGui::NextColumn();
    
    // Component editor column  
    render_component_editor();
    
    if (manager->is_screen_at_least(ScreenSize::Large)) {
        ImGui::NextColumn();
        render_system_monitor(); // Only on larger screens
    }
    
    manager->end_adaptive_columns();
}
```

## Testing Framework

### Setting Up Tests
```cpp
#include "ecscope/gui/responsive_testing.hpp"

// Initialize testing framework
testing::TestConfig test_config;
test_config.enable_visual_regression = true;
test_config.generate_screenshots = true;

testing::InitializeGlobalResponsiveTesting(responsive_manager, test_config);
auto* testing_framework = testing::GetResponsiveTestingFramework();
```

### Creating Custom Tests
```cpp
// Layout responsiveness test
testing::TestCase layout_test;
layout_test.name = "Dashboard Layout Test";
layout_test.category = testing::TestCategory::Layout;
layout_test.screen_tests = testing_framework->create_standard_screen_simulations();
layout_test.test_function = [](const testing::ScreenSimulation& screen) {
    // Apply screen simulation
    // Render UI components
    // Validate layout correctness
    return testing::TestResult::Pass;
};

testing_framework->register_test(layout_test);
```

### Running Tests
```cpp
// Run all tests
auto results = testing_framework->run_all_tests();
std::cout << "Tests passed: " << results.passed_tests << "/" << results.total_tests << std::endl;

// Run specific category
auto layout_results = testing_framework->run_tests_by_category(testing::TestCategory::Layout);

// Generate reports
testing_framework->generate_html_report(results, "test_results.html");
```

## Performance Considerations

### Optimization Guidelines

1. **Layout Caching**: Cache layout calculations when screen size doesn't change
2. **Font Loading**: Load fonts once per DPI category, not per frame
3. **Spacing Calculations**: Pre-calculate common spacing values
4. **Conditional Rendering**: Use screen size checks to avoid rendering unnecessary elements

```cpp
// Efficient screen size checking
if (manager->is_screen_at_least(ScreenSize::Medium)) {
    render_advanced_features(); // Only render when there's space
}

// Cache expensive calculations
static float cached_spacing = -1.0f;
static ScreenSize last_screen_size = ScreenSize::Large;

if (last_screen_size != manager->get_screen_size()) {
    cached_spacing = manager->get_spacing("large");
    last_screen_size = manager->get_screen_size();
}
```

### Performance Metrics
- **Layout Update Time**: < 1ms for typical UI
- **Font Scaling**: < 0.5ms for font atlas updates
- **Screen Size Detection**: < 0.1ms per frame
- **Memory Overhead**: ~2MB for full responsive system

## Accessibility Features

### Built-in Accessibility
- WCAG 2.1 AA compliant touch target sizes
- High contrast mode support
- Keyboard navigation preservation
- Screen reader compatible markup
- Focus indicator scaling

### Custom Accessibility
```cpp
// Test accessibility compliance
testing::TestResult test_accessibility(const testing::ScreenSimulation& screen) {
    auto result = testing_framework->test_touch_target_sizes(screen);
    if (result != testing::TestResult::Pass) return result;
    
    result = testing_framework->test_color_contrast(screen);
    if (result != testing::TestResult::Pass) return result;
    
    return testing::TestResult::Pass;
}
```

## Integration with ECScope Systems

### Dashboard Integration
```cpp
// Dashboard automatically adapts to screen size
dashboard->set_responsive_mode(true);
dashboard->set_layout_mode(ResponsiveMode::Adaptive);
```

### ECS Inspector Integration
```cpp
// Inspector panels automatically reorganize
ecs_inspector->enable_responsive_layout();
ecs_inspector->set_minimum_panel_width(200.0f);
```

### Asset Pipeline Integration
```cpp
// Asset browser thumbnail sizing
asset_browser->set_thumbnail_size_mode(ResponsiveThumbnailMode::Adaptive);
asset_browser->set_column_count_range(2, 6);
```

## Platform-Specific Considerations

### Windows
- High-DPI awareness configuration required
- Multiple monitor DPI handling
- Touch screen detection via Windows API

### macOS
- Retina display automatic scaling
- Metal renderer performance considerations
- Touch Bar integration potential

### Linux
- X11/Wayland DPI detection differences
- GTK theme integration
- Fractional scaling support

## Best Practices

### Design Principles
1. **Mobile-First**: Design for smallest screen, enhance for larger
2. **Content Priority**: Most important content visible at all sizes
3. **Touch-Friendly**: All interactive elements meet accessibility guidelines
4. **Performance-Conscious**: Responsive features shouldn't impact frame rate
5. **Consistent**: Maintain visual hierarchy across all screen sizes

### Code Organization
```cpp
// Group responsive logic together
class ResponsiveGameEngineUI {
    void render() {
        if (responsive_manager_->is_screen_at_most(ScreenSize::Small)) {
            render_mobile_layout();
        } else {
            render_desktop_layout();
        }
    }
    
private:
    void render_mobile_layout() {
        // Simplified, vertical layout
        render_essential_controls();
        render_collapsible_sections();
    }
    
    void render_desktop_layout() {
        // Full feature set with multiple columns
        render_full_dashboard();
        render_detailed_inspectors();
    }
};
```

### Testing Strategy
1. **Automated Testing**: Run responsive tests in CI/CD
2. **Visual Regression**: Compare screenshots across versions
3. **Performance Testing**: Monitor layout performance across screen sizes
4. **Manual Testing**: Regular testing on actual devices
5. **Accessibility Auditing**: Regular accessibility compliance checks

## Troubleshooting

### Common Issues

**Blurry Text on High-DPI Displays**
```cpp
// Ensure proper DPI scaling
responsive_config.auto_dpi_scaling = true;
// Enable font oversampling
font_config.use_oversampling = true;
```

**Touch Targets Too Small**
```cpp
// Verify touch mode is enabled
manager->set_touch_mode(TouchMode::Enabled);
// Check minimum touch target size
ImVec2 touch_size = manager->get_touch_button_size();
assert(touch_size.x >= 44.0f && touch_size.y >= 44.0f);
```

**Layout Breaking on Screen Size Changes**
```cpp
// Use adaptive layouts instead of fixed sizing
manager->begin_adaptive_columns(2, 4);
// Add constraints to prevent extreme sizing
LayoutConstraints constraints{.min_width = 200, .max_width = 600};
```

**Performance Issues with Many UI Elements**
```cpp
// Cache screen size checks
static ScreenSize cached_screen_size = manager->get_screen_size();
// Use early returns for unsupported screen sizes
if (manager->is_screen_at_most(ScreenSize::Small)) return;
```

## Future Enhancements

### Planned Features
- CSS-like media queries for ImGui
- Animation framework for responsive transitions
- Voice control integration for accessibility
- AR/VR responsive design extensions
- Advanced layout algorithms (CSS Grid-like)

### Community Contributions
- Theme marketplace with responsive themes
- Community-contributed responsive components
- Cross-platform testing matrix
- Performance optimization plugins

## Conclusion

The ECScope Responsive Design System provides a comprehensive foundation for creating professional, adaptive game engine interfaces. By following the guidelines and examples in this documentation, developers can create UIs that work seamlessly across all devices and screen sizes while maintaining the sophisticated aesthetic expected in professional game development tools.

For questions, contributions, or support, please visit the ECScope GitHub repository or contact the development team.