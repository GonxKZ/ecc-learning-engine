#include "../include/ecscope/visual_shader_editor.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>

// JSON library for serialization (would typically use nlohmann/json or similar)
#include <string>
#include <regex>

namespace ecscope::renderer::visual_editor {

//=============================================================================
// Visual Shader Graph Implementation
//=============================================================================

std::string VisualShaderGraph::serialize_to_json() const {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"version\": \"1.0\",\n";
    json << "  \"name\": \"" << name << "\",\n";
    json << "  \"description\": \"" << description << "\",\n";
    json << "  \"target_stage\": " << static_cast<int>(target_stage) << ",\n";
    json << "  \"nodes\": [\n";
    
    bool first_node = true;
    for (const auto& [id, node] : nodes_) {
        if (!first_node) json << ",\n";
        first_node = false;
        
        json << "    {\n";
        json << "      \"id\": " << id << ",\n";
        json << "      \"type\": \"" << get_node_type_string(node.get()) << "\",\n";
        json << "      \"name\": \"" << node->name << "\",\n";
        json << "      \"position\": {\"x\": " << node->x_position << ", \"y\": " << node->y_position << "},\n";
        json << "      \"properties\": {\n";
        
        // Serialize node-specific properties
        serialize_node_properties(node.get(), json);
        
        json << "      }\n";
        json << "    }";
    }
    
    json << "\n  ],\n";
    json << "  \"connections\": [\n";
    
    bool first_conn = true;
    for (const auto& conn : connections_) {
        if (!first_conn) json << ",\n";
        first_conn = false;
        
        json << "    {\n";
        json << "      \"from_node\": " << conn.from_node_id << ",\n";
        json << "      \"from_pin\": \"" << conn.from_pin << "\",\n";
        json << "      \"to_node\": " << conn.to_node_id << ",\n";
        json << "      \"to_pin\": \"" << conn.to_pin << "\"\n";
        json << "    }";
    }
    
    json << "\n  ]\n";
    json << "}\n";
    
    return json.str();
}

bool VisualShaderGraph::deserialize_from_json(const std::string& json_data) {
    // Simple JSON parsing implementation
    // In a real implementation, you'd use a proper JSON library
    
    try {
        // Clear existing data
        nodes_.clear();
        connections_.clear();
        next_node_id_ = 1;
        
        // Parse basic info (simplified parsing)
        auto name_pos = json_data.find("\"name\":");
        if (name_pos != std::string::npos) {
            auto start = json_data.find("\"", name_pos + 7) + 1;
            auto end = json_data.find("\"", start);
            name = json_data.substr(start, end - start);
        }
        
        // Parse target stage
        auto stage_pos = json_data.find("\"target_stage\":");
        if (stage_pos != std::string::npos) {
            auto start = json_data.find_first_of("0123456789", stage_pos + 15);
            if (start != std::string::npos) {
                int stage_val = std::stoi(json_data.substr(start, 1));
                target_stage = static_cast<resources::ShaderStage>(stage_val);
            }
        }
        
        // For a complete implementation, you would parse nodes and connections here
        // This is simplified for demonstration purposes
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

std::string VisualShaderGraph::get_node_type_string(const VisualShaderNode* node) const {
    if (dynamic_cast<const AttributeInputNode*>(node)) return "AttributeInput";
    if (dynamic_cast<const UniformInputNode*>(node)) return "UniformInput";
    if (dynamic_cast<const MathOperationNode*>(node)) return "MathOperation";
    if (dynamic_cast<const TextureSampleNode*>(node)) return "TextureSample";
    if (dynamic_cast<const FragmentOutputNode*>(node)) return "FragmentOutput";
    return "Unknown";
}

void VisualShaderGraph::serialize_node_properties(const VisualShaderNode* node, 
                                                  std::ostringstream& json) const {
    // Serialize common properties
    json << "        \"category\": " << static_cast<int>(node->category);
    
    // Serialize node-specific properties
    if (const auto* attr_node = dynamic_cast<const AttributeInputNode*>(node)) {
        json << ",\n        \"attribute_type\": " << static_cast<int>(attr_node->attribute_type);
    }
    else if (const auto* uniform_node = dynamic_cast<const UniformInputNode*>(node)) {
        json << ",\n        \"uniform_name\": \"" << uniform_node->uniform_name << "\",\n";
        json << "        \"uniform_type\": " << static_cast<int>(uniform_node->uniform_type);
    }
    else if (const auto* math_node = dynamic_cast<const MathOperationNode*>(node)) {
        json << ",\n        \"operation\": " << static_cast<int>(math_node->operation);
    }
    else if (const auto* tex_node = dynamic_cast<const TextureSampleNode*>(node)) {
        json << ",\n        \"texture_path\": \"" << tex_node->texture_path << "\",\n";
        json << "        \"use_mipmaps\": " << (tex_node->use_mipmaps ? "true" : "false");
    }
    
    json << "\n";
}

//=============================================================================
// Visual Shader Editor Implementation
//=============================================================================

VisualShaderEditor::VisualShaderEditor(shader_compiler::AdvancedShaderCompiler* compiler)
    : compiler_(compiler), current_graph_(std::make_unique<VisualShaderGraph>()) {
    
    last_compile_time_ = std::chrono::steady_clock::now();
    
    // Initialize with a basic fragment shader graph
    new_graph(resources::ShaderStage::Fragment);
}

void VisualShaderEditor::new_graph(resources::ShaderStage stage) {
    current_graph_ = std::make_unique<VisualShaderGraph>();
    current_graph_->target_stage = stage;
    current_graph_->name = (stage == resources::ShaderStage::Fragment) ? 
                           "New Fragment Shader" : "New Vertex Shader";
    
    // Add default nodes for a basic setup
    if (stage == resources::ShaderStage::Fragment) {
        // Create a basic fragment shader setup
        auto output_node = std::make_unique<FragmentOutputNode>(1);
        output_node->x_position = 400.0f;
        output_node->y_position = 200.0f;
        current_graph_->add_node(std::move(output_node));
        
        // Add a texture sample node
        auto texture_node = std::make_unique<TextureSampleNode>(2);
        texture_node->x_position = 200.0f;
        texture_node->y_position = 150.0f;
        current_graph_->add_node(std::move(texture_node));
        
        // Add UV input
        auto uv_node = std::make_unique<AttributeInputNode>(3);
        uv_node->attribute_type = AttributeInputNode::AttributeType::UV;
        uv_node->x_position = 50.0f;
        uv_node->y_position = 100.0f;
        current_graph_->add_node(std::move(uv_node));
        
        // Connect UV to texture sample
        current_graph_->add_connection(3, "UV", 2, "UV");
        // Connect texture to output
        current_graph_->add_connection(2, "Color", 1, "Color");
    }
    else if (stage == resources::ShaderStage::Vertex) {
        // Create a basic vertex shader setup
        // Position input
        auto pos_node = std::make_unique<AttributeInputNode>(1);
        pos_node->attribute_type = AttributeInputNode::AttributeType::Position;
        pos_node->x_position = 50.0f;
        pos_node->y_position = 100.0f;
        current_graph_->add_node(std::move(pos_node));
        
        // MVP matrix
        auto mvp_node = std::make_unique<UniformInputNode>(2);
        mvp_node->uniform_name = "u_mvp_matrix";
        mvp_node->uniform_type = DataType::Mat4;
        mvp_node->x_position = 50.0f;
        mvp_node->y_position = 200.0f;
        current_graph_->add_node(std::move(mvp_node));
    }
    
    if (config_.enable_real_time_compilation) {
        update_real_time_compilation();
    }
}

bool VisualShaderEditor::load_graph(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) return false;
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    auto new_graph = std::make_unique<VisualShaderGraph>();
    if (new_graph->deserialize_from_json(content)) {
        current_graph_ = std::move(new_graph);
        return true;
    }
    
    return false;
}

bool VisualShaderEditor::save_graph(const std::string& file_path) {
    if (!current_graph_) return false;
    
    std::ofstream file(file_path);
    if (!file.is_open()) return false;
    
    file << current_graph_->serialize_to_json();
    file.close();
    
    return true;
}

u32 VisualShaderEditor::add_node(std::unique_ptr<VisualShaderNode> node, f32 x, f32 y) {
    if (!current_graph_) return 0;
    
    node->x_position = x;
    node->y_position = y;
    u32 node_id = current_graph_->add_node(std::move(node));
    
    if (config_.enable_real_time_compilation) {
        update_real_time_compilation();
    }
    
    return node_id;
}

void VisualShaderEditor::remove_selected_nodes() {
    if (!current_graph_) return;
    
    for (u32 node_id : current_graph_->selected_nodes_) {
        current_graph_->remove_node(node_id);
    }
    current_graph_->selected_nodes_.clear();
    
    if (config_.enable_real_time_compilation) {
        update_real_time_compilation();
    }
}

void VisualShaderEditor::duplicate_selected_nodes() {
    if (!current_graph_) return;
    
    std::vector<u32> new_node_ids;
    f32 offset = 50.0f;
    
    for (u32 node_id : current_graph_->selected_nodes_) {
        auto* original_node = current_graph_->get_node(node_id);
        if (original_node) {
            auto cloned_node = original_node->clone();
            cloned_node->x_position = original_node->x_position + offset;
            cloned_node->y_position = original_node->y_position + offset;
            cloned_node->is_selected = false;
            
            u32 new_id = current_graph_->add_node(std::move(cloned_node));
            new_node_ids.push_back(new_id);
        }
    }
    
    // Select the new nodes
    current_graph_->selected_nodes_ = new_node_ids;
}

bool VisualShaderEditor::create_connection(u32 from_node, const std::string& from_pin,
                                          u32 to_node, const std::string& to_pin) {
    if (!current_graph_) return false;
    
    bool result = current_graph_->add_connection(from_node, from_pin, to_node, to_pin);
    
    if (result && config_.enable_real_time_compilation) {
        update_real_time_compilation();
    }
    
    return result;
}

void VisualShaderEditor::remove_connection(u32 connection_id) {
    if (!current_graph_) return;
    
    current_graph_->remove_connection(connection_id);
    
    if (config_.enable_real_time_compilation) {
        update_real_time_compilation();
    }
}

shader_compiler::CompilationResult VisualShaderEditor::compile_current_graph() {
    if (!current_graph_ || !compiler_) {
        shader_compiler::CompilationResult result;
        result.success = false;
        result.add_diagnostic(shader_compiler::CompilationDiagnostic::Severity::Error,
                            "No graph to compile or compiler not available");
        return result;
    }
    
    std::string glsl_code = current_graph_->compile_to_glsl();
    
    auto result = compiler_->compile_shader(
        glsl_code,
        current_graph_->target_stage,
        "main",
        "visual_shader_graph"
    );
    
    last_compilation_ = result;
    return result;
}

std::string VisualShaderEditor::get_generated_code() const {
    if (!current_graph_) return "";
    return current_graph_->compile_to_glsl();
}

bool VisualShaderEditor::enable_live_preview(bool enabled) {
    config_.enable_real_time_compilation = enabled;
    
    if (enabled) {
        update_real_time_compilation();
    }
    
    return true;
}

void VisualShaderEditor::handle_mouse_input(f32 x, f32 y, bool left_button, bool right_button) {
    if (!current_graph_) return;
    
    mouse_position_ = {x, y};
    
    if (left_button && !is_dragging_node_) {
        // Check if clicking on a node
        for (const auto& [id, node] : current_graph_->nodes_) {
            if (node->contains_point(x, y)) {
                is_dragging_node_ = true;
                dragged_node_id_ = id;
                drag_offset_ = {x - node->x_position, y - node->y_position};
                
                // Select the node
                if (std::find(current_graph_->selected_nodes_.begin(),
                             current_graph_->selected_nodes_.end(), id) == 
                    current_graph_->selected_nodes_.end()) {
                    
                    current_graph_->selected_nodes_.clear();
                    current_graph_->selected_nodes_.push_back(id);
                    node->is_selected = true;
                }
                break;
            }
        }
    }
    
    if (is_dragging_node_ && left_button) {
        // Update node position
        auto* node = current_graph_->get_node(dragged_node_id_);
        if (node) {
            node->x_position = x - drag_offset_[0];
            node->y_position = y - drag_offset_[1];
            
            if (config_.snap_to_grid) {
                node->x_position = std::round(node->x_position / config_.grid_size) * config_.grid_size;
                node->y_position = std::round(node->y_position / config_.grid_size) * config_.grid_size;
            }
        }
    }
    
    if (!left_button) {
        is_dragging_node_ = false;
        dragged_node_id_ = 0;
    }
    
    if (right_button) {
        // Right-click context menu handling
        // Implementation would show context menu for creating nodes, etc.
    }
}

void VisualShaderEditor::handle_keyboard_input(i32 key, bool pressed) {
    if (!pressed) return;
    
    // Handle keyboard shortcuts
    switch (key) {
        case 'D': // Delete key equivalent
            if (!current_graph_->selected_nodes_.empty()) {
                remove_selected_nodes();
            }
            break;
            
        case 'C': // Copy
            // Implementation for copying nodes
            break;
            
        case 'V': // Paste
            // Implementation for pasting nodes
            break;
            
        case 'A': // Select all
            current_graph_->selected_nodes_.clear();
            for (const auto& [id, node] : current_graph_->nodes_) {
                current_graph_->selected_nodes_.push_back(id);
                node->is_selected = true;
            }
            break;
            
        case 'F': // Frame all nodes
            // Implementation for framing all nodes in view
            break;
            
        default:
            break;
    }
}

void VisualShaderEditor::show_tutorial_overlay(bool show) {
    show_tutorial_ = show;
}

void VisualShaderEditor::highlight_data_flow(bool highlight) {
    highlight_flow_ = highlight;
}

std::string VisualShaderEditor::get_current_explanation() const {
    if (!current_graph_) return "";
    return current_graph_->generate_explanation();
}

std::vector<std::string> VisualShaderEditor::get_performance_tips() const {
    if (!current_graph_) return {};
    
    auto tips = current_graph_->get_optimization_suggestions();
    
    // Add editor-specific tips
    if (last_compilation_.success) {
        if (last_compilation_.performance.compilation_time > 100.0f) {
            tips.push_back("Compilation time is high - consider simplifying the shader");
        }
        
        if (last_compilation_.performance.estimated_gpu_cost > 3.0f) {
            tips.push_back("High GPU cost detected - optimize for better performance");
        }
    }
    
    return tips;
}

void VisualShaderEditor::update_real_time_compilation() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_compile_time_);
    
