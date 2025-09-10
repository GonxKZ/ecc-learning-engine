/**
 * @file vulkan_texture.cpp
 * @brief Vulkan Texture Resource Management Implementation
 * 
 * Complete texture resource management including creation, updates,
 * mipmap generation, and format conversions.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "ecscope/rendering/vulkan_backend.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ecscope::rendering {

// =============================================================================
// TEXTURE RESOURCE MANAGEMENT
// =============================================================================

TextureHandle VulkanRenderer::create_texture(const TextureDesc& desc, const void* initial_data) {
    if (desc.width == 0 || desc.height == 0) {
        std::cerr << "Cannot create texture with zero dimensions" << std::endl;
        return TextureHandle{};
    }
    
    uint64_t handle_id = next_resource_id_.fetch_add(1);
    VulkanTexture vk_texture;
    vk_texture.format = texture_format_to_vulkan(desc.format);
    vk_texture.extent = {desc.width, desc.height, desc.depth};
    vk_texture.mip_levels = desc.mip_levels;
    vk_texture.array_layers = desc.array_layers;
    vk_texture.samples = samples_to_vulkan(desc.samples);
    vk_texture.debug_name = desc.debug_name;
    vk_texture.current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    // Determine image usage
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (desc.render_target) {
        if (desc.depth_stencil) {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        } else {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }
    if (desc.mip_levels > 1) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // For mipmap generation
    }
    
    // Create the image
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = desc.depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
    image_info.extent = vk_texture.extent;
    image_info.mipLevels = vk_texture.mip_levels;
    image_info.arrayLayers = vk_texture.array_layers;
    image_info.format = vk_texture.format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = vk_texture.samples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkResult result = vkCreateImage(device_, &image_info, nullptr, &vk_texture.image);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create texture image!" << std::endl;
        return TextureHandle{};
    }
    
    // Allocate memory
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (!allocate_image_memory(vk_texture.image, properties, vk_texture.memory)) {
        vkDestroyImage(device_, vk_texture.image, nullptr);
        return TextureHandle{};
    }
    
    // Create image view
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = vk_texture.image;
    view_info.viewType = desc.array_layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : 
                        (desc.depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D);
    view_info.format = vk_texture.format;
    view_info.subresourceRange.aspectMask = desc.depth_stencil ? 
        VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = vk_texture.mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = vk_texture.array_layers;
    
    result = vkCreateImageView(device_, &view_info, nullptr, &vk_texture.image_view);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create texture image view!" << std::endl;
        vkFreeMemory(device_, vk_texture.memory, nullptr);
        vkDestroyImage(device_, vk_texture.image, nullptr);
        return TextureHandle{};
    }
    
    // Create sampler
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = device_properties_.limits.maxSamplerAnisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = static_cast<float>(vk_texture.mip_levels);
    
    result = vkCreateSampler(device_, &sampler_info, nullptr, &vk_texture.sampler);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create texture sampler!" << std::endl;
        vkDestroyImageView(device_, vk_texture.image_view, nullptr);
        vkFreeMemory(device_, vk_texture.memory, nullptr);
        vkDestroyImage(device_, vk_texture.image, nullptr);
        return TextureHandle{};
    }
    
    // Set debug name if validation layers are enabled
    if (enable_validation_layers_ && !desc.debug_name.empty()) {
        VkDebugUtilsObjectNameInfoEXT name_info{};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_IMAGE;
        name_info.objectHandle = reinterpret_cast<uint64_t>(vk_texture.image);
        name_info.pObjectName = desc.debug_name.c_str();
        
        auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device_, "vkSetDebugUtilsObjectNameEXT");
        if (func != nullptr) {
            func(device_, &name_info);
        }
    }
    
    // Upload initial data if provided
    if (initial_data) {
        // Transition to transfer destination layout
        transition_image_layout(vk_texture.image, vk_texture.format, 
                               VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                               vk_texture.mip_levels, vk_texture.array_layers);
        
        // Calculate image size for uploading
        size_t image_size = calculate_texture_size(desc);
        
        // Create staging buffer
        BufferDesc staging_desc;
        staging_desc.size = image_size;
        staging_desc.usage = BufferUsage::Staging;
        staging_desc.debug_name = desc.debug_name + "_staging";
        
        BufferHandle staging_buffer = create_buffer(staging_desc, initial_data);
        if (!staging_buffer.is_valid()) {
            std::cerr << "Failed to create staging buffer for texture!" << std::endl;
            destroy_texture(TextureHandle{handle_id});
            return TextureHandle{};
        }
        
        // Copy buffer to image
        copy_buffer_to_image(staging_buffer, vk_texture.image, desc.width, desc.height, desc.depth);
        destroy_buffer(staging_buffer);
        
        vk_texture.current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        
        // Generate mipmaps if requested
        if (desc.mip_levels > 1) {
            generate_mipmaps_internal(vk_texture.image, vk_texture.format, 
                                    desc.width, desc.height, vk_texture.mip_levels);
            vk_texture.current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        } else {
            // Transition to shader read layout
            transition_image_layout(vk_texture.image, vk_texture.format,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                   vk_texture.mip_levels, vk_texture.array_layers);
            vk_texture.current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }
    
    // Store the texture
    {
        std::lock_guard<std::mutex> lock(resource_mutex_);
        textures_[handle_id] = std::move(vk_texture);
    }
    
    return TextureHandle{handle_id};
}

void VulkanRenderer::destroy_texture(TextureHandle handle) {
    if (!handle.is_valid()) return;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = textures_.find(handle.id());
    if (it == textures_.end()) return;
    
    VulkanTexture& vk_texture = it->second;
    
    // Destroy Vulkan resources
    if (vk_texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device_, vk_texture.sampler, nullptr);
    }
    if (vk_texture.image_view != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, vk_texture.image_view, nullptr);
    }
    if (vk_texture.image != VK_NULL_HANDLE) {
        vkDestroyImage(device_, vk_texture.image, nullptr);
    }
    if (vk_texture.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device_, vk_texture.memory, nullptr);
    }
    
    textures_.erase(it);
}

void VulkanRenderer::update_texture(TextureHandle handle, uint32_t mip_level, uint32_t array_layer,
                                   uint32_t x, uint32_t y, uint32_t z,
                                   uint32_t width, uint32_t height, uint32_t depth,
                                   const void* data) {
    if (!handle.is_valid() || !data) return;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = textures_.find(handle.id());
    if (it == textures_.end()) return;
    
    VulkanTexture& vk_texture = it->second;
    
    // Calculate size for the update region
    size_t update_size = calculate_texture_region_size(vk_texture.format, width, height, depth);
    
    // Create staging buffer
    BufferDesc staging_desc;
    staging_desc.size = update_size;
    staging_desc.usage = BufferUsage::Staging;
    staging_desc.debug_name = vk_texture.debug_name + "_update_staging";
    
    BufferHandle staging_buffer = create_buffer(staging_desc, data);
    if (!staging_buffer.is_valid()) {
        std::cerr << "Failed to create staging buffer for texture update!" << std::endl;
        return;
    }
    
    // Transition image to transfer destination layout if needed
    if (vk_texture.current_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        transition_image_layout(vk_texture.image, vk_texture.format,
                               vk_texture.current_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               vk_texture.mip_levels, vk_texture.array_layers);
        vk_texture.current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }
    
    // Copy buffer to image region
    copy_buffer_to_image_region(staging_buffer, vk_texture.image, 
                               x, y, z, width, height, depth, mip_level, array_layer);
    
    // Transition back to shader read layout
    transition_image_layout(vk_texture.image, vk_texture.format,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           vk_texture.mip_levels, vk_texture.array_layers);
    vk_texture.current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    destroy_buffer(staging_buffer);
}

void VulkanRenderer::generate_mipmaps(TextureHandle handle) {
    if (!handle.is_valid()) return;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = textures_.find(handle.id());
    if (it == textures_.end()) return;
    
    VulkanTexture& vk_texture = it->second;
    
    if (vk_texture.mip_levels <= 1) {
        return; // No mipmaps to generate
    }
    
    generate_mipmaps_internal(vk_texture.image, vk_texture.format,
                            vk_texture.extent.width, vk_texture.extent.height,
                            vk_texture.mip_levels);
    
    vk_texture.current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

// =============================================================================
// TEXTURE HELPER FUNCTIONS
// =============================================================================

VkFormat VulkanRenderer::texture_format_to_vulkan(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:           return VK_FORMAT_R8_UNORM;
        case TextureFormat::RG8:          return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::RGB8:         return VK_FORMAT_R8G8B8_UNORM;
        case TextureFormat::RGBA8:        return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R16F:         return VK_FORMAT_R16_SFLOAT;
        case TextureFormat::RG16F:        return VK_FORMAT_R16G16_SFLOAT;
        case TextureFormat::RGB16F:       return VK_FORMAT_R16G16B16_SFLOAT;
        case TextureFormat::RGBA16F:      return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::R32F:         return VK_FORMAT_R32_SFLOAT;
        case TextureFormat::RG32F:        return VK_FORMAT_R32G32_SFLOAT;
        case TextureFormat::RGB32F:       return VK_FORMAT_R32G32B32_SFLOAT;
        case TextureFormat::RGBA32F:      return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::SRGB8:        return VK_FORMAT_R8G8B8_SRGB;
        case TextureFormat::SRGBA8:       return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::Depth16:      return VK_FORMAT_D16_UNORM;
        case TextureFormat::Depth24:      return VK_FORMAT_X8_D24_UNORM_PACK32;
        case TextureFormat::Depth32F:     return VK_FORMAT_D32_SFLOAT;
        case TextureFormat::Depth24Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::Depth32FStencil8: return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case TextureFormat::BC1_RGB:      return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        case TextureFormat::BC1_RGBA:     return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case TextureFormat::BC3_RGBA:     return VK_FORMAT_BC3_UNORM_BLOCK;
        case TextureFormat::BC4_R:        return VK_FORMAT_BC4_UNORM_BLOCK;
        case TextureFormat::BC5_RG:       return VK_FORMAT_BC5_UNORM_BLOCK;
        case TextureFormat::BC6H_RGB_UF16: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
        case TextureFormat::BC7_RGBA:     return VK_FORMAT_BC7_UNORM_BLOCK;
        default:                          return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

VkSampleCountFlagBits VulkanRenderer::samples_to_vulkan(uint32_t samples) {
    switch (samples) {
        case 1:  return VK_SAMPLE_COUNT_1_BIT;
        case 2:  return VK_SAMPLE_COUNT_2_BIT;
        case 4:  return VK_SAMPLE_COUNT_4_BIT;
        case 8:  return VK_SAMPLE_COUNT_8_BIT;
        case 16: return VK_SAMPLE_COUNT_16_BIT;
        case 32: return VK_SAMPLE_COUNT_32_BIT;
        case 64: return VK_SAMPLE_COUNT_64_BIT;
        default: return VK_SAMPLE_COUNT_1_BIT;
    }
}

size_t VulkanRenderer::calculate_texture_size(const TextureDesc& desc) {
    size_t bytes_per_pixel = get_format_size(desc.format);
    return desc.width * desc.height * desc.depth * desc.array_layers * bytes_per_pixel;
}

size_t VulkanRenderer::calculate_texture_region_size(VkFormat format, uint32_t width, uint32_t height, uint32_t depth) {
    size_t bytes_per_pixel = get_vulkan_format_size(format);
    return width * height * depth * bytes_per_pixel;
}

size_t VulkanRenderer::get_format_size(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:           return 1;
        case TextureFormat::RG8:          return 2;
        case TextureFormat::RGB8:         return 3;
        case TextureFormat::RGBA8:        return 4;
        case TextureFormat::R16F:         return 2;
        case TextureFormat::RG16F:        return 4;
        case TextureFormat::RGB16F:       return 6;
        case TextureFormat::RGBA16F:      return 8;
        case TextureFormat::R32F:         return 4;
        case TextureFormat::RG32F:        return 8;
        case TextureFormat::RGB32F:       return 12;
        case TextureFormat::RGBA32F:      return 16;
        case TextureFormat::SRGB8:        return 3;
        case TextureFormat::SRGBA8:       return 4;
        case TextureFormat::Depth16:      return 2;
        case TextureFormat::Depth24:      return 4;
        case TextureFormat::Depth32F:     return 4;
        case TextureFormat::Depth24Stencil8: return 4;
        case TextureFormat::Depth32FStencil8: return 8;
        // Compressed formats - these are block-based, this is a simplified calculation
        case TextureFormat::BC1_RGB:
        case TextureFormat::BC1_RGBA:     return 8;  // 8 bytes per 4x4 block
        case TextureFormat::BC3_RGBA:
        case TextureFormat::BC5_RG:
        case TextureFormat::BC6H_RGB_UF16:
        case TextureFormat::BC7_RGBA:     return 16; // 16 bytes per 4x4 block
        case TextureFormat::BC4_R:        return 8;  // 8 bytes per 4x4 block
        default:                          return 4;
    }
}

size_t VulkanRenderer::get_vulkan_format_size(VkFormat format) {
    // This is a simplified implementation - real applications should use a comprehensive lookup table
    switch (format) {
        case VK_FORMAT_R8_UNORM:                    return 1;
        case VK_FORMAT_R8G8_UNORM:                  return 2;
        case VK_FORMAT_R8G8B8_UNORM:                return 3;
        case VK_FORMAT_R8G8B8A8_UNORM:              return 4;
        case VK_FORMAT_R16_SFLOAT:                  return 2;
        case VK_FORMAT_R16G16_SFLOAT:               return 4;
        case VK_FORMAT_R16G16B16_SFLOAT:            return 6;
        case VK_FORMAT_R16G16B16A16_SFLOAT:         return 8;
        case VK_FORMAT_R32_SFLOAT:                  return 4;
        case VK_FORMAT_R32G32_SFLOAT:               return 8;
        case VK_FORMAT_R32G32B32_SFLOAT:            return 12;
        case VK_FORMAT_R32G32B32A32_SFLOAT:         return 16;
        case VK_FORMAT_D16_UNORM:                   return 2;
        case VK_FORMAT_D32_SFLOAT:                  return 4;
        case VK_FORMAT_D24_UNORM_S8_UINT:           return 4;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:          return 8;
        default:                                   return 4;
    }
}

void VulkanRenderer::transition_image_layout(VkImage image, VkFormat format, 
                                            VkImageLayout old_layout, VkImageLayout new_layout,
                                            uint32_t mip_levels, uint32_t layer_count) {
    VkCommandBuffer command_buffer = begin_single_time_commands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layer_count;
    
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("Unsupported layout transition!");
    }
    
    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    end_single_time_commands(command_buffer);
}

void VulkanRenderer::copy_buffer_to_image(BufferHandle buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth) {
    VkCommandBuffer command_buffer = begin_single_time_commands();
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, depth};
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = buffers_.find(buffer.id());
    if (it != buffers_.end()) {
        vkCmdCopyBufferToImage(command_buffer, it->second.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    
    end_single_time_commands(command_buffer);
}

void VulkanRenderer::copy_buffer_to_image_region(BufferHandle buffer, VkImage image,
                                                uint32_t x, uint32_t y, uint32_t z,
                                                uint32_t width, uint32_t height, uint32_t depth,
                                                uint32_t mip_level, uint32_t array_layer) {
    VkCommandBuffer command_buffer = begin_single_time_commands();
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mip_level;
    region.imageSubresource.baseArrayLayer = array_layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(z)};
    region.imageExtent = {width, height, depth};
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = buffers_.find(buffer.id());
    if (it != buffers_.end()) {
        vkCmdCopyBufferToImage(command_buffer, it->second.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
    
    end_single_time_commands(command_buffer);
}

void VulkanRenderer::generate_mipmaps_internal(VkImage image, VkFormat format, int32_t tex_width, int32_t tex_height, uint32_t mip_levels) {
    // Check if image format supports linear blitting
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(physical_device_, format, &format_properties);
    
    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }
    
    VkCommandBuffer command_buffer = begin_single_time_commands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    
    int32_t mip_width = tex_width;
    int32_t mip_height = tex_height;
    
    for (uint32_t i = 1; i < mip_levels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                           0, nullptr, 0, nullptr, 1, &barrier);
        
        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mip_width, mip_height, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        
        vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                           0, nullptr, 0, nullptr, 1, &barrier);
        
        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }
    
    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                       0, nullptr, 0, nullptr, 1, &barrier);
    
    end_single_time_commands(command_buffer);
}

} // namespace ecscope::rendering