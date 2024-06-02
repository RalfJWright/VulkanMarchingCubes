#pragma once

#include "../core/utils.hpp"
#include "../window.hpp"
#include "../pipelines/graphics/graphics_pipeline.hpp"

#include <set>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

#define DESIRED_PHYSICAL_DEVICE_TYPE VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU

namespace tmx {

  struct QueueFamilyIndices{
    std::optional<u32> graphics_family;
    std::optional<u32> compute_family;
    std::optional<u32> present_family;

        
    [[nodiscard]] inline
    bool is_complete(void) const {
      return graphics_family.has_value() && compute_family.has_value() && present_family.has_value();
    }
  };

  struct SwapchainSupportDetails{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    std::vector<VkPresentModeKHR> present_modes;
  };

  struct TmxCopyBufferInfo{
    VkBuffer src;
    VkBuffer dst;
    VkDeviceSize srcOffset;
    VkDeviceSize dstOffset;
    VkDeviceSize size;
  };

  struct TmxCopyBufferToImageInfo{
    VkBuffer buffer;
    VkImage image;
    VkImageAspectFlags aspect_mask;
    u32 width;
    u32 height;
  };

  struct TmxImageLayoutTransitionInfo{
    VkImage image;
    VkFormat format;
    VkImageAspectFlags aspectMask;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    VkAccessFlagBits2 srcAccessMask;
    VkAccessFlagBits2 dstAccessMask;
    VkPipelineStageFlagBits2 srcStage;
    VkPipelineStageFlagBits2 dstStage;
  };

  struct TmxSubmitInfo{
    VkQueue queue;
    u32 waitSemaphoreInfoCount;
    const VkSemaphoreSubmitInfo *pWaitSemaphoreInfos;
    u32 signalSemaphoreInfoCount;
    const VkSemaphoreSubmitInfo *pSignalSemaphoreInfos;
  };

  struct Context {
    public:
    Context(Window &window) : window{window} {
      create_vulkan_instance();
      setup_debug_messenger();
      create_surface();
      choose_physical_device();
      create_logical_device();
      create_swapchain();
      create_swapchain_image_views();
      create_command_pool();
      create_command_buffers();
      create_syncronization_objects();
      create_depth_buffer();
    }

    ~Context(void) {
      std::cout << "Destroying Vulkan Objects!" << std::endl;

      if (enable_validation_layers) {
        destroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
      }

      vkFreeCommandBuffers(device, command_pool, command_buffers.size(), command_buffers.data());
      vkDestroyCommandPool(device, command_pool, nullptr);
      
      for(auto &vk_image_view : swapchain_image_views) {
        vkDestroyImageView(device, vk_image_view, nullptr);
      }

      for(auto &vk_device_memory : depth_image_memories) {
        vkFreeMemory(device, vk_device_memory, nullptr);
      }

      for(auto &vk_image_view : depth_image_views) {
        vkDestroyImageView(device, vk_image_view, nullptr);
      }

      for(auto &vk_image : depth_images) {
        vkDestroyImage(device, vk_image, nullptr);
      }

      for(auto &fence : in_flight_fences) {
        vkDestroyFence(device, fence, nullptr);
      }

      for(auto &semaphore : render_finished_semaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
      }

      for(auto &semaphore : image_available_semaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
      }

      vkDestroySwapchainKHR(device, swapchain, nullptr);
      vkDestroyDevice(device, nullptr);
      vkDestroySurfaceKHR(instance, surface, nullptr);
      vkDestroyInstance(instance, nullptr);

      std::cout << "Destroyed all Vulkan objects!" << std::endl;
    }

    [[nodiscard]] inline
    u32 get_current_frame(void) { return current_frame; }

    [[nodiscard]] inline
    VkPhysicalDevice &get_physical_device(void) { return physical_device; }

    [[nodiscard]] inline
    VkDevice &get_device(void) { return device; }

    [[nodiscard]] inline
    VkFormat &get_swapchain_image_format(void) { return swapchain_image_format; }

    [[nodiscard]] inline
    VkFormat &get_depth_format(void) { return depth_image_format; }