    // Compile every 500ms to avoid too frequent compilation
    if (elapsed.count() > 500) {
        compile_current_graph();
        last_compile_time_ = now;
    }
}

void VisualShaderEditor::render_editor() {
    // This would integrate with your UI system (ImGui, custom UI, etc.)
    // Implementation would render the graph editor interface
    
    if (!current_graph_) return;
    
    // Render grid if enabled
    if (config_.show_grid) {
        render_grid();
    }
    
    // Render connections
    for (const auto& connection : current_graph_->connections_) {
        render_connection(connection);
    }
    
    // Render nodes
    for (const auto& [id, node] : current_graph_->nodes_) {
        render_node(*node);
    }
    
    // Render overlays
    if (show_tutorial_) {
        render_tutorial_overlay();
    }
    
    if (config_.show_performance_overlay) {
        render_performance_overlay();
    }
    
    if (config_.show_minimap) {
        render_minimap();
    }
}

void VisualShaderEditor::render_node(const VisualShaderNode& node) {
    // Node rendering implementation
    // Would draw the node box, pins, labels, etc.
    // This is a placeholder for the actual rendering code
}

void VisualShaderEditor::render_connection(const NodeConnection& connection) {
    // Connection rendering implementation
    // Would draw bezier curves between connected pins
    // This is a placeholder for the actual rendering code
}

