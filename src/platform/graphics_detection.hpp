#pragma once

/**
 * @file platform/graphics_detection.hpp
 * @brief Comprehensive GPU and Graphics Capability Detection System
 * 
 * This system provides detailed detection and analysis of graphics hardware,
 * APIs, and capabilities across different platforms. It integrates with the
 * main hardware detection system to provide complete graphics subsystem
 * information for optimization and educational purposes.
 * 
 * Key Features:
 * - Multi-GPU detection and enumeration
 * - Graphics API support detection (OpenGL, Vulkan, DirectX, Metal, OpenCL, CUDA)
 * - GPU memory hierarchy analysis
 * - Compute capability detection and benchmarking
 * - Driver version and feature support analysis
 * - Cross-platform graphics optimization recommendations
 * - Educational graphics programming insights
 * 
 * Educational Value:
 * - Graphics pipeline architecture analysis
 * - GPU vs CPU performance comparisons
 * - Memory bandwidth and latency analysis
 * - API feature support matrix
 * - Real-time rendering capability assessment
 * - Compute shader performance analysis
 * 
 * @author ECScope Educational ECS Framework - Graphics Detection
 * @date 2025
 */

#include "hardware_detection.hpp"
#include "core/types.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>

// Platform-specific includes
#ifdef ECSCOPE_PLATFORM_WINDOWS
    #include <d3d11.h>
    #include <d3d12.h>
    #include <dxgi1_6.h>
    #include <wrl.h>
    using Microsoft::WRL::ComPtr;
#endif

#ifdef ECSCOPE_PLATFORM_LINUX
    #include <GL/glx.h>
    #include <X11/Xlib.h>
#endif

#ifdef ECSCOPE_PLATFORM_MACOS
    #include <Metal/Metal.h>
    #include <OpenGL/OpenGL.h>
#endif

// OpenGL detection (cross-platform)
#ifndef ECSCOPE_NO_OPENGL
    #ifdef ECSCOPE_PLATFORM_WINDOWS
        #include <GL/gl.h>
        #include <GL/wglext.h>
    #else
        #include <GL/gl.h>
    #endif
#endif

namespace ecscope::platform::graphics {

//=============================================================================
// Graphics API Enumeration
//=============================================================================

/**
 * @brief Supported graphics APIs
 */
enum class GraphicsAPI : u8 {
    Unknown,
    OpenGL,
    OpenGL_ES,
    Vulkan,
    DirectX_9,
    DirectX_10,
    DirectX_11,
    DirectX_12,
    Metal,
    WebGL,
    Software_Renderer
};

/**
 * @brief Compute API support
 */
enum class ComputeAPI : u8 {
    Unknown,
    OpenCL,
    CUDA,
    DirectCompute,
    Metal_Performance_Shaders,
    Vulkan_Compute,
    OpenGL_Compute
};

/**
 * @brief GPU vendor enumeration
 */
enum class GpuVendor : u8 {
    Unknown,
    NVIDIA,
    AMD,
    Intel,
    Apple,
    ARM,
    Qualcomm,
    PowerVR,
    Mali,
    Adreno,
    Software
};

/**
 * @brief GPU type classification
 */
enum class GpuType : u8 {
    Unknown,
    Discrete,           // Dedicated graphics card
    Integrated,         // Integrated graphics
    Virtual,            // Virtual/emulated GPU
    Software,           // Software renderer
    External            // External GPU (eGPU)
};

//=============================================================================
// Graphics API Information Structures
//=============================================================================

/**
 * @brief OpenGL capability information
 */
struct OpenGLInfo {
    std::string version;
    std::string glsl_version;
    std::string renderer_string;
    std::string vendor_string;
    
    // Core capabilities
    u32 max_texture_size = 0;
    u32 max_3d_texture_size = 0;
    u32 max_cube_map_texture_size = 0;
    u32 max_array_texture_layers = 0;
    u32 max_renderbuffer_size = 0;
    u32 max_viewport_width = 0;
    u32 max_viewport_height = 0;
    
