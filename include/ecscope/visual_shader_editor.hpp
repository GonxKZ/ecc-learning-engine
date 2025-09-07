#pragma once

/**
 * @file visual_shader_editor.hpp
 * @brief Visual Node-Based Shader Editor System for ECScope
 * 
 * This system provides a complete visual shader editor with:
 * - Node-based visual programming interface
 * - Real-time shader compilation and preview
 * - Comprehensive shader node library
 * - Advanced graph editing capabilities
 * - Export to GLSL/HLSL/SPIR-V
 * - Educational shader learning tools
 * - Performance analysis and optimization hints
 * - Integration with the ECScope shader system
 * 
 * @author ECScope Educational ECS Framework - Advanced Shader System
 * @date 2024
 */

#include "advanced_shader_compiler.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>
#include <array>
#include <optional>

namespace ecscope::renderer::visual_editor {

//=============================================================================
// Data Types and Values for Visual Shader Nodes
//=============================================================================

enum class DataType : u8 {
    Float = 0,
    Vec2,
    Vec3, 
    Vec4,
    Int,
    IVec2,
    IVec3,
    IVec4,
    Bool,
    BVec2,
    BVec3,
    BVec4,
    Mat2,
    Mat3,
    Mat4,
    Sampler2D,
    SamplerCube,
    Texture2D,
    TextureCube,
    Color,              ///< Special type for color picker
    UV,                 ///< Special type for UV coordinates
    Normal,             ///< Special type for normal vectors
    Tangent,            ///< Special type for tangent vectors
    Position,           ///< Special type for positions
    Custom              ///< User-defined custom type
};

using ShaderValue = std::variant<
    f32,                        ///< Float value
    std::array<f32, 2>,         ///< Vec2 value
    std::array<f32, 3>,         ///< Vec3 value
    std::array<f32, 4>,         ///< Vec4 value
    i32,                        ///< Int value
    std::array<i32, 2>,         ///< IVec2 value
    std::array<i32, 3>,         ///< IVec3 value
    std::array<i32, 4>,         ///< IVec4 value
    bool,                       ///< Bool value
    std::array<bool, 2>,        ///< BVec2 value
    std::array<bool, 3>,        ///< BVec3 value
    std::array<bool, 4>,        ///< BVec4 value
    std::array<f32, 4>,         ///< Mat2 (stored as 4 floats)
    std::array<f32, 9>,         ///< Mat3 (stored as 9 floats)
    std::array<f32, 16>,        ///< Mat4 (stored as 16 floats)
    std::string                 ///< String for texture paths, etc.
>;

//=============================================================================
// Node Input/Output System
//=============================================================================

struct NodePin {
    enum class Direction {
        Input = 0,
        Output
    };
    
    std::string name;               ///< Display name of the pin
    std::string internal_name;      ///< Internal variable name in shader
    DataType type;                  ///< Data type of this pin
    Direction direction;            ///< Input or output
    ShaderValue default_value;      ///< Default value if not connected
    bool is_connected{false};       ///< Whether this pin has connections
    bool is_required{true};         ///< Whether this input is required
    std::string tooltip;            ///< Help text for the pin
    std::string semantic;           ///< Semantic hint (e.g., "POSITION", "COLOR")
    
    // Visual properties
    std::array<f32, 4> color{1.0f, 1.0f, 1.0f, 1.0f}; ///< Pin color for visual distinction
    f32 radius{5.0f};               ///< Visual radius
    bool is_visible{true};          ///< Whether to show this pin
    
    NodePin() = default;
    NodePin(std::string n, DataType t, Direction d, ShaderValue def_val = f32{0.0f})
        : name(std::move(n)), internal_name(name), type(t), direction(d), default_value(def_val) {
        set_default_color();
    }
    