void VisualShaderEditor::render_grid() {
    // Grid rendering implementation
    // This is a placeholder for the actual rendering code
}

void VisualShaderEditor::render_minimap() {
    // Minimap rendering implementation
    // This is a placeholder for the actual rendering code
}

void VisualShaderEditor::render_tutorial_overlay() {
    // Tutorial overlay rendering implementation
    // This is a placeholder for the actual rendering code
}

void VisualShaderEditor::render_performance_overlay() {
    // Performance overlay rendering implementation
    // Would show compilation time, node count, etc.
    // This is a placeholder for the actual rendering code
}

std::unique_ptr<VisualShaderNode> VisualShaderEditor::create_node_by_type(const std::string& node_type) {
    if (node_type == "AttributeInput") {
        return std::make_unique<AttributeInputNode>(0);
    } else if (node_type == "UniformInput") {
        return std::make_unique<UniformInputNode>(0);
    } else if (node_type == "MathOperation") {
        return std::make_unique<MathOperationNode>(0);
    } else if (node_type == "TextureSample") {
        return std::make_unique<TextureSampleNode>(0);
    } else if (node_type == "FragmentOutput") {
        return std::make_unique<FragmentOutputNode>(0);
    }
    
    return nullptr;
}

//=============================================================================
// Node Factory Implementation
//=============================================================================

