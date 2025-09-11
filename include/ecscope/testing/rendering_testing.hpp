#pragma once

#include "test_framework.hpp"
#include "../rendering/renderer.hpp"
#include "../rendering/shader.hpp"
#include "../rendering/texture.hpp"
#include "../rendering/framebuffer.hpp"
#include <vector>
#include <memory>
#include <cmath>

#ifdef ECSCOPE_OPENGL
    #include <GL/glew.h>
    #ifdef _WIN32
        #include <GL/wglew.h>
    #elif defined(__linux__)
        #include <GL/glxew.h>
    #endif
#endif

namespace ecscope::testing {

// Pixel comparison utilities
struct PixelData {
    uint8_t r, g, b, a;
    
    bool operator==(const PixelData& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    
    double distance(const PixelData& other) const {
        double dr = static_cast<double>(r) - other.r;
        double dg = static_cast<double>(g) - other.g;
        double db = static_cast<double>(b) - other.b;
        double da = static_cast<double>(a) - other.a;
        return std::sqrt(dr*dr + dg*dg + db*db + da*da);
    }
};

class ImageComparator {
public:
    struct ComparisonResult {
        bool images_match;
        double mse; // Mean Squared Error
        double psnr; // Peak Signal-to-Noise Ratio
        double ssim; // Structural Similarity Index
        size_t different_pixels;
        double max_pixel_difference;
        std::vector<PixelData> reference_image;
        std::vector<PixelData> test_image;
        size_t width, height;
    };

    static ComparisonResult compare_images(const std::vector<PixelData>& reference,
                                         const std::vector<PixelData>& test,
                                         size_t width, size_t height,
                                         double tolerance = 1.0) {
        ComparisonResult result;
        result.reference_image = reference;
        result.test_image = test;
        result.width = width;
        result.height = height;
        result.different_pixels = 0;
        result.max_pixel_difference = 0.0;

        if (reference.size() != test.size()) {
            result.images_match = false;
            return result;
        }

        double mse_sum = 0.0;
        size_t total_pixels = reference.size();

        for (size_t i = 0; i < total_pixels; ++i) {
            double pixel_diff = reference[i].distance(test[i]);
            
            if (pixel_diff > tolerance) {
                result.different_pixels++;
                result.max_pixel_difference = std::max(result.max_pixel_difference, pixel_diff);
            }

            // Calculate MSE contribution
            double r_diff = static_cast<double>(reference[i].r) - test[i].r;
            double g_diff = static_cast<double>(reference[i].g) - test[i].g;
            double b_diff = static_cast<double>(reference[i].b) - test[i].b;
            mse_sum += r_diff*r_diff + g_diff*g_diff + b_diff*b_diff;
        }

        result.mse = mse_sum / (total_pixels * 3.0); // 3 channels (RGB)
        result.psnr = result.mse > 0 ? 20.0 * std::log10(255.0 / std::sqrt(result.mse)) : 100.0;
        result.ssim = calculate_ssim(reference, test, width, height);
        result.images_match = result.different_pixels == 0;

        return result;
    }

    static std::vector<PixelData> load_reference_image(const std::string& filename) {
        // Implementation would depend on image loading library (e.g., stb_image)
        std::vector<PixelData> pixels;
        // Load image from file...
        return pixels;
    }