    [[nodiscard]] inline
    VkQueue get_graphics_queue(void) { return graphics_queue; }

    [[nodiscard]] inline
    VkQueue get_compute_queue(void) { return compute_queue; }

    [[nodiscard]] inline
    VkDeviceSize get_non_coherent_atom_size(void) { return non_coherent_atom_size; }


   //********************************************************//
   //********************************************************//
   //********************************************************//


    template<u32 command_buffer_count>
    [[nodiscard]] inline
    VkCommandBuffer begin_command_buffers(void) {
      VkCommandBufferAllocateInfo command_buffer_alllocation_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = command_buffer_count,
      };

      VkCommandBuffer command_buffer;
      VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_alllocation_info, &command_buffer));

      VkCommandBufferBeginInfo command_buffer_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      };

      VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

      return command_buffer;
    }
    
    inline void end_command_buffer(VkCommandBuffer command_buffer) {
      VK_CHECK(vkEndCommandBuffer(command_buffer));
    }

    inline void queue_submit(VkCommandBuffer command_buffer, const TmxSubmitInfo &info, VkFence fence = VK_NULL_HANDLE) {
      const VkCommandBufferSubmitInfo cmd_buf_submit_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = command_buffer,
        .deviceMask = 0,
      };

      const VkSubmitInfo2 submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0,
        .waitSemaphoreInfoCount = info.waitSemaphoreInfoCount,
        .pWaitSemaphoreInfos = info.pWaitSemaphoreInfos,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_buf_submit_info,
        .signalSemaphoreInfoCount = info.signalSemaphoreInfoCount,
        .pSignalSemaphoreInfos = info.pSignalSemaphoreInfos,
      };

      VK_CHECK(vkQueueSubmit2(info.queue, 1, &submit_info, fence));
    }

    template<u32 command_buffer_count>
    inline void free_command_buffers(const VkCommandBuffer *pcommand_buffers) {
      vkFreeCommandBuffers(device, command_pool, command_buffer_count, pcommand_buffers);
    }

    inline void queue_wait_idle(VkQueue queue) {
      VK_CHECK(vkQueueWaitIdle(queue));
    }

    inline void device_wait_idle(void) {
      std::cout << "\n" << "vkDeviceWaitIdle()" << "\n" << std::endl;
      VK_CHECK(vkDeviceWaitIdle(device));
    }

    void transition_image_layout(const TmxImageLayoutTransitionInfo &info) {

      VkCommandBuffer command_buffer = begin_command_buffers<1>();

      bool stencil = (info.format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (info.format == VK_FORMAT_D24_UNORM_S8_UINT);

      const VkImageMemoryBarrier2 barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = info.srcStage,
        .srcAccessMask = info.srcAccessMask,
        .dstStageMask = info.dstStage,
        .dstAccessMask = info.dstAccessMask,
        .oldLayout = info.oldLayout,
        .newLayout = info.newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = info.image,
        .subresourceRange =
          {
            .aspectMask = stencil ? info.aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT : info.aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
          },
      };

      const VkDependencyInfo dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
      };

      vkCmdPipelineBarrier2(command_buffer, &dependency_info);

      end_command_buffer(command_buffer);
      queue_submit(
        command_buffer,
        TmxSubmitInfo{
          .queue = graphics_queue,
          .waitSemaphoreInfoCount = 0,
          .pWaitSemaphoreInfos = nullptr,
          .signalSemaphoreInfoCount = 0,
          .pSignalSemaphoreInfos = nullptr,
        }
      );
      queue_wait_idle(graphics_queue);
      vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    }

    void copy_buffer(const TmxCopyBufferInfo &info) {
      VkCommandBuffer command_buffer = begin_command_buffers<1>();

      const VkBufferCopy2 region{
        .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
        .pNext = nullptr,
        .srcOffset = info.srcOffset,
        .dstOffset = info.dstOffset,
        .size = info.size,
      };

      const VkCopyBufferInfo2 copy_info{
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
        .pNext = nullptr,
        .srcBuffer = info.src,
        .dstBuffer = info.dst,
        .regionCount = 1,
        .pRegions = &region,
      };

      vkCmdCopyBuffer2(command_buffer, &copy_info);

      end_command_buffer(command_buffer);
      queue_submit(
        command_buffer,
        TmxSubmitInfo{
          .queue = graphics_queue,
          .waitSemaphoreInfoCount = 0,
          .pWaitSemaphoreInfos = nullptr,
          .signalSemaphoreInfoCount = 0,
          .pSignalSemaphoreInfos = nullptr,
        }
      );
      queue_wait_idle(graphics_queue);
      vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    }

    void copy_buffer_to_image(const TmxCopyBufferToImageInfo &info) {
      VkCommandBuffer command_buffer = begin_command_buffers<1>();

      const VkBufferImageCopy2 region{
        .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
        .pNext = nullptr,
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
          {
            .aspectMask = info.aspect_mask,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
          },
        .imageOffset = {0, 0, 0},
        .imageExtent = {info.width, info.height, 1},
      };

      const VkCopyBufferToImageInfo2 copy_info{
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
        .pNext = nullptr,
        .srcBuffer = info.buffer,
        .dstImage = info.image,
        .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .regionCount = 1,
        .pRegions = &region,
      };

      vkCmdCopyBufferToImage2(command_buffer, &copy_info);

      end_command_buffer(command_buffer);
      queue_submit(
        command_buffer,
        TmxSubmitInfo{
          .queue = graphics_queue,
          .waitSemaphoreInfoCount = 0,
          .pWaitSemaphoreInfos = nullptr,
          .signalSemaphoreInfoCount = 0,
          .pSignalSemaphoreInfos = nullptr,
        }
      );
      queue_wait_idle(graphics_queue);
      vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
    }

    //***************************************************//

    VkCommandBuffer rendering_begin_command_buffers(void) {
      //  Block host and wait for 1 second
      VK_CHECK(vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, 1000000000));

      VK_CHECK(vkResetFences(device, 1, &in_flight_fences[current_frame]));
      
      // Aquire the next swapchain image, timeout => 1 second
      #pragma diag_suppress 20
      do {
        VkResult result =
          vkAcquireNextImageKHR(
            device,
            swapchain,
            1000000000,
            image_available_semaphores[current_frame],
            VK_NULL_HANDLE, 
            &image_index
          );
        std::cout << "AQUIRE RESULT IS " << string_VkResult(result) << std::endl;
        if ( (result & (VK_SUCCESS | VK_SUBOPTIMAL_KHR) == 0) ) {
          std::string color = "\n\033[1;31m";
          std::string colour = "\033[0m\n";
          std::string str = "\n[[FATAL ERROR]]\n";
          throw std::runtime_error(
            color + str + std::string{string_VkResult(result)} + colour
          );
        }
      } while(0);
      #pragma diag_default 20
      
      VK_CHECK(vkResetCommandBuffer(command_buffers[current_frame], 0));

      VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr,
      };

      VK_CHECK(vkBeginCommandBuffer(command_buffers[current_frame], &begin_info));

      return command_buffers[current_frame];
    }

    void cmd_begin_rendering(VkCommandBuffer &command_buffer) {

      // For color attachment
      const VkImageMemoryBarrier2 color_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_images[image_index],
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        }
      };
      
      // For depth attachment
      const VkImageMemoryBarrier2 depth_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = depth_images[image_index],
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        }
      };

      const VkImageMemoryBarrier2 barriers[] = {
        color_barrier,
        depth_barrier,
      };

      const VkDependencyInfo dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers = barriers,
      };

      vkCmdPipelineBarrier2(command_buffer, &dependency_info);

      VkClearValue clear_value {
        .color = {{0.025, 0.025, 0.025, 1.0}}
      };

      const VkRenderingAttachmentInfo color_attachment_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = swapchain_image_views[image_index],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_value
      };

      VkClearValue ds_clear_value {
        .depthStencil = {1.0, 0}
      };

      const VkRenderingAttachmentInfo depth_attachment_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = depth_image_views[image_index],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = ds_clear_value
      };

      VkRect2D render_area {
        .offset = {0, 0},
        .extent = swapchain_extent
      };

      const VkRenderingInfo rendering_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .renderArea = render_area,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = &depth_attachment_info,
        .pStencilAttachment = nullptr
      };

      vkCmdBeginRendering(command_buffer, &rendering_info);

      VkViewport viewport {
        .x = 0.0,
        .y = static_cast<f32>(swapchain_extent.height),
        .width = static_cast<f32>(swapchain_extent.width),
        .height = -static_cast<f32>(swapchain_extent.height),
        .minDepth = 0.0,
        .maxDepth = 1.0,
      };

      VkRect2D scissor {
        .offset = {0, 0},
        .extent = swapchain_extent,
      };

      vkCmdSetViewport(command_buffer, 0, 1, &viewport);
      vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    }

    void cmd_end_rendering(VkCommandBuffer &command_buffer) {
      vkCmdEndRendering(command_buffer);

      // For color attachment
      const VkImageMemoryBarrier2 color_image_memory_barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = swapchain_images[image_index],
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        }
      };

      // For depth attachment
      const VkImageMemoryBarrier2 depth_image_memory_barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = depth_images[image_index],
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        }
      };

      const VkImageMemoryBarrier2 barriers[] = {
        color_image_memory_barrier,
        depth_image_memory_barrier
      };

      const VkDependencyInfo dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers = barriers,
      };

      vkCmdPipelineBarrier2(command_buffer, &dependency_info);

    }

    inline void rendering_end_command_buffer(VkCommandBuffer &command_buffer) {
      VK_CHECK(vkEndCommandBuffer(command_buffer));
    }

    void queue_submit_and_present(VkCommandBuffer &command_buffer) {

      VkSemaphoreSubmitInfo wait{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = image_available_semaphores[current_frame],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .deviceIndex = 0,
      };

      VkSemaphoreSubmitInfo signal{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = render_finished_semaphores[current_frame],
        .stageMask = VK_PIPELINE_STAGE_2_NONE,
        .deviceIndex = 0,
      };

      queue_submit(
        command_buffer,
        TmxSubmitInfo{
          .queue = graphics_queue,
          .waitSemaphoreInfoCount = 1,
          .pWaitSemaphoreInfos = &wait,
          .signalSemaphoreInfoCount = 1,
          .pSignalSemaphoreInfos = &signal,
        },
        in_flight_fences[current_frame]
      );

      VkSwapchainKHR swapchains[] = {swapchain};

      VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished_semaphores[current_frame],
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
      };

      #pragma diag_suppress 20
      do {
        VkResult result = vkQueuePresentKHR(present_queue, &present_info);
        if ( (result & (VK_SUCCESS | VK_SUBOPTIMAL_KHR) == 0) ) {
          std::string color = "\n\033[1;31m";
          std::string colour = "\033[0m\n";
          std::string str = "\n[[FATAL ERROR]]\n";
          throw std::runtime_error(
            color + str + std::string{string_VkResult(result)} + colour
          );
        }
      } while(0);
      #pragma diag_default 20

      current_frame = (current_frame + 1) % RENDERER_FRAMES_IN_FLIGHT;
    }

    u32 current_frame{0};
    u32 subgroup_size{0};

    private:
    const bool enable_validation_layers = true;

    const std::vector<const char *> validation_layers{
      "VK_LAYER_KHRONOS_validation"
    };

    std::vector<const char *> get_required_extensions(void) {
      u32 glfw_extension_count = 0;
      const char **glfw_extensions;
      glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

      std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

      if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
      }

      return extensions;
    }

    bool check_lalidation_layer_support(void) {
      u32 layer_count;
      vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

      std::vector<VkLayerProperties> available_layers(layer_count);
      vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

      for (const char *layer_name : validation_layers) {
        bool layer_found = false;

        for (const auto &layer_properties : available_layers) {
          if (strcmp(layer_name, layer_properties.layerName) == 0) {
            layer_found = true;
            break;
          }
        }

        if (!layer_found) {
          return false;
        }
      }

      return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
      std::cout << "\n\n" << pCallbackData->pMessage << "\n" << std::endl;
      return VK_FALSE;
    }

    void create_vulkan_instance(void) {

      if (enable_validation_layers && !check_lalidation_layer_support()) {
        throw std::runtime_error("The Vulkan Validation layers were requested, but are unavailable!");
      }

      VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Rendering",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "Rendering Engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
      };

      auto extensions = get_required_extensions();
      VkInstanceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = static_cast<u32>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
      };

      VkDebugUtilsMessengerCreateInfoEXT debug_create_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
      };
      if (enable_validation_layers) {
        create_info.ppEnabledLayerNames = validation_layers.data();
        create_info.enabledLayerCount = static_cast<u32>(validation_layers.size());
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
      } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
      }

      VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance));
    }

    VkResult createdebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pcreate_info, const VkAllocationCallbacks* pallocator, VkDebugUtilsMessengerEXT* pdebug_messenger) {
      auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
      if (func != nullptr) {
        return func(instance, pcreate_info, pallocator, pdebug_messenger);
      } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
      }
    }

    void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* pallocator) {
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
      if (func != nullptr) {
        func(instance, debug_messenger, pallocator);
      }
    }

    void setup_debug_messenger(void) {
      if (!enable_validation_layers) return;

      VkDebugUtilsMessengerCreateInfoEXT create_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
      };

      VK_CHECK(createdebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger));
    }

    QueueFamilyIndices get_queue_family_indices(VkPhysicalDevice device) {
      QueueFamilyIndices queue_family_indices{};
      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(device, &props);

      VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
        .pNext = nullptr,
      };

      VkPhysicalDeviceFeatures2 feats{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &descriptor_indexing,
      };
      vkGetPhysicalDeviceFeatures2(device, &feats);

      bool bindless_images_supported = descriptor_indexing.descriptorBindingPartiallyBound && descriptor_indexing.runtimeDescriptorArray;

      bool compatible_device_type = props.deviceType == DESIRED_PHYSICAL_DEVICE_TYPE;
      
      if(compatible_device_type && bindless_images_supported) {
        u32 queue_family_count{0};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        int i = 0;

        for(const auto &queue_family : queue_families) {
          if(queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
              queue_family_indices.graphics_family = i;
          }
          if(queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queue_family_indices.compute_family = i;
          }

          VkBool32 present_support{false};
          VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support));
          if(present_support) {
            queue_family_indices.present_family = i;
          }

          if(queue_family_indices.is_complete()) {
            break;
          }
          i++;
        }
      }

      return queue_family_indices;
    }

    bool check_device_extension_support(VkPhysicalDevice device) {
      const std::vector<const char*> device_extensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      };

      u32 extension_count;
      VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr));

      std::vector<VkExtensionProperties> available_extensions(extension_count);
      VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data()));

      std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

      for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
      }

      return required_extensions.empty();
    }

    SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device) {
      SwapchainSupportDetails details;
      VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.surface_capabilities));

      u32 format_count;
      VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr));

      if (format_count != 0) {
        details.surface_formats.resize(format_count);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.surface_formats.data()));
      }

      u32 present_mode_count;
      VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr));

      if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data()));
      }
      
      return details;
    }

    bool device_is_compatible(VkPhysicalDevice device) {
      bool extensions_supported = check_device_extension_support(device);

      bool swapchain_compatible{false};
      if (extensions_supported) {
        SwapchainSupportDetails swapchain_support = query_swapchain_support(device);
        swapchain_compatible = !swapchain_support.surface_formats.empty() && !swapchain_support.present_modes.empty();
      } else {
        return false;
      }

      return get_queue_family_indices(device).is_complete() && extensions_supported && swapchain_compatible;
    }

    void choose_physical_device(void) {
      u32 device_count = 0;
      vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
      if (device_count == 0)
        throw std::runtime_error("No Vulkan drivers present...");
      
      std::vector<VkPhysicalDevice> devices(device_count);
      vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

      for (const auto &device : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        std::cout << "Found device: " << props.deviceName << std::endl;
        if (device_is_compatible(device)) {
          physical_device = device;
          break;
        }
      }

      if (physical_device == VK_NULL_HANDLE)
        throw std::runtime_error("Failed to select device...");

      VkPhysicalDeviceProperties props;
      vkGetPhysicalDeviceProperties(physical_device, &props);
      non_coherent_atom_size = props.limits.nonCoherentAtomSize;

      std::cout << "VkPhysicalDevice " << props.deviceName << "\n";

      VkPhysicalDeviceSubgroupSizeControlProperties subgroup_size_control_properties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES,
        .pNext = nullptr,
      };

      VkPhysicalDeviceSubgroupProperties subgroup_properties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES,
        .pNext = &subgroup_size_control_properties,
      };

      VkPhysicalDeviceProperties2 physical_device_properties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &subgroup_properties,
      };

      vkGetPhysicalDeviceProperties2(physical_device, &physical_device_properties);
      subgroup_size = subgroup_properties.subgroupSize;

      VkSubgroupFeatureFlags required_feature_flags{
        VK_SUBGROUP_FEATURE_BASIC_BIT |
        VK_SUBGROUP_FEATURE_ARITHMETIC_BIT |
        VK_SUBGROUP_FEATURE_BALLOT_BIT |
        VK_SUBGROUP_FEATURE_VOTE_BIT |
        VK_SUBGROUP_FEATURE_SHUFFLE_BIT |
        VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT
      };

      if(
          (subgroup_properties.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT) != VK_SHADER_STAGE_COMPUTE_BIT
          ||
          (subgroup_properties.supportedOperations & required_feature_flags) != required_feature_flags
          ||
          (subgroup_size_control_properties.requiredSubgroupSizeStages & VK_SHADER_STAGE_COMPUTE_BIT) != VK_SHADER_STAGE_COMPUTE_BIT
      ) {
        throw std::runtime_error("Device selection failed. Cause: Insufficient subgroupOp support...");
      }
    }

    void create_logical_device(void) {
      QueueFamilyIndices queue_family_indices = get_queue_family_indices(physical_device);

      std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
      std::set<u32> unique_queue_families{
        queue_family_indices.graphics_family.value(),
        queue_family_indices.compute_family.value(),
        queue_family_indices.present_family.value()
      };

      f32 queue_priorities[1] = {1.0};
      for(u32 queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .queueFamilyIndex = queue_family,
          .queueCount = 1,
          .pQueuePriorities = queue_priorities,
        };
        queue_create_infos.push_back(queue_create_info);
      }
      
      VkPhysicalDeviceVariablePointersFeatures variable_ptr_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTER_FEATURES,
        .pNext = nullptr,
      };

      VkPhysicalDeviceShaderFloat16Int8Features float16_int8_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES,
        .pNext = &variable_ptr_features,
      };

      VkPhysicalDevice8BitStorageFeatures bit8_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES,
        .pNext = &float16_int8_features,
      };

      VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext = &bit8_features,
      };
      
      VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &descriptor_indexing_features,
      };

      VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &buffer_device_address_features,
        .robustImageAccess = VK_FALSE,
        .inlineUniformBlock = VK_FALSE,
        .descriptorBindingInlineUniformBlockUpdateAfterBind = VK_FALSE,
        .pipelineCreationCacheControl = VK_FALSE,
        .privateData = VK_FALSE,
        .shaderDemoteToHelperInvocation = VK_FALSE,
        .shaderTerminateInvocation = VK_FALSE,
        .subgroupSizeControl = VK_TRUE,
        .computeFullSubgroups = VK_TRUE,
        .synchronization2 = VK_TRUE,
        .textureCompressionASTC_HDR = VK_FALSE,
        .shaderZeroInitializeWorkgroupMemory = VK_FALSE,
        .dynamicRendering = VK_TRUE,
        .shaderIntegerDotProduct = VK_FALSE,
        .maintenance4 = VK_TRUE,
      };

      VkPhysicalDeviceFeatures2 device_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features13,
      };
      vkGetPhysicalDeviceFeatures2(physical_device, &device_features);
      assert(bit8_features.storageBuffer8BitAccess);
      assert(features13.subgroupSizeControl);
      assert(features13.computeFullSubgroups);
      assert(features13.synchronization2);
      assert(features13.dynamicRendering);
      assert(features13.maintenance4);
      assert(buffer_device_address_features.bufferDeviceAddress);
      assert(descriptor_indexing_features.descriptorBindingPartiallyBound);
      assert(descriptor_indexing_features.descriptorBindingStorageBufferUpdateAfterBind);

      const std::vector<const char*> device_extensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      };

      VkDeviceCreateInfo device_create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &device_features,
        .flags = 0,
        .queueCreateInfoCount = static_cast<u32>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = enable_validation_layers ? static_cast<u32>(validation_layers.size()) : 0,
        .ppEnabledLayerNames = validation_layers.data(),
        .enabledExtensionCount = static_cast<u32>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = nullptr,
      };

      VK_CHECK(vkCreateDevice(physical_device, &device_create_info, nullptr, &device));

      vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
      vkGetDeviceQueue(device, queue_family_indices.compute_family.value(), 0, &compute_queue);
      vkGetDeviceQueue(device, queue_family_indices.present_family.value(), 0, &present_queue);
    }

    void create_surface(void) {
      VK_CHECK(glfwCreateWindowSurface(instance, window.get_glfw_window(), nullptr, &surface));
    }

    VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats) {
      for(const auto &available_format : available_formats) {
        if(
            (available_format.format == VK_FORMAT_B8G8R8A8_SRGB)
            &&
            (available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
          ) {
          return available_format;
        }
      }
      return available_formats[0];
    }

    VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes) {
      for(const auto& available_present_mode : available_present_modes) {
        if(available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
          return available_present_mode;
        }
      }

      return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window) {
      if(capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
        return capabilities.currentExtent;
      } else {
        i32 width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return VkExtent2D {
          std::clamp(static_cast<u32>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
          std::clamp(static_cast<u32>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
        };
      }
    }

    void create_swapchain(void) {
      SwapchainSupportDetails swapchain_support = query_swapchain_support(physical_device);
      
      VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.surface_formats);
      VkPresentModeKHR present_mode = choose_swapchain_present_mode(swapchain_support.present_modes);
      VkExtent2D extent = choose_swapchain_extent(swapchain_support.surface_capabilities, window.get_glfw_window());

      image_count = swapchain_support.surface_capabilities.minImageCount + 1;

      if(
          (swapchain_support.surface_capabilities.maxImageCount > 0)
          &&
          (image_count > swapchain_support.surface_capabilities.maxImageCount)
        ) {
        image_count = swapchain_support.surface_capabilities.maxImageCount;
      }

      VkSwapchainCreateInfoKHR swapchain_create_info{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = swapchain_support.surface_capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
      };
      
      QueueFamilyIndices indices = get_queue_family_indices(physical_device);
      u32 queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

      if (indices.graphics_family != indices.present_family) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
      } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = nullptr;
      }

      VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain));

      vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
      swapchain_images.resize(image_count);
      vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());
      std::cout << "Image count: " << image_count << std::endl;
      
      swapchain_image_format = surface_format.format;
      swapchain_extent = extent;
    }

    void create_swapchain_image_views(void) {
      swapchain_image_views.resize(swapchain_images.size());

      for(u32 i = 0; i < swapchain_images.size(); i++) {
        VkImageViewCreateInfo image_view_create_info{
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = swapchain_images[i],
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = swapchain_image_format,
          .components =
            {
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
              VK_COMPONENT_SWIZZLE_IDENTITY,
            },
          .subresourceRange = 
            {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
            },
        };

        VK_CHECK(vkCreateImageView(device, &image_view_create_info, nullptr, &swapchain_image_views[i]));
      }
    }

    void create_command_pool(void) {
      QueueFamilyIndices queue_family_indices = get_queue_family_indices(physical_device);

      VkCommandPoolCreateInfo command_pool_create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_indices.graphics_family.value(),
      };

      VK_CHECK(vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool));
    }

    void create_command_buffers(void) {
      command_buffers.resize(RENDERER_FRAMES_IN_FLIGHT);

      VkCommandBufferAllocateInfo command_buffer_allocate_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<u32>(command_buffers.size()),
      };

      VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, command_buffers.data()));
    }

    void create_syncronization_objects(void) {
      image_available_semaphores.resize(RENDERER_FRAMES_IN_FLIGHT);
      render_finished_semaphores.resize(RENDERER_FRAMES_IN_FLIGHT);
      in_flight_fences.resize(RENDERER_FRAMES_IN_FLIGHT);

      VkSemaphoreCreateInfo semaphore_create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
      };

      VkFenceCreateInfo fence_create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
      };

      for (int i = 0; i < RENDERER_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphores[i]));
        VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &in_flight_fences[i]));
      }
    }

    VkFormat choose_supported_format(
      const std::vector<VkFormat> &candidates,
      VkImageTiling tiling,
      VkFormatFeatureFlags features
    ) {
      for(VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
        
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
          return format;
        } else if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
          return format;
        }
      }

      throw std::runtime_error("Failed to find supported swapchain format!");
    }

    VkFormat choose_depth_format(void) {
      return
      choose_supported_format(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
      );
    }

    static u32 find_image_memory_type(VkPhysicalDevice physical_device, u32 type_bits, VkMemoryPropertyFlags properties) {
      VkPhysicalDeviceMemoryProperties memory_properties;
      vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
      for (u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_bits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
          return i;
        }
      }
      throw std::runtime_error("Could not find required image memory type!");
    }

    void create_depth_buffer(void) {
      depth_image_format = choose_depth_format();
      std::cout << "Depth Format: " << string_VkFormat(depth_image_format) << std::endl;

      depth_images.resize(swapchain_images.size());
      depth_image_memories.resize(swapchain_images.size());
      depth_image_views.resize(swapchain_image_views.size());

      for(size_t i = 0; i < depth_images.size(); i++) {

      VkImageCreateInfo image_create_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_image_format,
        .extent =
          {
            .width = swapchain_extent.width,
            .height = swapchain_extent.height,
            .depth = 1,
          },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      };

      VK_CHECK(vkCreateImage(device, &image_create_info, nullptr, &depth_images[i]));

      VkMemoryRequirements memory_requirements;
      vkGetImageMemoryRequirements(device, depth_images[i], &memory_requirements);

      VkMemoryAllocateInfo allocation_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_image_memory_type(physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
      };

      VK_CHECK(vkAllocateMemory(device, &allocation_info, nullptr, &depth_image_memories[i]));

      VK_CHECK(vkBindImageMemory(device, depth_images[i], depth_image_memories[i], 0));

      VkImageViewCreateInfo view_create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depth_images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depth_image_format,
        .components = 
          {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
          },
        .subresourceRange =
          {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
          },
      };

      VK_CHECK(vkCreateImageView(device, &view_create_info, nullptr, &depth_image_views[i]));
      

      transition_image_layout(
        TmxImageLayoutTransitionInfo {
          depth_images[i],
          depth_image_format,
          VK_IMAGE_ASPECT_DEPTH_BIT,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
          static_cast<VkAccessFlagBits>(0),
          VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
          VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
        }
      );

      }
    }
    
    Window &window;
    VkInstance instance;
    VkSurfaceKHR surface;

    VkDebugUtilsMessengerEXT debug_messenger;

    VkPhysicalDevice physical_device{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue present_queue;
    VkDeviceSize non_coherent_atom_size;

    u32 image_index;
    u32 image_count;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    VkFormat depth_image_format;
    std::vector<VkDeviceMemory> depth_image_memories;
    std::vector<VkImage> depth_images;
    std::vector<VkImageView> depth_image_views;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
  };
};