    // Shader capabilities
    u32 max_vertex_attribs = 0;
    u32 max_vertex_uniform_components = 0;
    u32 max_fragment_uniform_components = 0;
    u32 max_vertex_texture_image_units = 0;
    u32 max_texture_image_units = 0;
    u32 max_combined_texture_image_units = 0;
    
    // Compute shader support (OpenGL 4.3+)
    bool supports_compute_shaders = false;
    u32 max_compute_work_group_count[3] = {0, 0, 0};
    u32 max_compute_work_group_size[3] = {0, 0, 0};
    u32 max_compute_work_group_invocations = 0;
    
    // Extension support
    std::vector<std::string> extensions;
    
    // Feature support
    bool supports_tessellation = false;
    bool supports_geometry_shaders = false;
    bool supports_instanced_rendering = false;
    bool supports_texture_compression = false;
    bool supports_anisotropic_filtering = false;
    bool supports_multisample = false;
    bool supports_debug_output = false;
    
    f32 max_anisotropy = 1.0f;
    u32 max_samples = 0;
    
    bool is_core_profile = false;
    bool is_compatibility_profile = false;
    
    std::string get_capability_summary() const;
    f32 get_feature_score() const;
};

/**
 * @brief Vulkan capability information
 */
struct VulkanInfo {
    std::string api_version;
    std::string driver_version;
    std::string device_name;
    std::string vendor_name;
    
    // Physical device properties
    u32 api_version_raw = 0;
    u32 driver_version_raw = 0;
    u32 vendor_id = 0;
    u32 device_id = 0;
    u32 device_type = 0;
    
    // Memory properties
    struct MemoryHeap {
        u64 size_bytes;
        bool device_local;
        bool host_visible;
        bool host_coherent;
    };
    std::vector<MemoryHeap> memory_heaps;
    u64 total_device_memory = 0;
    u64 total_host_memory = 0;
    
    // Queue family properties
    struct QueueFamily {
        u32 queue_count;
        bool graphics_support;
        bool compute_support;
        bool transfer_support;
        bool present_support;
    };
    std::vector<QueueFamily> queue_families;
    
    // Device limits
    u32 max_image_dimension_1d = 0;
    u32 max_image_dimension_2d = 0;
    u32 max_image_dimension_3d = 0;
    u32 max_image_dimension_cube = 0;
    u32 max_image_array_layers = 0;
    u32 max_texel_buffer_elements = 0;
    u32 max_uniform_buffer_range = 0;
    u32 max_storage_buffer_range = 0;
    u32 max_push_constants_size = 0;
    u32 max_memory_allocation_count = 0;
    u32 max_bound_descriptor_sets = 0;
    
    // Compute capabilities
    u32 max_compute_shared_memory_size = 0;
    u32 max_compute_work_group_count[3] = {0, 0, 0};
    u32 max_compute_work_group_size[3] = {0, 0, 0};
    u32 max_compute_work_group_invocations = 0;
    
    // Extensions and layers
    std::vector<std::string> instance_extensions;
    std::vector<std::string> device_extensions;
    std::vector<std::string> validation_layers;
    