    static void save_image(const std::vector<PixelData>& pixels, size_t width, size_t height, 
                          const std::string& filename) {
        // Implementation would depend on image saving library
        // Save image to file...
    }

private:
    static double calculate_ssim(const std::vector<PixelData>& img1, 
                                const std::vector<PixelData>& img2,
                                size_t width, size_t height) {
        // Simplified SSIM calculation
        double mean1 = 0.0, mean2 = 0.0;
        size_t total = img1.size();

        // Calculate means
        for (size_t i = 0; i < total; ++i) {
            mean1 += (img1[i].r + img1[i].g + img1[i].b) / 3.0;
            mean2 += (img2[i].r + img2[i].g + img2[i].b) / 3.0;
        }
        mean1 /= total;
        mean2 /= total;

        // Calculate variances and covariance
        double var1 = 0.0, var2 = 0.0, covar = 0.0;
        for (size_t i = 0; i < total; ++i) {
            double val1 = (img1[i].r + img1[i].g + img1[i].b) / 3.0;
            double val2 = (img2[i].r + img2[i].g + img2[i].b) / 3.0;
            
            var1 += (val1 - mean1) * (val1 - mean1);
            var2 += (val2 - mean2) * (val2 - mean2);
            covar += (val1 - mean1) * (val2 - mean2);
        }
        var1 /= total - 1;
        var2 /= total - 1;
        covar /= total - 1;

        // SSIM constants
        const double c1 = 6.5025; // (0.01 * 255)^2
        const double c2 = 58.5225; // (0.03 * 255)^2

        double numerator = (2 * mean1 * mean2 + c1) * (2 * covar + c2);
        double denominator = (mean1*mean1 + mean2*mean2 + c1) * (var1 + var2 + c2);

        return denominator > 0 ? numerator / denominator : 1.0;
    }
};

// OpenGL state validator
class OpenGLStateValidator {
public:
    struct GLState {
        bool depth_test_enabled;
        bool blend_enabled;
        bool cull_face_enabled;
        int viewport[4];
        float clear_color[4];
        int active_texture_unit;
        std::vector<unsigned int> bound_textures;
        unsigned int bound_framebuffer;
        unsigned int bound_vertex_array;
        unsigned int bound_program;
    };

    static GLState capture_state() {
        GLState state;
        
        #ifdef ECSCOPE_OPENGL
        state.depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
        state.blend_enabled = glIsEnabled(GL_BLEND);
        state.cull_face_enabled = glIsEnabled(GL_CULL_FACE);
        
        glGetIntegerv(GL_VIEWPORT, state.viewport);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, state.clear_color);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &state.active_texture_unit);
        
        int bound_fb;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<int*>(&state.bound_framebuffer));
        
        int bound_vao;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, reinterpret_cast<int*>(&state.bound_vertex_array));
        
        int bound_prog;
        glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<int*>(&state.bound_program));
        
        // Capture bound textures for multiple texture units
        int max_texture_units;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
        state.bound_textures.resize(max_texture_units);
        
        for (int i = 0; i < max_texture_units; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            int bound_texture;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
            state.bound_textures[i] = bound_texture;
        }
        
        // Restore original active texture
        glActiveTexture(state.active_texture_unit);
        #endif
        
        return state;
    }

    static bool compare_states(const GLState& state1, const GLState& state2) {
        if (state1.depth_test_enabled != state2.depth_test_enabled) return false;
        if (state1.blend_enabled != state2.blend_enabled) return false;
        if (state1.cull_face_enabled != state2.cull_face_enabled) return false;
        
        for (int i = 0; i < 4; ++i) {
            if (state1.viewport[i] != state2.viewport[i]) return false;
            if (std::abs(state1.clear_color[i] - state2.clear_color[i]) > 0.001f) return false;
        }
        
        if (state1.bound_framebuffer != state2.bound_framebuffer) return false;
        if (state1.bound_vertex_array != state2.bound_vertex_array) return false;
        if (state1.bound_program != state2.bound_program) return false;
        
        return true;
    }

    static std::string state_diff(const GLState& expected, const GLState& actual) {
        std::stringstream ss;
        
        if (expected.depth_test_enabled != actual.depth_test_enabled) {
            ss << "Depth test: expected " << expected.depth_test_enabled 
               << ", actual " << actual.depth_test_enabled << "\n";
        }
        
        if (expected.bound_program != actual.bound_program) {
            ss << "Bound program: expected " << expected.bound_program 
               << ", actual " << actual.bound_program << "\n";
        }
        
        // Add more state comparisons...
        
        return ss.str();
    }
};

// Shader compilation and validation
class ShaderValidator {
public:
    struct ShaderCompilationResult {
        bool success;
        std::string error_log;
        std::string info_log;
        unsigned int shader_id;
        std::vector<std::string> uniforms;
        std::vector<std::string> attributes;
    };

