/**
 * @file gui_text.hpp
 * @brief Text Rendering System for GUI Framework
 * 
 * Advanced text rendering with font loading, glyph caching, text layout,
 * multi-language support, and high-performance text rendering.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#pragma once

#include "gui_core.hpp"
#include "gui_memory.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>
#include <span>

namespace ecscope::gui {

// =============================================================================
// FONT AND GLYPH DEFINITIONS
// =============================================================================

/**
 * @brief Unicode codepoint type
 */
using Codepoint = uint32_t;

/**
 * @brief Font weight enumeration
 */
enum class FontWeight : uint16_t {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

/**
 * @brief Font style flags
 */
enum class FontStyle : uint8_t {
    Normal = 0,
    Italic = 1 << 0,
    Oblique = 1 << 1
};

inline FontStyle operator|(FontStyle a, FontStyle b) {
    return static_cast<FontStyle>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

/**
 * @brief Text alignment options
 */
enum class TextAlign : uint8_t {
    Left,
    Center,
    Right,
    Justify
};

/**
 * @brief Text baseline alignment
 */
enum class TextBaseline : uint8_t {
    Top,
    Middle,
    Bottom,
    Alphabetic,
    Hanging
};

/**
 * @brief Text wrapping modes
 */
enum class TextWrap : uint8_t {
    None,      // No wrapping
    Word,      // Wrap at word boundaries
    Character, // Wrap at any character
    Ellipsis   // Truncate with ellipsis
};

/**
 * @brief Individual glyph information
 */
struct Glyph {
    Codepoint codepoint = 0;
    
    // Metrics (in font units, typically 1/64th of a pixel)
    float advance_x = 0.0f;        // Horizontal advance
    float advance_y = 0.0f;        // Vertical advance
    float bearing_x = 0.0f;        // Left side bearing
    float bearing_y = 0.0f;        // Top side bearing
    float width = 0.0f;            // Glyph width
    float height = 0.0f;           // Glyph height
    
    // Atlas coordinates (normalized UV coordinates)
    float u0 = 0.0f, v0 = 0.0f;   // Top-left UV
    float u1 = 0.0f, v1 = 0.0f;   // Bottom-right UV
    
    // Atlas pixel coordinates
    uint16_t x = 0, y = 0;         // Atlas position
    uint16_t w = 0, h = 0;         // Glyph dimensions
    
    bool is_valid() const { return codepoint != 0; }
};

/**
 * @brief Font metrics and properties
 */
struct FontMetrics {
    float ascender = 0.0f;         // Distance from baseline to top
    float descender = 0.0f;        // Distance from baseline to bottom (negative)
    float line_height = 0.0f;      // Recommended line spacing
    float underline_position = 0.0f;
    float underline_thickness = 0.0f;
    float x_height = 0.0f;         // Height of lowercase 'x'
    float cap_height = 0.0f;       // Height of uppercase letters
    float max_advance = 0.0f;      // Maximum character width
    
    float get_total_height() const { return ascender - descender; }
};

// =============================================================================
// FONT ATLAS SYSTEM
// =============================================================================

/**
 * @brief Font atlas configuration
 */
struct FontAtlasConfig {
    uint32_t width = 1024;
    uint32_t height = 1024;
    uint32_t padding = 1;          // Padding between glyphs
    bool sdf_enabled = false;      // Signed distance field rendering
    float sdf_radius = 8.0f;       // SDF radius for crisp scaling
    uint32_t oversample_h = 3;     // Horizontal oversampling
    uint32_t oversample_v = 1;     // Vertical oversampling
    bool power_of_two = true;      // Force power-of-two dimensions
};

/**
 * @brief Font atlas managing glyph textures
 */
class FontAtlas {
public:
    FontAtlas();
    ~FontAtlas();
    
    bool initialize(const FontAtlasConfig& config = {});
    void shutdown();
    
    // =============================================================================
    // FONT MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Font handle for identifying loaded fonts
     */
    using FontHandle = uint32_t;
    static constexpr FontHandle INVALID_FONT = 0;
    
    /**
     * @brief Load font from file
     */
    FontHandle load_font_from_file(const std::string& filename, float size,
                                   FontWeight weight = FontWeight::Normal,
                                   FontStyle style = FontStyle::Normal);
    
    /**
     * @brief Load font from memory buffer
     */
    FontHandle load_font_from_memory(std::span<const uint8_t> data, float size,
                                     FontWeight weight = FontWeight::Normal,
                                     FontStyle style = FontStyle::Normal);
    
    /**
     * @brief Get default font (embedded)
     */
    FontHandle get_default_font();
    
    /**
     * @brief Get font metrics
     */
    const FontMetrics& get_font_metrics(FontHandle font) const;
    
    /**
     * @brief Scale font to new size
     */
    FontHandle scale_font(FontHandle font, float new_size);
    
    // =============================================================================
    // GLYPH MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Get or cache a glyph
     */
    const Glyph* get_glyph(FontHandle font, Codepoint codepoint);
    
    /**
     * @brief Pre-cache a range of glyphs
     */
    void cache_glyph_range(FontHandle font, Codepoint start, Codepoint end);
    
    /**
     * @brief Cache commonly used ASCII characters
     */
    void cache_ascii_glyphs(FontHandle font);
    
    /**
     * @brief Cache glyphs for a specific string
     */
    void cache_string_glyphs(FontHandle font, const std::string& text);
    
    /**
     * @brief Get kerning between two glyphs
     */
    float get_kerning(FontHandle font, Codepoint left, Codepoint right);
    
    // =============================================================================
    // ATLAS MANAGEMENT
    // =============================================================================
    
    /**
     * @brief Get atlas texture ID for rendering
     */
    uint32_t get_atlas_texture_id() const { return atlas_texture_id_; }
    
    /**
     * @brief Rebuild atlas (useful after loading many fonts)
     */
    void rebuild_atlas();
    
    /**
     * @brief Clear unused glyphs to free memory
     */
    void garbage_collect();
    
    /**
     * @brief Get atlas statistics
     */
    struct AtlasStats {
        uint32_t atlas_width = 0;
        uint32_t atlas_height = 0;
        uint32_t used_pixels = 0;
        uint32_t total_pixels = 0;
        uint32_t num_glyphs = 0;
        uint32_t num_fonts = 0;
        float utilization_ratio = 0.0f;
        size_t memory_usage = 0;
    };
    
    AtlasStats get_stats() const;
    
    // =============================================================================
    // DEBUG AND UTILITIES
    // =============================================================================
    
    void save_atlas_debug_image(const std::string& filename) const;
    void print_debug_info() const;
    
private:
    struct FontData {
        void* ft_face = nullptr;           // FreeType face
        float size = 12.0f;
        FontWeight weight = FontWeight::Normal;
        FontStyle style = FontStyle::Normal;
        FontMetrics metrics;
        std::unordered_map<Codepoint, Glyph> glyphs;
        std::unordered_map<uint64_t, float> kerning_cache; // Packed (left,right) -> kerning
        std::vector<uint8_t> font_data;    // Font file data (for memory fonts)
        std::string filename;
        bool is_default = false;
    };
    
    struct AtlasNode {
        uint16_t x = 0, y = 0;
        uint16_t w = 0, h = 0;
        bool used = false;
        std::unique_ptr<AtlasNode> left;
        std::unique_ptr<AtlasNode> right;
        
        AtlasNode* insert(uint16_t width, uint16_t height);
    };
    
    // Core state
    FontAtlasConfig config_;
    bool initialized_ = false;
    
    // Font management
    std::unordered_map<FontHandle, std::unique_ptr<FontData>> fonts_;
    FontHandle next_font_handle_ = 1;
    FontHandle default_font_ = INVALID_FONT;
    
    // Atlas management
    std::vector<uint8_t> atlas_pixels_;
    std::unique_ptr<AtlasNode> atlas_root_;
    uint32_t atlas_texture_id_ = 0;
    bool atlas_dirty_ = false;
    
    // FreeType library
    void* ft_library_ = nullptr;
    
    // Helper methods
    bool init_freetype();
    void shutdown_freetype();
    FontHandle create_font_handle() { return next_font_handle_++; }
    bool load_default_font();
    bool rasterize_glyph(FontData& font, Codepoint codepoint, Glyph& glyph);
    bool pack_glyph_to_atlas(const uint8_t* bitmap, uint16_t width, uint16_t height, 
                            uint16_t& out_x, uint16_t& out_y);
    void update_atlas_texture();
    uint64_t pack_kerning_key(Codepoint left, Codepoint right) const;
    
    // Unicode helpers
    static bool is_whitespace(Codepoint cp);
    static bool is_line_break(Codepoint cp);
    static bool is_word_break(Codepoint cp);
};

// =============================================================================
// TEXT LAYOUT SYSTEM
// =============================================================================

/**
 * @brief Text layout configuration
 */
struct TextLayoutConfig {
    FontAtlas::FontHandle font = FontAtlas::INVALID_FONT;
    float font_size = 12.0f;
    Color color = Color::WHITE;
    TextAlign align = TextAlign::Left;
    TextBaseline baseline = TextBaseline::Top;
    TextWrap wrap = TextWrap::None;
    float max_width = 0.0f;         // 0 = unlimited
    float max_height = 0.0f;        // 0 = unlimited
    float line_spacing = 1.0f;      // Multiplier for line height
    float letter_spacing = 0.0f;    // Additional space between characters
    float word_spacing = 0.0f;      // Additional space between words
    bool kerning_enabled = true;
    bool subpixel_positioning = true;
};

/**
 * @brief Positioned glyph for rendering
 */
struct PositionedGlyph {
    const Glyph* glyph = nullptr;
    Vec2 position = {0, 0};
    Color color = Color::WHITE;
    float scale = 1.0f;
};

/**
 * @brief Text layout result
 */
struct TextLayout {
    std::vector<PositionedGlyph> glyphs;
    std::vector<uint32_t> line_breaks;     // Indices into glyphs array
    Vec2 size = {0, 0};                    // Total layout size
    float baseline_y = 0.0f;               // Y position of first baseline
    
    void clear() {
        glyphs.clear();
        line_breaks.clear();
        size = {0, 0};
        baseline_y = 0.0f;
    }
    
    size_t get_line_count() const { return line_breaks.empty() ? 0 : line_breaks.size(); }
    
    // Get glyphs for a specific line
    std::span<const PositionedGlyph> get_line_glyphs(size_t line_index) const;
};

/**
 * @brief Text layout engine
 */
class TextLayoutEngine {
public:
    TextLayoutEngine();
    ~TextLayoutEngine();
    
    bool initialize(FontAtlas* font_atlas);
    void shutdown();
    
    // =============================================================================
    // LAYOUT OPERATIONS
    // =============================================================================
    
    /**
     * @brief Layout text with specified configuration
     */
    void layout_text(const std::string& text, const TextLayoutConfig& config, TextLayout& output);
    
    /**
     * @brief Layout text within a bounding rectangle
     */
    void layout_text_in_rect(const std::string& text, const Rect& bounds, 
                            const TextLayoutConfig& config, TextLayout& output);
    
    /**
     * @brief Calculate text size without full layout
     */
    Vec2 measure_text(const std::string& text, const TextLayoutConfig& config);
    
    /**
     * @brief Find character position from pixel coordinates
     */
    size_t find_character_at_position(const TextLayout& layout, const Vec2& position);
    
    /**
     * @brief Get pixel position of character
     */
    Vec2 get_character_position(const TextLayout& layout, size_t character_index);
    
    /**
     * @brief Get line information
     */
    struct LineInfo {
        size_t start_char = 0;
        size_t end_char = 0;
        float width = 0.0f;
        float height = 0.0f;
        Vec2 position = {0, 0};
    };
    
    std::vector<LineInfo> get_line_info(const TextLayout& layout);
    
    // =============================================================================
    // FONT UTILITIES
    // =============================================================================
    
    /**
     * @brief Get font metrics for a specific font/size
     */
    FontMetrics get_scaled_font_metrics(FontAtlas::FontHandle font, float size);
    
    /**
     * @brief Calculate optimal line height
     */
    float calculate_line_height(FontAtlas::FontHandle font, float size, float line_spacing = 1.0f);
    
private:
    FontAtlas* font_atlas_ = nullptr;
    bool initialized_ = false;
    
    // Layout state
    struct LayoutState {
        const std::string* text = nullptr;
        const TextLayoutConfig* config = nullptr;
        TextLayout* output = nullptr;
        
        Vec2 cursor = {0, 0};
        float current_line_width = 0.0f;
        float current_line_height = 0.0f;
        size_t current_line_start = 0;
        size_t current_char_index = 0;
        
        // Break opportunities
        size_t last_word_break_glyph = 0;
        size_t last_word_break_char = 0;
        float last_word_break_width = 0.0f;
        Vec2 last_word_break_cursor = {0, 0};
        
        void reset() {
            cursor = {0, 0};
            current_line_width = 0.0f;
            current_line_height = 0.0f;
            current_line_start = 0;
            current_char_index = 0;
            last_word_break_glyph = 0;
            last_word_break_char = 0;
            last_word_break_width = 0.0f;
            last_word_break_cursor = {0, 0};
        }
    };
    
    // Layout helpers
    void process_character(LayoutState& state, Codepoint codepoint);
    void break_line(LayoutState& state, bool force = false);
    void align_line(LayoutState& state, size_t line_start, size_t line_end);
    void finalize_layout(LayoutState& state);
    
    // Unicode processing
    std::vector<Codepoint> utf8_to_codepoints(const std::string& utf8_text);
    bool is_breaking_character(Codepoint cp);
    bool should_break_before(Codepoint cp);
    bool should_break_after(Codepoint cp);
};

// =============================================================================
// TEXT RENDERER
// =============================================================================

/**
 * @brief High-performance text renderer
 */
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    
    bool initialize(FontAtlas* font_atlas, rendering::IRenderer* renderer);
    void shutdown();
    
    // =============================================================================
    // IMMEDIATE RENDERING
    // =============================================================================
    
    /**
     * @brief Render text immediately at position
     */
    void render_text(const std::string& text, const Vec2& position, 
                    const TextLayoutConfig& config);
    
    /**
     * @brief Render pre-laid-out text
     */
    void render_layout(const TextLayout& layout, const Vec2& offset = {0, 0});
    
    /**
     * @brief Render text with highlighting
     */
    void render_text_with_selection(const std::string& text, const Vec2& position,
                                   const TextLayoutConfig& config,
                                   size_t selection_start, size_t selection_end,
                                   const Color& selection_color);
    
    // =============================================================================
    // BATCH RENDERING
    // =============================================================================
    
    /**
     * @brief Begin batch rendering for multiple text draws
     */
    void begin_batch();
    
    /**
     * @brief Add text to current batch
     */
    void add_to_batch(const TextLayout& layout, const Vec2& offset = {0, 0});
    
    /**
     * @brief Render all batched text
     */
    void end_batch();
    
    // =============================================================================
    // EFFECTS AND STYLING
    // =============================================================================
    
    /**
     * @brief Render text with shadow effect
     */
    void render_text_with_shadow(const std::string& text, const Vec2& position,
                                const TextLayoutConfig& config,
                                const Vec2& shadow_offset, const Color& shadow_color);
    
    /**
     * @brief Render text with outline
     */
    void render_text_with_outline(const std::string& text, const Vec2& position,
                                 const TextLayoutConfig& config,
                                 float outline_thickness, const Color& outline_color);
    
    // =============================================================================
    // PERFORMANCE MONITORING
    // =============================================================================
    
    struct RenderStats {
        uint32_t glyphs_rendered = 0;
        uint32_t draw_calls = 0;
        uint32_t vertices_generated = 0;
        float cpu_time_ms = 0.0f;
        float gpu_time_ms = 0.0f;
        uint32_t atlas_cache_hits = 0;
        uint32_t atlas_cache_misses = 0;
    };
    
    RenderStats get_render_stats() const { return render_stats_; }
    void reset_render_stats() { render_stats_ = {}; }
    
private:
    FontAtlas* font_atlas_ = nullptr;
    rendering::IRenderer* renderer_ = nullptr;
    TextLayoutEngine layout_engine_;
    bool initialized_ = false;
    
    // Rendering resources
    rendering::ShaderHandle text_shader_;
    rendering::BufferHandle vertex_buffer_;
    rendering::BufferHandle index_buffer_;
    
    // Batch state
    bool batching_ = false;
    std::vector<PositionedGlyph> batch_glyphs_;
    
    // Statistics
    RenderStats render_stats_;
    
    // Vertex structure for text rendering
    struct TextVertex {
        Vec2 position;
        Vec2 uv;
        Color color;
    };
    
    // Rendering helpers
    void upload_glyph_data(std::span<const PositionedGlyph> glyphs);
    void create_text_shader();
    void setup_rendering_state();
};

// =============================================================================
// CONVENIENCE FUNCTIONS
// =============================================================================

/**
 * @brief Calculate text size (convenience function)
 */
Vec2 calc_text_size(const std::string& text, FontAtlas::FontHandle font = FontAtlas::INVALID_FONT, float size = 12.0f);

/**
 * @brief Render text immediately (convenience function)
 */
void render_text(const std::string& text, const Vec2& position, const Color& color = Color::WHITE,
                FontAtlas::FontHandle font = FontAtlas::INVALID_FONT, float size = 12.0f);

/**
 * @brief Word wrap text to fit in width
 */
std::vector<std::string> word_wrap_text(const std::string& text, float max_width,
                                       FontAtlas::FontHandle font = FontAtlas::INVALID_FONT, float size = 12.0f);

} // namespace ecscope::gui