    void set_default_color() {
        // Set default colors based on data type
        switch (type) {
            case DataType::Float: color = {0.6f, 0.6f, 0.6f, 1.0f}; break;
            case DataType::Vec2: color = {0.8f, 0.6f, 0.4f, 1.0f}; break;
            case DataType::Vec3: color = {0.8f, 0.4f, 0.6f, 1.0f}; break;
            case DataType::Vec4: color = {0.6f, 0.4f, 0.8f, 1.0f}; break;
            case DataType::Color: color = {1.0f, 0.3f, 0.3f, 1.0f}; break;
            case DataType::UV: color = {0.3f, 1.0f, 0.3f, 1.0f}; break;
            case DataType::Normal: color = {0.3f, 0.3f, 1.0f, 1.0f}; break;
            case DataType::Sampler2D: color = {1.0f, 1.0f, 0.3f, 1.0f}; break;
            default: color = {0.7f, 0.7f, 0.7f, 1.0f}; break;
        }
    }
};

//=============================================================================
// Connection System
//=============================================================================

struct NodeConnection {
    u32 id;                         ///< Unique connection ID
    u32 from_node_id;              ///< Source node ID
    std::string from_pin;           ///< Source pin name
    u32 to_node_id;                ///< Destination node ID
    std::string to_pin;             ///< Destination pin name
    DataType data_type;             ///< Type of data flowing through connection
    bool is_valid{true};            ///< Whether connection is type-compatible
    
    // Visual properties
    std::array<f32, 4> color{1.0f, 1.0f, 1.0f, 1.0f}; ///< Connection line color
    f32 thickness{2.0f};            ///< Line thickness
    bool is_highlighted{false};     ///< Whether connection is highlighted
    
    NodeConnection(u32 from_node, std::string from_p, u32 to_node, std::string to_p, DataType type)
        : from_node_id(from_node), from_pin(std::move(from_p)), 
          to_node_id(to_node), to_pin(std::move(to_p)), data_type(type) {
        id = generate_id();
    }
    
    bool connects_nodes(u32 node1, u32 node2) const {
        return (from_node_id == node1 && to_node_id == node2) ||
               (from_node_id == node2 && to_node_id == node1);
    }
    
private:
    static u32 generate_id() {
        static u32 next_id = 1;
        return next_id++;
    }
};

//=============================================================================
// Visual Shader Node Base Class and Node Types
//=============================================================================

class VisualShaderNode {
public:
    enum class Category {
        Input = 0,          ///< Input nodes (attributes, uniforms)
        Output,             ///< Output nodes (final values)
        Constants,          ///< Constant values
        Math,               ///< Mathematical operations
        Vector,             ///< Vector operations
        Matrix,             ///< Matrix operations
        Trigonometry,       ///< Trigonometric functions
        Texture,            ///< Texture sampling
        Noise,              ///< Noise functions
        Utility,            ///< Utility functions
        Flow,               ///< Control flow
        Custom,             ///< User-defined functions
        Educational         ///< Educational demonstration nodes
    };
    
    u32 id;                         ///< Unique node ID
    std::string name;               ///< Display name
    std::string description;        ///< Detailed description
    Category category;              ///< Node category
    std::string shader_code;        ///< GLSL code template
    
    // Visual properties
    f32 x_position{0.0f}, y_position{0.0f}; ///< Position in editor
    f32 width{120.0f}, height{60.0f};       ///< Visual size
    std::array<f32, 4> color{0.2f, 0.2f, 0.2f, 1.0f}; ///< Node color
    bool is_selected{false};        ///< Whether node is selected
    bool is_hovered{false};         ///< Whether node is being hovered
    bool is_collapsed{false};       ///< Whether node is collapsed
    
    // Pins
    std::vector<NodePin> input_pins;
    std::vector<NodePin> output_pins;
    
    // Properties and parameters
    std::unordered_map<std::string, ShaderValue> properties;
    std::unordered_map<std::string, std::string> metadata;
    
    // Educational features
    std::string help_text;          ///< Educational explanation
    std::string code_explanation;   ///< Explanation of generated code
    std::vector<std::string> tips;  ///< Usage tips
    bool is_beginner_friendly{true}; ///< Whether suitable for beginners
    
    VisualShaderNode(u32 node_id, std::string node_name, Category cat)
        : id(node_id), name(std::move(node_name)), category(cat) {
        set_default_color();
    }
    
    virtual ~VisualShaderNode() = default;
    
    // Core functionality
    virtual std::string generate_code(resources::ShaderStage stage) const = 0;
    virtual bool validate() const { return true; }
    virtual void on_property_changed(const std::string& property_name) {}
    virtual std::unique_ptr<VisualShaderNode> clone() const = 0;
    
    // Pin management
    void add_input_pin(const std::string& name, DataType type, ShaderValue default_value = f32{0.0f}) {
        input_pins.emplace_back(name, type, NodePin::Direction::Input, default_value);
    }
    