std::unique_ptr<VisualShaderNode> NodeFactory::create_attribute_input(u32 id, AttributeInputNode::AttributeType type) {
    auto node = std::make_unique<AttributeInputNode>(id);
    node->attribute_type = type;
    
    // Update name based on type
    switch (type) {
        case AttributeInputNode::AttributeType::Position:
            node->name = "Position";
            node->description = "Vertex position attribute";
            break;
        case AttributeInputNode::AttributeType::Normal:
            node->name = "Normal";
            node->description = "Vertex normal attribute";
            break;
        case AttributeInputNode::AttributeType::UV:
            node->name = "UV";
            node->description = "UV texture coordinates";
            break;
        case AttributeInputNode::AttributeType::Color:
            node->name = "Vertex Color";
            node->description = "Per-vertex color attribute";
            break;
        default:
            break;
    }
    
    return std::move(node);
}

std::unique_ptr<VisualShaderNode> NodeFactory::create_uniform_input(u32 id, const std::string& name, DataType type) {
    auto node = std::make_unique<UniformInputNode>(id);
    node->uniform_name = name;
    node->uniform_type = type;
    node->name = "Uniform: " + name;
    node->description = "Uniform input of type " + std::to_string(static_cast<int>(type));
    
    return std::move(node);
}

std::unique_ptr<VisualShaderNode> NodeFactory::create_math_operation(u32 id, MathOperationNode::Operation op) {
    auto node = std::make_unique<MathOperationNode>(id);
    node->operation = op;
    
    // Update name and description based on operation
    switch (op) {
        case MathOperationNode::Operation::Add:
            node->name = "Add";
            node->description = "Add two values together";
            break;
        case MathOperationNode::Operation::Multiply:
            node->name = "Multiply";
            node->description = "Multiply two values";
            break;
        case MathOperationNode::Operation::Normalize:
            node->name = "Normalize";
            node->description = "Normalize a vector to unit length";
            break;
        case MathOperationNode::Operation::Dot:
            node->name = "Dot Product";
            node->description = "Calculate dot product of two vectors";
            break;
        default:
            node->name = "Math Operation";
            break;
    }
    
    return std::move(node);
}