    // Feature support
    bool supports_geometry_shader = false;
    bool supports_tessellation_shader = false;
    bool supports_sample_rate_shading = false;
    bool supports_dual_src_blend = false;
    bool supports_logic_op = false;
    bool supports_multi_draw_indirect = false;
    bool supports_draw_indirect_first_instance = false;
    bool supports_depth_clamp = false;
    bool supports_depth_bias_clamp = false;
    bool supports_fill_mode_non_solid = false;
    bool supports_depth_bounds = false;
    bool supports_wide_lines = false;
    bool supports_large_points = false;
    bool supports_alpha_to_one = false;
    bool supports_multi_viewport = false;
    bool supports_sampler_anisotropy = false;
    bool supports_texture_compression_etc2 = false;
    bool supports_texture_compression_astc_ldr = false;
    bool supports_texture_compression_bc = false;
    bool supports_occlusion_query_precise = false;
    bool supports_pipeline_statistics_query = false;
    bool supports_vertex_pipeline_stores_and_atomics = false;
    bool supports_fragment_stores_and_atomics = false;
    bool supports_shader_tessellation_and_geometry_point_size = false;
    bool supports_shader_image_gather_extended = false;
    bool supports_shader_storage_image_extended_formats = false;
    bool supports_shader_storage_image_multisample = false;
    bool supports_shader_storage_image_read_without_format = false;
    bool supports_shader_storage_image_write_without_format = false;
    bool supports_shader_uniform_buffer_array_dynamic_indexing = false;
    bool supports_shader_sampled_image_array_dynamic_indexing = false;
    bool supports_shader_storage_buffer_array_dynamic_indexing = false;
    bool supports_shader_storage_image_array_dynamic_indexing = false;
    bool supports_shader_clip_distance = false;
    bool supports_shader_cull_distance = false;
    bool supports_shader_float64 = false;
    bool supports_shader_int64 = false;
    bool supports_shader_int16 = false;
    bool supports_shader_resource_residency = false;
    bool supports_shader_resource_min_lod = false;
    bool supports_sparse_binding = false;
    bool supports_sparse_residency_buffer = false;
    bool supports_sparse_residency_image_2d = false;
    bool supports_sparse_residency_image_3d = false;
    bool supports_sparse_residency2_samples = false;
    bool supports_sparse_residency4_samples = false;
    bool supports_sparse_residency8_samples = false;
    bool supports_sparse_residency16_samples = false;
    bool supports_sparse_residency_aliased = false;
    bool supports_variable_multisample_rate = false;
    bool supports_inherited_queries = false;
    
    std::string get_capability_summary() const;
    f32 get_feature_score() const;
    bool supports_raytracing() const;
};

/**
 * @brief DirectX capability information
 */
struct DirectXInfo {
    std::string version;              // "11.1", "12_1", etc.
    std::string feature_level;        // "11_1", "12_0", etc.
    std::string adapter_description;
    
    u64 dedicated_video_memory = 0;
    u64 dedicated_system_memory = 0;
    u64 shared_system_memory = 0;
    
    // DirectX 11 specific
    bool supports_dx11_compute = false;
    bool supports_dx11_tessellation = false;
    bool supports_dx11_multithreading = false;
    
    // DirectX 12 specific
    bool supports_dx12 = false;
    bool supports_dx12_raytracing = false;
    bool supports_dx12_variable_rate_shading = false;
    bool supports_dx12_mesh_shaders = false;
    bool supports_dx12_sampler_feedback = false;
    
    std::string get_capability_summary() const;
    f32 get_feature_score() const;
};

/**
 * @brief Metal capability information
 */
struct MetalInfo {
    std::string device_name;
    std::string family_name;
    bool is_low_power = false;
    bool is_headless = false;
    bool is_removable = false;
    
    u64 recommended_max_working_set_size = 0;
    u64 max_buffer_length = 0;
    u64 max_texture_width_1d = 0;
    u64 max_texture_width_2d = 0;
    u64 max_texture_height_2d = 0;
    u64 max_texture_depth_3d = 0;
    
    bool supports_shader_debugger = false;
    bool supports_function_pointers = false;
    bool supports_dynamic_libraries = false;
    bool supports_render_dynamic_libraries = false;
    bool supports_raytracing = false;
    bool supports_primitive_motion_blur = false;
    
    std::string get_capability_summary() const;
    f32 get_feature_score() const;
};

//=============================================================================
// Compute API Information
//=============================================================================

/**
 * @brief OpenCL capability information
 */
struct OpenCLInfo {
    std::string platform_name;
    std::string platform_vendor;
    std::string platform_version;
    std::string device_name;
    std::string device_vendor;
    std::string device_version;
    std::string driver_version;
    
    u32 compute_units = 0;
    u32 max_clock_frequency = 0;
    u64 global_memory_size = 0;
    u64 local_memory_size = 0;
    u64 max_constant_buffer_size = 0;
    u64 max_memory_allocation_size = 0;
    
    u32 max_work_group_size = 0;
    std::array<u32, 3> max_work_item_sizes = {0, 0, 0};
    u32 max_work_item_dimensions = 0;
    