    void add_output_pin(const std::string& name, DataType type) {
        output_pins.emplace_back(name, type, NodePin::Direction::Output);
    }
    
    NodePin* find_pin(const std::string& pin_name, NodePin::Direction direction) {
        auto& pins = (direction == NodePin::Direction::Input) ? input_pins : output_pins;
        auto it = std::find_if(pins.begin(), pins.end(),
                              [&](const NodePin& pin) { return pin.name == pin_name; });
        return (it != pins.end()) ? &(*it) : nullptr;
    }
    
    bool can_connect_to(const VisualShaderNode& other, const std::string& my_pin, 
                       const std::string& other_pin) const {
        // Type compatibility checking
        auto* my_out_pin = const_cast<VisualShaderNode*>(this)->find_pin(my_pin, NodePin::Direction::Output);
        auto* other_in_pin = const_cast<VisualShaderNode&>(other).find_pin(other_pin, NodePin::Direction::Input);
        
        if (!my_out_pin || !other_in_pin) return false;
        
        return is_type_compatible(my_out_pin->type, other_in_pin->type);
    }
    
    // Property management
    void set_property(const std::string& name, const ShaderValue& value) {
        properties[name] = value;
        on_property_changed(name);
    }
    
    template<typename T>
    std::optional<T> get_property(const std::string& name) const {
        auto it = properties.find(name);
        if (it != properties.end()) {
            if (std::holds_alternative<T>(it->second)) {
                return std::get<T>(it->second);
            }
        }
        return std::nullopt;
    }
    
    // Visual operations
    bool contains_point(f32 x, f32 y) const {
        return x >= x_position && x <= x_position + width &&
               y >= y_position && y <= y_position + height;
    }
    
    std::array<f32, 2> get_pin_position(const std::string& pin_name, NodePin::Direction direction) const {
        const auto& pins = (direction == NodePin::Direction::Input) ? input_pins : output_pins;
        auto it = std::find_if(pins.begin(), pins.end(),
                              [&](const NodePin& pin) { return pin.name == pin_name; });
        
        if (it != pins.end()) {
            f32 pin_index = static_cast<f32>(std::distance(pins.begin(), it));
            f32 pin_spacing = height / static_cast<f32>(pins.size() + 1);
            f32 x = (direction == NodePin::Direction::Input) ? x_position : x_position + width;
            f32 y = y_position + pin_spacing * (pin_index + 1);
            return {x, y};
        }
        
        return {x_position + width * 0.5f, y_position + height * 0.5f};
    }
    
protected:
    void set_default_color() {
        switch (category) {
            case Category::Input: color = {0.2f, 0.3f, 0.6f, 1.0f}; break;
            case Category::Output: color = {0.6f, 0.3f, 0.2f, 1.0f}; break;
            case Category::Constants: color = {0.3f, 0.3f, 0.3f, 1.0f}; break;
            case Category::Math: color = {0.3f, 0.5f, 0.3f, 1.0f}; break;
            case Category::Vector: color = {0.5f, 0.3f, 0.5f, 1.0f}; break;
            case Category::Texture: color = {0.6f, 0.5f, 0.2f, 1.0f}; break;
            case Category::Noise: color = {0.4f, 0.4f, 0.6f, 1.0f}; break;
            case Category::Educational: color = {0.2f, 0.6f, 0.4f, 1.0f}; break;
            default: color = {0.4f, 0.4f, 0.4f, 1.0f}; break;
        }
    }
    
private:
    static bool is_type_compatible(DataType from, DataType to) {
        if (from == to) return true;
        
        // Allow implicit conversions
        if (from == DataType::Float) {
            return to == DataType::Vec2 || to == DataType::Vec3 || to == DataType::Vec4;
        }
        if (from == DataType::Vec3 && to == DataType::Vec4) return true;
        if (from == DataType::Vec2 && (to == DataType::Vec3 || to == DataType::Vec4)) return true;
        
        // Color/UV special cases
        if (from == DataType::Color && (to == DataType::Vec3 || to == DataType::Vec4)) return true;
        if (from == DataType::UV && to == DataType::Vec2) return true;
        
        return false;
    }
};

//=============================================================================
// Specific Node Implementations
//=============================================================================

// Input Nodes
class AttributeInputNode : public VisualShaderNode {
public:
    enum class AttributeType {
        Position = 0,
        Normal,
        Tangent,
        UV,
        UV2,
        Color,
        Custom
    };
    
