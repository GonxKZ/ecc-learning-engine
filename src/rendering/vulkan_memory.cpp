/**
 * @file vulkan_memory.cpp
 * @brief Vulkan Memory Management Implementation with VMA Support
 * 
 * Efficient memory management for Vulkan resources using Vulkan Memory Allocator
 * when available, with fallback to manual memory management.
 * 
 * @author ECScope Development Team
 * @version 1.0.0
 */

#include "ecscope/rendering/vulkan_backend.hpp"
#include <iostream>
#include <algorithm>

#ifdef ECSCOPE_HAS_VMA
#include "vk_mem_alloc.h"
#endif

namespace ecscope::rendering {

// =============================================================================
// MEMORY MANAGEMENT HELPERS
// =============================================================================

uint32_t VulkanRenderer::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);
    
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && 
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

bool VulkanRenderer::allocate_buffer_memory(VkBuffer buffer, VkMemoryPropertyFlags properties, VkDeviceMemory& memory) {
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device_, buffer, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);
    
    VkResult result = vkAllocateMemory(device_, &alloc_info, nullptr, &memory);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate buffer memory!" << std::endl;
        return false;
    }
    
    vkBindBufferMemory(device_, buffer, memory, 0);
    return true;
}

bool VulkanRenderer::allocate_image_memory(VkImage image, VkMemoryPropertyFlags properties, VkDeviceMemory& memory) {
    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device_, image, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);
    
    VkResult result = vkAllocateMemory(device_, &alloc_info, nullptr, &memory);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate image memory!" << std::endl;
        return false;
    }
    
    vkBindImageMemory(device_, image, memory, 0);
    return true;
}

// =============================================================================
// BUFFER RESOURCE MANAGEMENT
// =============================================================================

BufferHandle VulkanRenderer::create_buffer(const BufferDesc& desc, const void* initial_data) {
    if (desc.size == 0) {
        std::cerr << "Cannot create buffer with size 0" << std::endl;
        return BufferHandle{};
    }
    
    uint64_t handle_id = next_resource_id_.fetch_add(1);
    VulkanBuffer vk_buffer;
    vk_buffer.size = desc.size;
    vk_buffer.usage = buffer_usage_to_vulkan(desc.usage);
    vk_buffer.debug_name = desc.debug_name;
    
    // Determine memory properties based on usage
    VkMemoryPropertyFlags memory_properties = 0;
    if (desc.gpu_only) {
        memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        vk_buffer.memory_properties = memory_properties;
    } else {
        switch (desc.usage) {
            case BufferUsage::Static:
                memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                // For static buffers with initial data, we'll use a staging buffer
                if (initial_data) {
                    vk_buffer.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                }
                break;
            case BufferUsage::Dynamic:
            case BufferUsage::Streaming:
                memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            case BufferUsage::Staging:
                memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                vk_buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                break;
        }
        vk_buffer.memory_properties = memory_properties;
    }
    
    // Create the buffer
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = desc.size;
    buffer_info.usage = vk_buffer.usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkResult result = vkCreateBuffer(device_, &buffer_info, nullptr, &vk_buffer.buffer);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create buffer!" << std::endl;
        return BufferHandle{};
    }
    
    // Allocate memory
    if (!allocate_buffer_memory(vk_buffer.buffer, memory_properties, vk_buffer.memory)) {
        vkDestroyBuffer(device_, vk_buffer.buffer, nullptr);
        return BufferHandle{};
    }
    
    // Map memory if it's host visible
    if (memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        result = vkMapMemory(device_, vk_buffer.memory, 0, desc.size, 0, &vk_buffer.mapped_data);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to map buffer memory!" << std::endl;
            vkFreeMemory(device_, vk_buffer.memory, nullptr);
            vkDestroyBuffer(device_, vk_buffer.buffer, nullptr);
            return BufferHandle{};
        }
    }
    
    // Set debug name if validation layers are enabled
    if (enable_validation_layers_ && !desc.debug_name.empty()) {
        VkDebugUtilsObjectNameInfoEXT name_info{};
        name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        name_info.objectType = VK_OBJECT_TYPE_BUFFER;
        name_info.objectHandle = reinterpret_cast<uint64_t>(vk_buffer.buffer);
        name_info.pObjectName = desc.debug_name.c_str();
        
        auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device_, "vkSetDebugUtilsObjectNameEXT");
        if (func != nullptr) {
            func(device_, &name_info);
        }
    }
    
    // Copy initial data if provided
    if (initial_data) {
        if (vk_buffer.mapped_data) {
            // Direct copy for host-visible buffers
            std::memcpy(vk_buffer.mapped_data, initial_data, desc.size);
        } else {
            // Use staging buffer for device-local buffers
            BufferDesc staging_desc;
            staging_desc.size = desc.size;
            staging_desc.usage = BufferUsage::Staging;
            staging_desc.debug_name = desc.debug_name + "_staging";
            
            BufferHandle staging_buffer = create_buffer(staging_desc, initial_data);
            if (!staging_buffer.is_valid()) {
                std::cerr << "Failed to create staging buffer!" << std::endl;
                if (vk_buffer.mapped_data) {
                    vkUnmapMemory(device_, vk_buffer.memory);
                }
                vkFreeMemory(device_, vk_buffer.memory, nullptr);
                vkDestroyBuffer(device_, vk_buffer.buffer, nullptr);
                return BufferHandle{};
            }
            
            // Copy from staging buffer to device buffer
            copy_buffer(staging_buffer, BufferHandle{handle_id}, desc.size);
            destroy_buffer(staging_buffer);
        }
    }
    
    // Store the buffer
    {
        std::lock_guard<std::mutex> lock(resource_mutex_);
        buffers_[handle_id] = std::move(vk_buffer);
    }
    
    return BufferHandle{handle_id};
}