    bool supports_images = false;
    bool supports_double_precision = false;
    bool supports_half_precision = false;
    bool supports_unified_memory = false;
    
    std::vector<std::string> extensions;
    
    std::string get_capability_summary() const;
    f32 get_compute_score() const;
};

/**
 * @brief CUDA capability information
 */
struct CudaInfo {
    std::string device_name;
    u32 major_compute_capability = 0;
    u32 minor_compute_capability = 0;
    
    u32 multiprocessor_count = 0;
    u32 cuda_cores = 0;
    u32 max_threads_per_multiprocessor = 0;
    u32 max_threads_per_block = 0;
    std::array<u32, 3> max_threads_per_block_dimension = {0, 0, 0};
    std::array<u32, 3> max_grid_dimension = {0, 0, 0};
    
    u64 total_global_memory = 0;
    u64 shared_memory_per_block = 0;
    u64 total_constant_memory = 0;
    u32 warp_size = 0;
    u32 max_pitch = 0;
    u32 registers_per_block = 0;
    
    u32 clock_rate_khz = 0;
    u32 memory_clock_rate_khz = 0;
    u32 memory_bus_width = 0;
    u64 l2_cache_size = 0;
    
    bool supports_unified_memory = false;
    bool supports_managed_memory = false;
    bool supports_concurrent_kernels = false;
    bool supports_async_engine = false;
    bool supports_surface_load_store = false;
    
    std::string get_capability_summary() const;
    f32 get_compute_score() const;
    std::string get_compute_capability_string() const;
};

//=============================================================================
// GPU Device Information
//=============================================================================

/**
 * @brief Comprehensive GPU device information
 */
struct GpuDevice {
    std::string device_name;
    std::string device_id;
    GpuVendor vendor = GpuVendor::Unknown;
    GpuType type = GpuType::Unknown;
    
    // Memory information
    u64 total_memory_bytes = 0;
    u64 available_memory_bytes = 0;
    u64 dedicated_memory_bytes = 0;
    u64 shared_memory_bytes = 0;
    f64 memory_bandwidth_gbps = 0.0;
    
    // Performance characteristics
    u32 base_clock_mhz = 0;
    u32 boost_clock_mhz = 0;
    u32 memory_clock_mhz = 0;
    u32 shader_units = 0;
    u32 texture_units = 0;
    u32 rop_units = 0;
    f32 compute_performance_tflops = 0.0f;
    f32 pixel_fillrate_gpixels = 0.0f;
    f32 texture_fillrate_gtexels = 0.0f;
    
    // API support
    std::optional<OpenGLInfo> opengl_info;
    std::optional<VulkanInfo> vulkan_info;
    std::optional<DirectXInfo> directx_info;
    std::optional<MetalInfo> metal_info;
    
    // Compute API support
    std::optional<OpenCLInfo> opencl_info;
    std::optional<CudaInfo> cuda_info;
    
    // Feature support
    bool supports_hardware_raytracing = false;
    bool supports_variable_rate_shading = false;
    bool supports_mesh_shaders = false;
    bool supports_async_compute = false;
    bool supports_multi_gpu = false;
    bool supports_virtual_reality = false;
    
    // Power and thermal
    f32 tdp_watts = 0.0f;
    f32 current_temperature_celsius = 0.0f;
    f32 current_power_consumption_watts = 0.0f;
    f32 fan_speed_percent = 0.0f;
    
    // Methods
    f32 get_overall_performance_score() const;
    std::string get_detailed_description() const;
    std::vector<GraphicsAPI> get_supported_graphics_apis() const;
    std::vector<ComputeAPI> get_supported_compute_apis() const;
    bool is_suitable_for_gaming() const;
    bool is_suitable_for_compute() const;
    bool is_suitable_for_machine_learning() const;
};

//=============================================================================
// Graphics System Information
//=============================================================================

/**
 * @brief Complete graphics system information
 */
struct GraphicsSystemInfo {
    std::vector<GpuDevice> devices;
    std::string primary_display_adapter;
    