    AttributeType attribute_type{AttributeType::Position};
    
    AttributeInputNode(u32 id) : VisualShaderNode(id, "Attribute Input", Category::Input) {
        setup_pins();
        help_text = "Provides vertex attributes from the mesh data. These are per-vertex values.";
    }
    
    std::string generate_code(resources::ShaderStage stage) const override {
        switch (attribute_type) {
            case AttributeType::Position: return "vec3 position = a_position;";
            case AttributeType::Normal: return "vec3 normal = a_normal;";
            case AttributeType::UV: return "vec2 uv = a_texcoord0;";
            case AttributeType::Color: return "vec4 color = a_color;";
            default: return "// Custom attribute";
        }
    }
    
    std::unique_ptr<VisualShaderNode> clone() const override {
        auto node = std::make_unique<AttributeInputNode>(0);
        node->attribute_type = attribute_type;
        return std::move(node);
    }
    
private:
    void setup_pins() {
        switch (attribute_type) {
            case AttributeType::Position:
                add_output_pin("Position", DataType::Vec3);
                break;
            case AttributeType::Normal:
                add_output_pin("Normal", DataType::Normal);
                break;
            case AttributeType::UV:
                add_output_pin("UV", DataType::UV);
                break;
            case AttributeType::Color:
                add_output_pin("Color", DataType::Color);
                break;
            default:
                add_output_pin("Value", DataType::Vec4);
                break;
        }
    }
};

class UniformInputNode : public VisualShaderNode {
public:
    std::string uniform_name{"u_custom"};
    DataType uniform_type{DataType::Float};
    
    UniformInputNode(u32 id) : VisualShaderNode(id, "Uniform Input", Category::Input) {
        add_output_pin("Value", uniform_type);
        help_text = "Provides uniform values that are constant across all vertices/fragments in a draw call.";
    }
    
    std::string generate_code(resources::ShaderStage stage) const override {
        return "// Uniform: " + uniform_name + " of type " + data_type_to_string(uniform_type);
    }
    
    std::unique_ptr<VisualShaderNode> clone() const override {
        auto node = std::make_unique<UniformInputNode>(0);
        node->uniform_name = uniform_name;
        node->uniform_type = uniform_type;
        return std::move(node);
    }
    
private:
    std::string data_type_to_string(DataType type) const {
        switch (type) {
            case DataType::Float: return "float";
            case DataType::Vec2: return "vec2";
            case DataType::Vec3: return "vec3";
            case DataType::Vec4: return "vec4";
            case DataType::Mat4: return "mat4";
            default: return "unknown";
        }
    }
};

// Math Nodes
class MathOperationNode : public VisualShaderNode {
public:
    enum class Operation {
        Add = 0, Subtract, Multiply, Divide,
        Power, SquareRoot, Sine, Cosine, Tangent,
        Min, Max, Clamp, Mix, Step, Smoothstep,
        Dot, Cross, Normalize, Length, Distance,
        Reflect, Refract, Abs, Sign, Floor, Ceil
    };
    
    Operation operation{Operation::Add};
    
    MathOperationNode(u32 id) : VisualShaderNode(id, "Math", Category::Math) {
        setup_for_operation();
        help_text = "Performs mathematical operations on input values.";
    }
    
    std::string generate_code(resources::ShaderStage stage) const override {
        switch (operation) {
            case Operation::Add: return "result = a + b;";
            case Operation::Multiply: return "result = a * b;";
            case Operation::Normalize: return "result = normalize(input);";
            case Operation::Dot: return "result = dot(a, b);";
            case Operation::Mix: return "result = mix(a, b, factor);";
            default: return "// Math operation";
        }
    }
    
    std::unique_ptr<VisualShaderNode> clone() const override {
        auto node = std::make_unique<MathOperationNode>(0);
        node->operation = operation;
        return std::move(node);
    }
    
