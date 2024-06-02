#pragma once

#include "../core/utils.hpp"
#include "context.hpp"

#include <vulkan/vulkan.h>

#include <cassert>
#include <cstring>
#include <memory>

namespace tmx {

  template<typename T>
  struct DeviceBuffer {
    public:
    DeviceBuffer(
      Context* vk_context,
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties
    ) : vk_context{vk_context}, buffer_size{get_size(size, vk_context->get_non_coherent_atom_size())} {
      
      VkBufferCreateInfo buffer_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = buffer_size,
        .usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
      };
      VK_CHECK(vkCreateBuffer(vk_context->get_device(), &buffer_create_info, nullptr, &buffer));

      VkMemoryRequirements memory_requirements{};
      vkGetBufferMemoryRequirements(vk_context->get_device(), buffer, &memory_requirements);

      VkMemoryAllocateFlagsInfo memory_allocate_flags_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
      };

      VkMemoryAllocateInfo memory_allocate_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &memory_allocate_flags_info,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties),
      };

      VK_CHECK(vkAllocateMemory(vk_context->get_device(), &memory_allocate_info, nullptr, &memory));
      VK_CHECK(vkBindBufferMemory(vk_context->get_device(), buffer, memory, 0));

      VkBufferDeviceAddressInfo bdai{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer};
      buffer_device_address = (T*)vkGetBufferDeviceAddress(vk_context->get_device(), &bdai);
    }

    ~DeviceBuffer() {
      std::cout << "Freeing buffer memory." << std::endl;
      vkFreeMemory(vk_context->get_device(), memory, nullptr);
      std::cout << "Destroying buffer." << std::endl;
      vkDestroyBuffer(vk_context->get_device(), buffer, nullptr);
    }

        
    [[nodiscard]] inline
    VkBuffer vk_buffer(void) const { return buffer; }
        
    [[nodiscard]] inline
    T* device_address(void) const { return buffer_device_address; }
        
    [[nodiscard]] inline
    T* host_address(void) const { return mapped_address; }

    inline void map_memory(void) {
      VK_CHECK(vkMapMemory(vk_context->get_device(), memory, 0, buffer_size, 0, (void**)&mapped_address));
    }

    inline void unmap_memory(void) {
      vkUnmapMemory(vk_context->get_device(), memory);
      mapped_address = nullptr;
    }

    private:
        
    [[nodiscard]] inline
    VkDeviceSize get_size(VkDeviceSize size, VkDeviceSize non_coherent_atom_size) {
      return (size+non_coherent_atom_size-1) & ~(non_coherent_atom_size-1);
    }

    inline u32 find_memory_type(u32 type_bits, VkMemoryPropertyFlags properties) {
      VkPhysicalDeviceMemoryProperties memory_properties;
      vkGetPhysicalDeviceMemoryProperties(vk_context->get_physical_device(), &memory_properties);
      for (u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_bits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
          return i;
        }
      }
      throw std::runtime_error("Could not find required buffer memory type!");
    }

    Context* vk_context;
    VkBuffer buffer{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    T* buffer_device_address{};
    const VkDeviceSize buffer_size;
    T* mapped_address{nullptr};
  };
}