    static ShaderCompilationResult validate_vertex_shader(const std::string& source) {
        return compile_and_validate_shader(source, ShaderType::VERTEX);
    }

    static ShaderCompilationResult validate_fragment_shader(const std::string& source) {
        return compile_and_validate_shader(source, ShaderType::FRAGMENT);
    }

    static ShaderCompilationResult validate_compute_shader(const std::string& source) {
        return compile_and_validate_shader(source, ShaderType::COMPUTE);
    }

    struct ProgramLinkResult {
        bool success;
        std::string error_log;
        unsigned int program_id;
        std::vector<std::string> active_uniforms;
        std::vector<std::string> active_attributes;
    };

    static ProgramLinkResult validate_program(const std::vector<unsigned int>& shaders) {
        ProgramLinkResult result;
        
        #ifdef ECSCOPE_OPENGL
        result.program_id = glCreateProgram();
        
        for (unsigned int shader : shaders) {
            glAttachShader(result.program_id, shader);
        }
        
        glLinkProgram(result.program_id);
        
        int success;
        glGetProgramiv(result.program_id, GL_LINK_STATUS, &success);
        result.success = success == GL_TRUE;
        
        if (!result.success) {
            char info_log[512];
            glGetProgramInfoLog(result.program_id, 512, nullptr, info_log);
            result.error_log = info_log;
        } else {
            // Query active uniforms and attributes
            result.active_uniforms = query_active_uniforms(result.program_id);
            result.active_attributes = query_active_attributes(result.program_id);
        }
        #endif
        
        return result;
    }

private:
    enum class ShaderType { VERTEX, FRAGMENT, GEOMETRY, COMPUTE };

    static ShaderCompilationResult compile_and_validate_shader(const std::string& source, 
                                                              ShaderType type) {
        ShaderCompilationResult result;
        
        #ifdef ECSCOPE_OPENGL
        unsigned int gl_type;
        switch (type) {
            case ShaderType::VERTEX: gl_type = GL_VERTEX_SHADER; break;
            case ShaderType::FRAGMENT: gl_type = GL_FRAGMENT_SHADER; break;
            case ShaderType::GEOMETRY: gl_type = GL_GEOMETRY_SHADER; break;
            case ShaderType::COMPUTE: gl_type = GL_COMPUTE_SHADER; break;
        }
        
        result.shader_id = glCreateShader(gl_type);
        const char* source_cstr = source.c_str();
        glShaderSource(result.shader_id, 1, &source_cstr, nullptr);
        glCompileShader(result.shader_id);
        
        int success;
        glGetShaderiv(result.shader_id, GL_COMPILE_STATUS, &success);
        result.success = success == GL_TRUE;
        
        char info_log[512];
        glGetShaderInfoLog(result.shader_id, 512, nullptr, info_log);
        if (result.success) {
            result.info_log = info_log;
        } else {
            result.error_log = info_log;
        }
        #endif
        
        return result;
    }

    static std::vector<std::string> query_active_uniforms(unsigned int program_id) {
        std::vector<std::string> uniforms;
        
        #ifdef ECSCOPE_OPENGL
        int uniform_count;
        glGetProgramiv(program_id, GL_ACTIVE_UNIFORMS, &uniform_count);
        
        for (int i = 0; i < uniform_count; ++i) {
            char name[256];
            int length, size;
            unsigned int type;
            glGetActiveUniform(program_id, i, sizeof(name), &length, &size, &type, name);
            uniforms.emplace_back(name);
        }
        #endif
        
        return uniforms;
    }

    static std::vector<std::string> query_active_attributes(unsigned int program_id) {
        std::vector<std::string> attributes;
        
        #ifdef ECSCOPE_OPENGL
        int attribute_count;
        glGetProgramiv(program_id, GL_ACTIVE_ATTRIBUTES, &attribute_count);
        
        for (int i = 0; i < attribute_count; ++i) {
            char name[256];
            int length, size;
            unsigned int type;
            glGetActiveAttrib(program_id, i, sizeof(name), &length, &size, &type, name);
            attributes.emplace_back(name);
        }
        #endif
        
        return attributes;
    }
};