    void on_property_changed(const std::string& property_name) override {
        if (property_name == "operation") {
            setup_for_operation();
        }
    }
    
private:
    void setup_for_operation() {
        input_pins.clear();
        output_pins.clear();
        
        switch (operation) {
            case Operation::Add:
            case Operation::Subtract:
            case Operation::Multiply:
            case Operation::Divide:
                add_input_pin("A", DataType::Float, f32{0.0f});
                add_input_pin("B", DataType::Float, f32{0.0f});
                add_output_pin("Result", DataType::Float);
                break;
            case Operation::Normalize:
                add_input_pin("Vector", DataType::Vec3);
                add_output_pin("Result", DataType::Vec3);
                break;
            case Operation::Dot:
                add_input_pin("A", DataType::Vec3);
                add_input_pin("B", DataType::Vec3);
                add_output_pin("Result", DataType::Float);
                break;
            case Operation::Mix:
                add_input_pin("A", DataType::Vec3);
                add_input_pin("B", DataType::Vec3);
                add_input_pin("Factor", DataType::Float, f32{0.5f});
                add_output_pin("Result", DataType::Vec3);
                break;
            default:
                add_input_pin("Input", DataType::Float);
                add_output_pin("Result", DataType::Float);
                break;
        }
    }
};

// Texture Nodes
class TextureSampleNode : public VisualShaderNode {
public:
    std::string texture_path;
    bool use_mipmaps{true};
    
    TextureSampleNode(u32 id) : VisualShaderNode(id, "Texture Sample", Category::Texture) {
        add_input_pin("Texture", DataType::Sampler2D);
        add_input_pin("UV", DataType::UV);
        add_output_pin("Color", DataType::Color);
        add_output_pin("Alpha", DataType::Float);
        
        help_text = "Samples a texture at the given UV coordinates to get color data.";
        code_explanation = "Uses the texture2D() function to sample the texture.";
    }
    
    std::string generate_code(resources::ShaderStage stage) const override {
        return R"(
vec4 texColor = texture2D(inputTexture, inputUV);
vec3 color = texColor.rgb;
float alpha = texColor.a;
)";
    }
    
    std::unique_ptr<VisualShaderNode> clone() const override {
        auto node = std::make_unique<TextureSampleNode>(0);
        node->texture_path = texture_path;
        node->use_mipmaps = use_mipmaps;
        return std::move(node);
    }
};

// Output Nodes
class FragmentOutputNode : public VisualShaderNode {
public:
    FragmentOutputNode(u32 id) : VisualShaderNode(id, "Fragment Output", Category::Output) {
        add_input_pin("Color", DataType::Color, std::array<f32, 4>{1.0f, 1.0f, 1.0f, 1.0f});
        add_input_pin("Alpha", DataType::Float, f32{1.0f});
        
        help_text = "Final output node for fragment shaders. Determines the final pixel color.";
        code_explanation = "Sets gl_FragColor or the output color variable.";
    }
    
    std::string generate_code(resources::ShaderStage stage) const override {
        return "gl_FragColor = vec4(inputColor.rgb, inputAlpha);";
    }
    
    std::unique_ptr<VisualShaderNode> clone() const override {
        return std::make_unique<FragmentOutputNode>(0);
    }
};

//=============================================================================
// Visual Shader Graph
//=============================================================================

class VisualShaderGraph {
public:
    std::string name{"Untitled Shader"};
    std::string description;
    resources::ShaderStage target_stage{resources::ShaderStage::Fragment};
    
    // Graph data
    std::unordered_map<u32, std::unique_ptr<VisualShaderNode>> nodes_;
    std::vector<NodeConnection> connections_;
    u32 next_node_id_{1};
    u32 next_connection_id_{1};
    
    // Visual properties
    f32 zoom_level{1.0f};
    f32 pan_x{0.0f}, pan_y{0.0f};
    std::array<f32, 4> background_color{0.1f, 0.1f, 0.15f, 1.0f};
    
    // Selection and editing
    std::vector<u32> selected_nodes_;
    std::optional<u32> active_connection_start_;
    
    VisualShaderGraph() = default;
    ~VisualShaderGraph() = default;
    
    // Node management
    u32 add_node(std::unique_ptr<VisualShaderNode> node) {
        u32 id = next_node_id_++;
        node->id = id;
        nodes_[id] = std::move(node);
        return id;
    }
    
