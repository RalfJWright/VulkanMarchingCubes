#pragma once

#include "core/components.hpp"
#include "core/event_bus.hpp"

#include "input.hpp"
#include "camera.hpp"

#include "pipelines/compute/compute_pipeline.hpp"
#include "pipelines/graphics/graphics_pipeline.hpp"

#include "systems/resource_manager.hpp"
#include "systems/terrain_system.hpp"

#include <array>
#include <cstring>
#include <cmath>
#include <vector>
#include <chrono>
#include <iostream>
#include <utility>
#include <filesystem>

#include <unistd.h>

namespace tmx {
  
  struct Application {

  void run(void) {
    f64 dt{0.0};
    f32 iTime{0.0}; // In seconds
    u64 frame_number{0};

    EventBus event_bus{};

    Window window{1280, 720, "Vulkan"};
    Context vk_context{window};
    
    GraphicsPipeline common_pipeline {
      {
        "voxel",
        sizeof(GraphicsPush),
        vk_context.get_device(),
        vk_context.get_swapchain_image_format(),
        vk_context.get_depth_format(),
        VK_FALSE,
      }
    };

    bool march{true};

    std::unique_ptr<ResourceManager> resource_manager =
      std::make_unique<ResourceManager>(&vk_context);
    
    Input input{window.get_glfw_window(), &event_bus, &dt};
    Camera camera {TransformComponent{float3{0.0}, float3{0.0}, float3{1.0}}, 0.01, 100000.0, &window, &input, &event_bus, resource_manager.get(), &dt};
    TerrainManager terrain_manager{&vk_context, &event_bus, &common_pipeline, resource_manager.get()};

    std::cout << "IsosurfaceGenerationEvent" << std::endl;
	  event_bus.notify<IsosurfaceGenerationEvent>(
      IsosurfaceGenerationEvent{.progress = int3{0, 0, 0}}
    );

    std::cout << "IsosurfaceMeshingEvent" << std::endl;
	  event_bus.notify<IsosurfaceMeshingEvent>(
      IsosurfaceMeshingEvent{.progress = int3{0, 0, 0}}
    );

    camera.update_view();

    auto initial = std::chrono::steady_clock::now();
    
    std::cout << "Initialization Successful!" << std::endl;

    while(!window.should_close() && !glfwGetKey(window.get_glfw_window(), GLFW_KEY_ESCAPE)) {

    frame_number++;
    auto final = std::chrono::steady_clock::now();
    dt = std::chrono::duration<f64, std::chrono::milliseconds::period>(final - initial).count();
    initial = final;
    iTime += dt/1000.0;
    //std::cout << "FRAMETIME: " << dt << " ms" << std::endl;

    input.process_events();
    camera.process_input();

    VkCommandBuffer command_buffer = vk_context.rendering_begin_command_buffers();
    vk_context.cmd_begin_rendering(command_buffer);
    const u32 frame = vk_context.get_current_frame();
    
    camera.update_self_data(frame, 1.35, window.get_aspect_ratio(), window.get_dim_f32());
	
    VkDrawIndirectCommand *draws = terrain_manager.get_indirect_cmds_host_address();

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, common_pipeline.get_pipeline());
      
    GraphicsPush push {
      .pVertices = SHADER_CAST(terrain_manager.get_terrain_vertex_buffer_address()),
      .pMatrices = SHADER_CAST(camera.matrices_device_address(frame)),
    };
  
    vkCmdPushConstants(
      command_buffer,
      common_pipeline.get_pipeline_layout(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      0,
      common_pipeline.get_push_constant_size(),
      &push
    );

    u32 cnt = terrain_manager.get_chunk_render_count();
    if(cnt > 0) std::cout << "COUNT " << cnt << std::endl;
    
    vkCmdDrawIndirect(
      command_buffer,
      terrain_manager.get_indirect_cmds_buffer(),
      0,
      terrain_manager.get_chunk_render_count(),
      sizeof(VkDrawIndirectCommand)
    );

    vk_context.cmd_end_rendering(command_buffer);
    vk_context.end_command_buffer(command_buffer);
    vk_context.queue_submit_and_present(command_buffer);

    }

    vkDeviceWaitIdle(vk_context.get_device());
  }

  }; // Application
}