// Framebuffer testing utilities
class FramebufferTester {
public:
    static std::vector<PixelData> capture_framebuffer(size_t x, size_t y, 
                                                     size_t width, size_t height) {
        std::vector<PixelData> pixels(width * height);
        
        #ifdef ECSCOPE_OPENGL
        glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        
        // Flip vertically (OpenGL has origin at bottom-left)
        for (size_t row = 0; row < height / 2; ++row) {
            for (size_t col = 0; col < width; ++col) {
                size_t top_idx = row * width + col;
                size_t bottom_idx = (height - 1 - row) * width + col;
                std::swap(pixels[top_idx], pixels[bottom_idx]);
            }
        }
        #endif
        
        return pixels;
    }

    static bool verify_framebuffer_completeness() {
        #ifdef ECSCOPE_OPENGL
        unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        return status == GL_FRAMEBUFFER_COMPLETE;
        #else
        return true;
        #endif
    }

    static std::string get_framebuffer_status_string() {
        #ifdef ECSCOPE_OPENGL
        unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        switch (status) {
            case GL_FRAMEBUFFER_COMPLETE: return "Complete";
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "Incomplete attachment";
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "Missing attachment";
            case GL_FRAMEBUFFER_UNSUPPORTED: return "Unsupported";
            default: return "Unknown status";
        }
        #else
        return "Not available";
        #endif
    }
};

// Rendering test fixture
class RenderingTestFixture : public TestFixture {
public:
    void setup() override {
        // Initialize OpenGL context for testing
        initialize_gl_context();
        
        // Create test framebuffer
        create_test_framebuffer();
        
        // Initialize state validator
        initial_gl_state_ = OpenGLStateValidator::capture_state();
    }

    void teardown() override {
        // Verify GL state hasn't been corrupted
        auto final_state = OpenGLStateValidator::capture_state();
        if (!OpenGLStateValidator::compare_states(initial_gl_state_, final_state)) {
            std::cerr << "OpenGL state was modified during test:\n"
                      << OpenGLStateValidator::state_diff(initial_gl_state_, final_state);
        }
        
        cleanup_test_framebuffer();
        cleanup_gl_context();
    }

protected:
    unsigned int test_framebuffer_{0};
    unsigned int test_color_texture_{0};
    unsigned int test_depth_texture_{0};
    static constexpr size_t test_width_ = 256;
    static constexpr size_t test_height_ = 256;
    OpenGLStateValidator::GLState initial_gl_state_;

    void initialize_gl_context() {
        // Platform-specific OpenGL context creation would go here
        // For testing, we assume a context is already available
    }

    void cleanup_gl_context() {
        // Platform-specific cleanup
    }

    void create_test_framebuffer() {
        #ifdef ECSCOPE_OPENGL
        // Create framebuffer
        glGenFramebuffers(1, &test_framebuffer_);
        glBindFramebuffer(GL_FRAMEBUFFER, test_framebuffer_);
        
        // Create color texture
        glGenTextures(1, &test_color_texture_);
        glBindTexture(GL_TEXTURE_2D, test_color_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, test_width_, test_height_, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                              GL_TEXTURE_2D, test_color_texture_, 0);
        
        // Create depth texture
        glGenTextures(1, &test_depth_texture_);
        glBindTexture(GL_TEXTURE_2D, test_depth_texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, test_width_, test_height_, 0, 
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                              GL_TEXTURE_2D, test_depth_texture_, 0);
        
        // Verify framebuffer completeness
        ASSERT_TRUE(FramebufferTester::verify_framebuffer_completeness());
        
        // Bind default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        #endif
    }

    void cleanup_test_framebuffer() {
        #ifdef ECSCOPE_OPENGL
        if (test_color_texture_) {
            glDeleteTextures(1, &test_color_texture_);
            test_color_texture_ = 0;
        }
        if (test_depth_texture_) {
            glDeleteTextures(1, &test_depth_texture_);
            test_depth_texture_ = 0;
        }
        if (test_framebuffer_) {
            glDeleteFramebuffers(1, &test_framebuffer_);
            test_framebuffer_ = 0;
        }
        #endif
    }