    void remove_node(u32 node_id) {
        // Remove all connections to this node
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                          [node_id](const NodeConnection& conn) {
                              return conn.from_node_id == node_id || conn.to_node_id == node_id;
                          }),
            connections_.end());
        
        nodes_.erase(node_id);
    }
    
    VisualShaderNode* get_node(u32 node_id) {
        auto it = nodes_.find(node_id);
        return (it != nodes_.end()) ? it->second.get() : nullptr;
    }
    
    // Connection management
    bool add_connection(u32 from_node, const std::string& from_pin,
                       u32 to_node, const std::string& to_pin) {
        auto* from_node_ptr = get_node(from_node);
        auto* to_node_ptr = get_node(to_node);
        
        if (!from_node_ptr || !to_node_ptr) return false;
        
        if (!from_node_ptr->can_connect_to(*to_node_ptr, from_pin, to_pin)) {
            return false;
        }
        
        // Remove existing connection to the input pin
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                          [to_node, &to_pin](const NodeConnection& conn) {
                              return conn.to_node_id == to_node && conn.to_pin == to_pin;
                          }),
            connections_.end());
        
        // Add new connection
        auto* from_pin_ptr = from_node_ptr->find_pin(from_pin, NodePin::Direction::Output);
        connections_.emplace_back(from_node, from_pin, to_node, to_pin, from_pin_ptr->type);
        
        return true;
    }
    
    void remove_connection(u32 connection_id) {
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                          [connection_id](const NodeConnection& conn) {
                              return conn.id == connection_id;
                          }),
            connections_.end());
    }
    
    // Graph validation and compilation
    bool validate_graph() const {
        // Check for cycles
        if (has_cycles()) return false;
        
        // Validate all nodes
        for (const auto& [id, node] : nodes_) {
            if (!node->validate()) return false;
        }
        
        // Validate connections
        for (const auto& conn : connections_) {
            if (!conn.is_valid) return false;
        }
        
        // Check for required output nodes
        bool has_output = std::any_of(nodes_.begin(), nodes_.end(),
                                    [](const auto& pair) {
                                        return pair.second->category == VisualShaderNode::Category::Output;
                                    });
        
        return has_output;
    }
    
    std::string compile_to_glsl() const {
        if (!validate_graph()) {
            return "// ERROR: Graph validation failed";
        }
        
        std::ostringstream code;
        
        // Generate shader header
        if (target_stage == resources::ShaderStage::Vertex) {
            code << "#version 330 core\n\n";
            code << "// Vertex shader generated from visual graph\n\n";
        } else if (target_stage == resources::ShaderStage::Fragment) {
            code << "#version 330 core\n\n";
            code << "// Fragment shader generated from visual graph\n\n";
        }
        
        // Generate uniform declarations
        generate_uniform_declarations(code);
        
        // Generate attribute declarations for vertex shader
        if (target_stage == resources::ShaderStage::Vertex) {
            generate_attribute_declarations(code);
        }
        
        // Generate main function
        code << "void main() {\n";
        
        // Generate node code in execution order
        auto execution_order = get_execution_order();
        for (u32 node_id : execution_order) {
            auto* node = const_cast<VisualShaderGraph*>(this)->get_node(node_id);
            if (node) {
                code << "    // " << node->name << " (ID: " << node_id << ")\n";
                std::string node_code = node->generate_code(target_stage);
                code << "    " << node_code << "\n\n";
            }
        }
        
        code << "}\n";
        
        return code.str();
    }
    
    // Educational features
    std::string generate_explanation() const {
        std::ostringstream explanation;
        
        explanation << "Shader Graph Explanation:\n";
        explanation << "=======================\n\n";
        
        explanation << "This " << (target_stage == resources::ShaderStage::Fragment ? "fragment" : "vertex")
                   << " shader performs the following operations:\n\n";
        
        auto execution_order = get_execution_order();
        for (u32 node_id : execution_order) {
            auto* node = const_cast<VisualShaderGraph*>(this)->get_node(node_id);
            if (node && !node->help_text.empty()) {
                explanation << "• " << node->name << ": " << node->help_text << "\n";
            }
        }
        
        explanation << "\nData Flow:\n";
        for (const auto& conn : connections_) {
            auto* from_node = const_cast<VisualShaderGraph*>(this)->get_node(conn.from_node_id);
            auto* to_node = const_cast<VisualShaderGraph*>(this)->get_node(conn.to_node_id);
            if (from_node && to_node) {
                explanation << "  " << from_node->name << "." << conn.from_pin 
                           << " → " << to_node->name << "." << conn.to_pin << "\n";
            }
        }
        
        return explanation.str();
    }
    
    std::vector<std::string> get_optimization_suggestions() const {
        std::vector<std::string> suggestions;
        
        // Count texture samples
        u32 texture_samples = 0;
        for (const auto& [id, node] : nodes_) {
            if (node->category == VisualShaderNode::Category::Texture) {
                texture_samples++;
            }
        }
        
        if (texture_samples > 4) {
            suggestions.push_back("Consider reducing the number of texture samples for better performance");
        }
        
        // Check for redundant calculations
        std::unordered_map<std::string, u32> operation_counts;
        for (const auto& [id, node] : nodes_) {
            operation_counts[node->name]++;
        }
        
        for (const auto& [op, count] : operation_counts) {
            if (count > 3) {
                suggestions.push_back("Multiple " + op + " nodes detected - consider consolidating");
            }
        }
        
        return suggestions;
    }
    
    // Serialization
    std::string serialize_to_json() const;
    bool deserialize_from_json(const std::string& json_data);
    