std::unique_ptr<VisualShaderNode> NodeFactory::create_texture_sample(u32 id) {
    auto node = std::make_unique<TextureSampleNode>(id);
    return std::move(node);
}

std::unique_ptr<VisualShaderNode> NodeFactory::create_fragment_output(u32 id) {
    auto node = std::make_unique<FragmentOutputNode>(id);
    return std::move(node);
}

std::vector<std::string> NodeFactory::get_available_node_types() {
    return {
        "AttributeInput", "UniformInput", "MathOperation", 
        "TextureSample", "FragmentOutput", "VertexOutput",
        "ConstantFloat", "ConstantVector", "ConstantColor",
        "NoisePerlin", "NoiseSimple", "NoiseFBM",
        "VectorSplit", "VectorCombine", "VectorTransform",
        "ColorRGB", "ColorHSV", "ColorMix",
        "ConditionalIf", "ConditionalSwitch",
        "Custom"
    };
}

std::string NodeFactory::get_node_category(const std::string& node_type) {
    if (node_type.find("Input") != std::string::npos) return "Input";
    if (node_type.find("Output") != std::string::npos) return "Output";
    if (node_type.find("Math") != std::string::npos) return "Math";
    if (node_type.find("Vector") != std::string::npos) return "Vector";
    if (node_type.find("Color") != std::string::npos) return "Color";
    if (node_type.find("Noise") != std::string::npos) return "Noise";
    if (node_type.find("Texture") != std::string::npos) return "Texture";
    if (node_type.find("Constant") != std::string::npos) return "Constants";
    if (node_type.find("Conditional") != std::string::npos) return "Flow";
    return "Utility";
}

std::string NodeFactory::get_node_description(const std::string& node_type) {
    static std::unordered_map<std::string, std::string> descriptions = {
        {"AttributeInput", "Provides vertex attribute data from the mesh"},
        {"UniformInput", "Provides uniform values constant across draw calls"},
        {"MathOperation", "Performs mathematical operations on input values"},
        {"TextureSample", "Samples a texture at given UV coordinates"},
        {"FragmentOutput", "Final output for fragment shaders"},
        {"ConstantFloat", "Provides a constant floating-point value"},
        {"ConstantVector", "Provides a constant vector value"},
        {"NoisePerlin", "Generates Perlin noise"},
        {"VectorSplit", "Splits a vector into individual components"},
        {"VectorCombine", "Combines individual values into a vector"},
        {"ColorRGB", "Works with RGB color values"},
        {"ConditionalIf", "Conditional branching based on a condition"}
    };
    
    auto it = descriptions.find(node_type);
    return (it != descriptions.end()) ? it->second : "Unknown node type";
}

} // namespace ecscope::renderer::visual_editor