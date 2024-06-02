#pragma once

#include <vulkan/vulkan.h>

typedef VkFlags TmxFlags;

enum TmxMemoryProperty {
  TMX_MEMORY_PROPERTY_HOST_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  TMX_MEMORY_PROPERTY_DEVICE_LOCAL = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  TMX_MEMORY_PROPERTY_UNIFIED      = TMX_MEMORY_PROPERTY_HOST_VISIBLE | TMX_MEMORY_PROPERTY_DEVICE_LOCAL,
};

enum TmxBufferCreateFlagBits {
  TMX_BUFFER_CREATE_MAPPED_BIT          = 0x00000001,
};
typedef TmxFlags TmxBufferCreateFlags;