    // Multi-GPU configuration
    bool has_multi_gpu = false;
    bool supports_sli_crossfire = false;
    std::string multi_gpu_configuration;
    
    // Display information
    struct DisplayInfo {
        std::string name;
        u32 width_pixels = 0;
        u32 height_pixels = 0;
        u32 refresh_rate_hz = 0;
        u32 bit_depth = 0;
        f32 diagonal_inches = 0.0f;
        f32 dpi = 0.0f;
        bool is_primary = false;
        bool supports_hdr = false;
        bool supports_variable_refresh = false;
        std::string color_profile;
    };
    std::vector<DisplayInfo> displays;
    
    // System graphics capabilities
    bool supports_hardware_acceleration = false;
    bool supports_video_decode = false;
    bool supports_video_encode = false;
    std::vector<std::string> supported_video_codecs;
    
    // Performance characteristics
    f32 total_compute_performance = 0.0f;
    f32 total_memory_bandwidth = 0.0f;
    u64 total_graphics_memory = 0.0f;
    
    // Methods
    const GpuDevice* get_primary_gpu() const;
    const GpuDevice* get_most_powerful_gpu() const;
    std::vector<const GpuDevice*> get_discrete_gpus() const;
    std::vector<const GpuDevice*> get_integrated_gpus() const;
    f32 get_system_graphics_score() const;
    std::string get_graphics_summary() const;
    std::vector<std::string> get_optimization_recommendations() const;
};

//=============================================================================
// Graphics Detection Engine
//=============================================================================

/**
 * @brief Comprehensive graphics hardware detection system
 */
class GraphicsDetector {
private:
    mutable std::optional<GraphicsSystemInfo> cached_graphics_info_;
    mutable std::chrono::steady_clock::time_point last_detection_;
    std::chrono::seconds cache_validity_duration_{30}; // 30 seconds for dynamic info
    
    // Platform-specific contexts
#ifdef ECSCOPE_PLATFORM_WINDOWS
    mutable ComPtr<IDXGIFactory6> dxgi_factory_;
    mutable bool dxgi_initialized_ = false;
#endif
    
#ifdef ECSCOPE_PLATFORM_LINUX
    mutable Display* x11_display_ = nullptr;
    mutable bool x11_initialized_ = false;
#endif
    
public:
    GraphicsDetector();
    ~GraphicsDetector();
    
    // Non-copyable
    GraphicsDetector(const GraphicsDetector&) = delete;
    GraphicsDetector& operator=(const GraphicsDetector&) = delete;
    
    // Main detection methods
    const GraphicsSystemInfo& get_graphics_info() const;
    void refresh_graphics_info();
    void set_cache_validity(std::chrono::seconds duration);
    bool is_cache_valid() const;
    void clear_cache();
    
    // Specific API detection
    std::vector<OpenGLInfo> detect_opengl_capabilities() const;
    std::vector<VulkanInfo> detect_vulkan_capabilities() const;
    std::vector<DirectXInfo> detect_directx_capabilities() const;
    std::vector<MetalInfo> detect_metal_capabilities() const;
    std::vector<OpenCLInfo> detect_opencl_capabilities() const;
    std::vector<CudaInfo> detect_cuda_capabilities() const;
    
    // Capability queries
    bool supports_graphics_api(GraphicsAPI api) const;
    bool supports_compute_api(ComputeAPI api) const;
    std::vector<GraphicsAPI> get_supported_graphics_apis() const;
    std::vector<ComputeAPI> get_supported_compute_apis() const;
    
    // Performance analysis
    f32 estimate_graphics_performance() const;
    f32 estimate_compute_performance() const;
    std::string get_graphics_recommendations() const;
    std::string analyze_graphics_bottlenecks() const;
    
private:
    // Core detection implementation
    GraphicsSystemInfo detect_graphics_system() const;
    std::vector<GpuDevice> enumerate_gpu_devices() const;
    std::vector<GraphicsSystemInfo::DisplayInfo> enumerate_displays() const;
    
