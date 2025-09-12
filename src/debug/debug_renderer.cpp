// Debug Renderer Implementation
#include "ecscope/debug/debug_renderer.hpp"
#include <iostream>

namespace ecscope::debug {

DebugRenderer::DebugRenderer() = default;
DebugRenderer::~DebugRenderer() = default;

bool DebugRenderer::initialize() {
    is_initialized_ = true;
    return true;
}

void DebugRenderer::shutdown() {
    is_initialized_ = false;
    debug_lines_.clear();
    debug_points_.clear();
}

void DebugRenderer::draw_line(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    if (!is_initialized_) return;
    
    DebugLine line;
    line.start = start;
    line.end = end;
    line.color = color;
    debug_lines_.push_back(line);
}

void DebugRenderer::draw_point(const glm::vec3& position, const glm::vec3& color, float size) {
    if (!is_initialized_) return;
    
    DebugPoint point;
    point.position = position;
    point.color = color;
    point.size = size;
    debug_points_.push_back(point);
}

void DebugRenderer::draw_sphere(const glm::vec3& center, float radius, const glm::vec3& color) {
    if (!is_initialized_) return;
    
    // Draw sphere using lines (simplified wireframe)
    const int segments = 16;
    const float angle_step = 2.0f * 3.14159f / segments;
    
    // Draw horizontal circles
    for (int ring = 0; ring < segments / 2; ++ring) {
        float y = center.y + radius * std::cos(ring * angle_step);
        float ring_radius = radius * std::sin(ring * angle_step);
        
        for (int i = 0; i < segments; ++i) {
            float angle1 = i * angle_step;
            float angle2 = (i + 1) * angle_step;
            
            glm::vec3 p1(center.x + ring_radius * std::cos(angle1), y, center.z + ring_radius * std::sin(angle1));
            glm::vec3 p2(center.x + ring_radius * std::cos(angle2), y, center.z + ring_radius * std::sin(angle2));
            
            draw_line(p1, p2, color);
        }
    }
}

void DebugRenderer::draw_box(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color) {
    if (!is_initialized_) return;
    
    // Draw box wireframe
    glm::vec3 corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {max.x, max.y, min.z}, {min.x, max.y, min.z},
        {min.x, min.y, max.z}, {max.x, min.y, max.z},
        {max.x, max.y, max.z}, {min.x, max.y, max.z}
    };
    
    // Bottom face
    draw_line(corners[0], corners[1], color);
    draw_line(corners[1], corners[2], color);
    draw_line(corners[2], corners[3], color);
    draw_line(corners[3], corners[0], color);
    
    // Top face
    draw_line(corners[4], corners[5], color);
    draw_line(corners[5], corners[6], color);
    draw_line(corners[6], corners[7], color);
    draw_line(corners[7], corners[4], color);
    
    // Vertical edges
    draw_line(corners[0], corners[4], color);
    draw_line(corners[1], corners[5], color);
    draw_line(corners[2], corners[6], color);
    draw_line(corners[3], corners[7], color);
}

void DebugRenderer::render() {
    if (!is_initialized_) return;
    
    // In a real implementation, this would render using OpenGL/Vulkan
    // For now, just output debug info
    if (!debug_lines_.empty() || !debug_points_.empty()) {
        std::cout << "Debug Renderer: " << debug_lines_.size() << " lines, " 
                  << debug_points_.size() << " points" << std::endl;
    }
}

void DebugRenderer::clear() {
    debug_lines_.clear();
    debug_points_.clear();
}

} // namespace ecscope::debug