    std::vector<PixelData> render_to_texture_and_capture() {
        #ifdef ECSCOPE_OPENGL
        glBindFramebuffer(GL_FRAMEBUFFER, test_framebuffer_);
        glViewport(0, 0, test_width_, test_height_);
        
        // Clear the framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render your test content here...
        
        // Capture the result
        auto pixels = FramebufferTester::capture_framebuffer(0, 0, test_width_, test_height_);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return pixels;
        #else
        return {};
        #endif
    }

    void compare_with_reference(const std::vector<PixelData>& test_result,
                               const std::string& reference_name,
                               double tolerance = 1.0) {
        auto reference = ImageComparator::load_reference_image(
            "test_data/references/" + reference_name + ".png");
        
        auto comparison = ImageComparator::compare_images(
            reference, test_result, test_width_, test_height_, tolerance);
        
        if (!comparison.images_match) {
            // Save failed test image for debugging
            ImageComparator::save_image(test_result, test_width_, test_height_,
                                      "test_output/" + reference_name + "_failed.png");
            
            std::stringstream error_msg;
            error_msg << "Image comparison failed for " << reference_name << ":\n"
                      << "Different pixels: " << comparison.different_pixels << "\n"
                      << "MSE: " << comparison.mse << "\n"
                      << "PSNR: " << comparison.psnr << "\n"
                      << "SSIM: " << comparison.ssim;
            
            throw AssertionFailure(error_msg.str(), __FILE__, __LINE__);
        }
    }
};

// Specific rendering tests
class BasicRenderingTest : public RenderingTestFixture {
public:
    BasicRenderingTest() : RenderingTestFixture() {
        context_.name = "Basic Rendering Test";
        context_.category = TestCategory::RENDERING;
    }

    void run() override {
        // Test basic triangle rendering
        auto result = render_triangle();
        compare_with_reference(result, "basic_triangle");
    }

private:
    std::vector<PixelData> render_triangle() {
        // Implementation would render a simple triangle and return the pixels
        return render_to_texture_and_capture();
    }
};

class ShaderCompilationTest : public TestCase {
public:
    ShaderCompilationTest() : TestCase("Shader Compilation", TestCategory::RENDERING) {}

    void run() override {
        // Test vertex shader compilation
        std::string vertex_source = R"(
            #version 330 core
            layout (location = 0) in vec3 position;
            uniform mat4 mvp;
            void main() {
                gl_Position = mvp * vec4(position, 1.0);
            }
        )";

        auto vertex_result = ShaderValidator::validate_vertex_shader(vertex_source);
        ASSERT_TRUE(vertex_result.success);

        // Test fragment shader compilation
        std::string fragment_source = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec3 color;
            void main() {
                FragColor = vec4(color, 1.0);
            }
        )";

        auto fragment_result = ShaderValidator::validate_fragment_shader(fragment_source);
        ASSERT_TRUE(fragment_result.success);

        // Test program linking
        std::vector<unsigned int> shaders = {vertex_result.shader_id, fragment_result.shader_id};
        auto program_result = ShaderValidator::validate_program(shaders);
        ASSERT_TRUE(program_result.success);

        // Verify expected uniforms are present
        ASSERT_TRUE(std::find(program_result.active_uniforms.begin(),
                             program_result.active_uniforms.end(),
                             "mvp") != program_result.active_uniforms.end());
        ASSERT_TRUE(std::find(program_result.active_uniforms.begin(),
                             program_result.active_uniforms.end(),
                             "color") != program_result.active_uniforms.end());
    }
};

// Performance test for rendering
class RenderingPerformanceTest : public BenchmarkTest {
public:
    RenderingPerformanceTest() : BenchmarkTest("Rendering Performance", 1000) {
        context_.category = TestCategory::PERFORMANCE;
    }

    void benchmark() override {
        // Benchmark drawing many objects
        draw_test_scene();
    }

private:
    void draw_test_scene() {
        // Implementation would draw a complex scene for performance testing
    }
};

} // namespace ecscope::testing