private:
    bool has_cycles() const {
        std::unordered_set<u32> visited, in_stack;
        
        std::function<bool(u32)> dfs = [&](u32 node_id) -> bool {
            if (in_stack.find(node_id) != in_stack.end()) return true;
            if (visited.find(node_id) != visited.end()) return false;
            
            visited.insert(node_id);
            in_stack.insert(node_id);
            
            for (const auto& conn : connections_) {
                if (conn.from_node_id == node_id) {
                    if (dfs(conn.to_node_id)) return true;
                }
            }
            
            in_stack.erase(node_id);
            return false;
        };
        
        for (const auto& [id, node] : nodes_) {
            if (visited.find(id) == visited.end()) {
                if (dfs(id)) return true;
            }
        }
        
        return false;
    }
    
    std::vector<u32> get_execution_order() const {
        std::vector<u32> order;
        std::unordered_set<u32> visited;
        
        std::function<void(u32)> dfs = [&](u32 node_id) {
            if (visited.find(node_id) != visited.end()) return;
            visited.insert(node_id);
            
            // Process dependencies first
            for (const auto& conn : connections_) {
                if (conn.to_node_id == node_id) {
                    dfs(conn.from_node_id);
                }
            }
            
            order.push_back(node_id);
        };
        
        // Start with output nodes
        for (const auto& [id, node] : nodes_) {
            if (node->category == VisualShaderNode::Category::Output) {
                dfs(id);
            }
        }
        
        return order;
    }
    
    void generate_uniform_declarations(std::ostringstream& code) const {
        for (const auto& [id, node] : nodes_) {
            if (node->category == VisualShaderNode::Category::Input) {
                // Generate uniform declarations for uniform input nodes
                auto* uniform_node = dynamic_cast<const UniformInputNode*>(node.get());
                if (uniform_node) {
                    code << "uniform " << data_type_to_glsl_string(uniform_node->uniform_type)
                         << " " << uniform_node->uniform_name << ";\n";
                }
            }
        }
        code << "\n";
    }
    
    void generate_attribute_declarations(std::ostringstream& code) const {
        for (const auto& [id, node] : nodes_) {
            if (node->category == VisualShaderNode::Category::Input) {
                auto* attr_node = dynamic_cast<const AttributeInputNode*>(node.get());
                if (attr_node) {
                    switch (attr_node->attribute_type) {
                        case AttributeInputNode::AttributeType::Position:
                            code << "layout(location = 0) in vec3 a_position;\n";
                            break;
                        case AttributeInputNode::AttributeType::Normal:
                            code << "layout(location = 1) in vec3 a_normal;\n";
                            break;
                        case AttributeInputNode::AttributeType::UV:
                            code << "layout(location = 2) in vec2 a_texcoord0;\n";
                            break;
                        case AttributeInputNode::AttributeType::Color:
                            code << "layout(location = 3) in vec4 a_color;\n";
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        code << "\n";
    }
    
    std::string data_type_to_glsl_string(DataType type) const {
        switch (type) {
            case DataType::Float: return "float";
            case DataType::Vec2: return "vec2";
            case DataType::Vec3: return "vec3";
            case DataType::Vec4: return "vec4";
            case DataType::Int: return "int";
            case DataType::IVec2: return "ivec2";
            case DataType::IVec3: return "ivec3";
            case DataType::IVec4: return "ivec4";
            case DataType::Bool: return "bool";
            case DataType::Mat2: return "mat2";
            case DataType::Mat3: return "mat3";
            case DataType::Mat4: return "mat4";
            case DataType::Sampler2D: return "sampler2D";
            case DataType::SamplerCube: return "samplerCube";
            default: return "float";
        }
    }
};

//=============================================================================
// Visual Shader Editor Interface
//=============================================================================

class VisualShaderEditor {
public:
    struct EditorConfig {
        bool show_grid{true};
        bool snap_to_grid{false};
        f32 grid_size{20.0f};
        bool show_minimap{true};
        bool enable_auto_layout{false};
        bool show_performance_overlay{true};
        bool enable_real_time_compilation{true};
        std::string theme{"Dark"};
    };
    
    explicit VisualShaderEditor(shader_compiler::AdvancedShaderCompiler* compiler);
    ~VisualShaderEditor() = default;
    
    // Graph management
    void new_graph(resources::ShaderStage stage = resources::ShaderStage::Fragment);
    bool load_graph(const std::string& file_path);
    bool save_graph(const std::string& file_path);
    
    // Node operations
    u32 add_node(std::unique_ptr<VisualShaderNode> node, f32 x = 0.0f, f32 y = 0.0f);
    void remove_selected_nodes();
    void duplicate_selected_nodes();
    
    // Connection operations
    bool create_connection(u32 from_node, const std::string& from_pin,
                          u32 to_node, const std::string& to_pin);
    void remove_connection(u32 connection_id);
    
    // Compilation and preview
    shader_compiler::CompilationResult compile_current_graph();
    std::string get_generated_code() const;
    bool enable_live_preview(bool enabled);
    
    // UI and interaction
    void render_editor(); // For integration with ImGui or custom UI
    void handle_mouse_input(f32 x, f32 y, bool left_button, bool right_button);
    void handle_keyboard_input(i32 key, bool pressed);
    
    // Educational features
    void show_tutorial_overlay(bool show);
    void highlight_data_flow(bool highlight);
    std::string get_current_explanation() const;
    std::vector<std::string> get_performance_tips() const;
    
    // Configuration
    void set_config(const EditorConfig& config) { config_ = config; }
    const EditorConfig& get_config() const { return config_; }
    
    VisualShaderGraph* get_current_graph() { return current_graph_.get(); }
    
private:
    shader_compiler::AdvancedShaderCompiler* compiler_;
    std::unique_ptr<VisualShaderGraph> current_graph_;
    EditorConfig config_;
    
    // UI state
    bool is_dragging_node_{false};
    bool is_creating_connection_{false};
    u32 dragged_node_id_{0};
    std::array<f32, 2> mouse_position_{0.0f, 0.0f};
    std::array<f32, 2> drag_offset_{0.0f, 0.0f};
    
    // Educational overlays
    bool show_tutorial_{false};
    bool highlight_flow_{false};
    
    // Real-time compilation
    std::chrono::steady_clock::time_point last_compile_time_;
    shader_compiler::CompilationResult last_compilation_;
    
    void update_real_time_compilation();
    void render_node(const VisualShaderNode& node);
    void render_connection(const NodeConnection& connection);
    void render_grid();
    void render_minimap();
    void render_tutorial_overlay();
    void render_performance_overlay();
    
    std::unique_ptr<VisualShaderNode> create_node_by_type(const std::string& node_type);
};

//=============================================================================
// Node Factory for Creating Standard Nodes
//=============================================================================

class NodeFactory {
public:
    static std::unique_ptr<VisualShaderNode> create_attribute_input(u32 id, AttributeInputNode::AttributeType type);
    static std::unique_ptr<VisualShaderNode> create_uniform_input(u32 id, const std::string& name, DataType type);
    static std::unique_ptr<VisualShaderNode> create_math_operation(u32 id, MathOperationNode::Operation op);
    static std::unique_ptr<VisualShaderNode> create_texture_sample(u32 id);
    static std::unique_ptr<VisualShaderNode> create_fragment_output(u32 id);
    
    // Get list of available node types
    static std::vector<std::string> get_available_node_types();
    static std::string get_node_category(const std::string& node_type);
    static std::string get_node_description(const std::string& node_type);
};

} // namespace ecscope::renderer::visual_editor