void VulkanRenderer::destroy_buffer(BufferHandle handle) {
    if (!handle.is_valid()) return;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = buffers_.find(handle.id());
    if (it == buffers_.end()) return;
    
    VulkanBuffer& vk_buffer = it->second;
    
    // Unmap memory if it was mapped
    if (vk_buffer.mapped_data) {
        vkUnmapMemory(device_, vk_buffer.memory);
        vk_buffer.mapped_data = nullptr;
    }
    
    // Destroy Vulkan resources
    if (vk_buffer.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, vk_buffer.buffer, nullptr);
    }
    if (vk_buffer.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device_, vk_buffer.memory, nullptr);
    }
    
    buffers_.erase(it);
}

void VulkanRenderer::update_buffer(BufferHandle handle, size_t offset, size_t size, const void* data) {
    if (!handle.is_valid() || !data) return;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto it = buffers_.find(handle.id());
    if (it == buffers_.end()) return;
    
    VulkanBuffer& vk_buffer = it->second;
    
    if (offset + size > vk_buffer.size) {
        std::cerr << "Buffer update out of bounds!" << std::endl;
        return;
    }
    
    if (vk_buffer.mapped_data) {
        // Direct copy for host-visible buffers
        std::memcpy(static_cast<char*>(vk_buffer.mapped_data) + offset, data, size);
        
        // Flush if memory is not coherent
        if (!(vk_buffer.memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange memory_range{};
            memory_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memory_range.memory = vk_buffer.memory;
            memory_range.offset = offset;
            memory_range.size = size;
            vkFlushMappedMemoryRanges(device_, 1, &memory_range);
        }
    } else {
        // Use staging buffer for device-local buffers
        BufferDesc staging_desc;
        staging_desc.size = size;
        staging_desc.usage = BufferUsage::Staging;
        staging_desc.debug_name = vk_buffer.debug_name + "_update_staging";
        
        BufferHandle staging_buffer = create_buffer(staging_desc, data);
        if (!staging_buffer.is_valid()) {
            std::cerr << "Failed to create staging buffer for update!" << std::endl;
            return;
        }
        
        // Copy from staging buffer to device buffer with offset
        copy_buffer_region(staging_buffer, handle, size, 0, offset);
        destroy_buffer(staging_buffer);
    }
}

void VulkanRenderer::copy_buffer(BufferHandle src_buffer, BufferHandle dst_buffer, VkDeviceSize size) {
    VkCommandBuffer command_buffer = begin_single_time_commands();
    
    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto src_it = buffers_.find(src_buffer.id());
    auto dst_it = buffers_.find(dst_buffer.id());
    
    if (src_it != buffers_.end() && dst_it != buffers_.end()) {
        vkCmdCopyBuffer(command_buffer, src_it->second.buffer, dst_it->second.buffer, 1, &copy_region);
    }
    
    end_single_time_commands(command_buffer);
}

void VulkanRenderer::copy_buffer_region(BufferHandle src_buffer, BufferHandle dst_buffer, 
                                       VkDeviceSize size, VkDeviceSize src_offset, VkDeviceSize dst_offset) {
    VkCommandBuffer command_buffer = begin_single_time_commands();
    
    VkBufferCopy copy_region{};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    
    std::lock_guard<std::mutex> lock(resource_mutex_);
    auto src_it = buffers_.find(src_buffer.id());
    auto dst_it = buffers_.find(dst_buffer.id());
    
    if (src_it != buffers_.end() && dst_it != buffers_.end()) {
        vkCmdCopyBuffer(command_buffer, src_it->second.buffer, dst_it->second.buffer, 1, &copy_region);
    }
    
    end_single_time_commands(command_buffer);
}

VkBufferUsageFlags VulkanRenderer::buffer_usage_to_vulkan(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case BufferUsage::Dynamic:
        case BufferUsage::Streaming:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | 
                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case BufferUsage::Staging:
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        default:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
}

VkCommandBuffer VulkanRenderer::begin_single_time_commands() {
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = transfer_command_pool_;
    alloc_info.commandBufferCount = 1;
    
    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(command_buffer, &begin_info);
    
    return command_buffer;
}

void VulkanRenderer::end_single_time_commands(VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    
    vkQueueSubmit(transfer_queue_, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(transfer_queue_);
    
    vkFreeCommandBuffers(device_, transfer_command_pool_, 1, &command_buffer);
}

} // namespace ecscope::rendering