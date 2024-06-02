#pragma once

#include <types.inl>

#include "../core/tmx.hpp"
#include "../vk/context.hpp"
#include "../vk/buffer.hpp"

#include <memory>

namespace tmx {

struct ResourceManager {
  public:
  ResourceManager(Context *vk_context) : vk_context{vk_context} {

  }

  ~ResourceManager(void) = default;
  
  template<typename T>
  [[nodiscard]]
  std::unique_ptr<DeviceBuffer<T>> create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    TmxMemoryProperty property,
    TmxBufferCreateFlags flags
  ) {
    std::unique_ptr<DeviceBuffer<T>> buffer =
    std::make_unique<DeviceBuffer<T>>(
      vk_context,
      size,
      usage,
      property
    );

    if(flags & TMX_BUFFER_CREATE_MAPPED_BIT) {
      buffer->map_memory();
    }

    return buffer;
  }

  private:
  Context *vk_context;
};

}