    // Platform-specific implementations
#ifdef ECSCOPE_PLATFORM_WINDOWS
    bool initialize_dxgi() const;
    void cleanup_dxgi() const;
    std::vector<GpuDevice> detect_windows_gpus() const;
    std::vector<GraphicsSystemInfo::DisplayInfo> detect_windows_displays() const;
    DirectXInfo detect_directx_info(IDXGIAdapter* adapter) const;
#endif
    
#ifdef ECSCOPE_PLATFORM_LINUX
    bool initialize_x11() const;
    void cleanup_x11() const;
    std::vector<GpuDevice> detect_linux_gpus() const;
    std::vector<GraphicsSystemInfo::DisplayInfo> detect_linux_displays() const;
    OpenGLInfo detect_glx_info() const;
#endif
    
#ifdef ECSCOPE_PLATFORM_MACOS
    std::vector<GpuDevice> detect_macos_gpus() const;
    std::vector<GraphicsSystemInfo::DisplayInfo> detect_macos_displays() const;
    MetalInfo detect_metal_info() const;
#endif
    
    // API-specific detection helpers
    OpenGLInfo detect_opengl_context_info() const;
    VulkanInfo detect_vulkan_device_info(void* device) const;
    OpenCLInfo detect_opencl_device_info(void* device) const;
    CudaInfo detect_cuda_device_info(int device_id) const;
    
    // Utility functions
    GpuVendor identify_gpu_vendor(const std::string& name) const;
    GpuVendor identify_gpu_vendor_by_id(u32 vendor_id) const;
    GpuType determine_gpu_type(const std::string& name, const std::string& description) const;
    f32 estimate_gpu_performance(const GpuDevice& device) const;
    bool is_cache_outdated() const;
};

//=============================================================================
// Graphics Benchmarking and Validation
//=============================================================================

/**
 * @brief Graphics performance benchmarking
 */
class GraphicsBenchmark {
public:
    struct GraphicsBenchmarkResult {
        std::string benchmark_name;
        GraphicsAPI api_used;
        std::string gpu_device;
        
        f32 average_fps = 0.0f;
        f32 min_fps = 0.0f;
        f32 max_fps = 0.0f;
        f32 frame_time_ms = 0.0f;
        f32 gpu_utilization_percent = 0.0f;
        f32 memory_utilization_percent = 0.0f;
        f32 power_consumption_watts = 0.0f;
        
        u32 triangles_per_second = 0;
        u32 pixels_per_second = 0;
        f32 memory_bandwidth_utilized_gbps = 0.0f;
        
        std::string get_performance_summary() const;
    };
    
    // Benchmark execution
    GraphicsBenchmarkResult run_triangle_throughput_test(GraphicsAPI api) const;
    GraphicsBenchmarkResult run_pixel_fillrate_test(GraphicsAPI api) const;
    GraphicsBenchmarkResult run_texture_bandwidth_test(GraphicsAPI api) const;
    GraphicsBenchmarkResult run_compute_shader_test(ComputeAPI api) const;
    
    // Comparison and analysis
    std::vector<GraphicsBenchmarkResult> compare_graphics_apis() const;
    std::string analyze_graphics_performance() const;
    
private:
    const GraphicsDetector& detector_;
    
public:
    explicit GraphicsBenchmark(const GraphicsDetector& detector) : detector_(detector) {}
};

//=============================================================================
// Global Graphics Detection
//=============================================================================

/**
 * @brief Get the global graphics detector instance
 */
GraphicsDetector& get_graphics_detector();

/**
 * @brief Initialize the global graphics detection system
 */
void initialize_graphics_detection();

/**
 * @brief Cleanup the global graphics detection system
 */
void shutdown_graphics_detection();

/**
 * @brief Quick graphics detection helpers
 */
namespace quick_graphics {
    bool has_discrete_gpu();
    bool supports_vulkan();
    bool supports_raytracing();
    bool supports_compute_shaders();
    std::string get_primary_gpu_name();
    f32 get_graphics_memory_gb();
    std::vector<std::string> get_supported_apis();
    std::string get_graphics_summary();
}

} // namespace ecscope::platform::graphics