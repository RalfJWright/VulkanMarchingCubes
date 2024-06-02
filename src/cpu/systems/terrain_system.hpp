#pragma once

#include <push.inl>

#include "../core/event_bus.hpp"
#include "../core/events.hpp"
#include "../vk/buffer.hpp"
#include "../pipelines/compute/compute_pipeline.hpp"
#include "../vk/context.hpp"
#include "resource_manager.hpp"

#include <glm/glm.hpp>

#include <memory>

#define MAX_DISPATCHES_PER_FRAME (1)

namespace tmx {

struct TerrainManager {
  public:
  TerrainManager(
    Context* vk_context,
    EventBus* event_bus,
    GraphicsPipeline* pipeline,
    ResourceManager* resource_manager
  ) : 
    vk_context{vk_context},
    event_bus{event_bus},
    pipeline{pipeline},
    resource_manager{resource_manager} {

    event_bus->add<IsosurfaceGenerationEvent>(this, &TerrainManager::generate_isosurface);
    event_bus->add<IsosurfaceMeshingEvent>(this, &TerrainManager::mesh_isosurface);
    event_bus->add<IsosurfaceModificationEvent>(this, &TerrainManager::modify_isosurface);

    compute_queue = vk_context->get_compute_queue();


    std::ifstream file("../../../assets/bin/MarchingCubesLUT.bin", std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error("Failed to open MarchingCubesLUT.bin!");
    }

    u32 file_size = static_cast<u32>(file.tellg());
    i8 *mc_lut = new i8[file_size/sizeof(i8)];
    file.seekg(0);
    file.read(reinterpret_cast<char *>(mc_lut), file_size);
    file.close();
    gpu_LUT =
      resource_manager->create_buffer<i32>(
        256*15*sizeof(i32),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );

    i32 *ptr_gpu_LUT = gpu_LUT->host_address();
    for(int idx = 0; idx < 256*15; idx++) {
      ptr_gpu_LUT[idx] = mc_lut[idx];
    }
    gpu_LUT->unmap_memory();
    delete[] mc_lut;

    std::ifstream file2("../../../assets/bin/MarchingCubesVertexCountLUT.bin", std::ios::ate | std::ios::binary);

    if (!file2.is_open()) {
      throw std::runtime_error("Failed to open MarchingCubesVertexCountLUT.bin!");
    }

    u32 file_size2 = static_cast<u32>(file2.tellg());
    u8 *mc_vc_lut = new u8[file_size/sizeof(u8)];
    file2.seekg(0);
    file2.read(reinterpret_cast<char *>(mc_vc_lut), file_size2);
    file2.close();
    gpu_vertex_count_LUT =
      resource_manager->create_buffer<u32>(
        256*sizeof(u32),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );

    u32 *ptr_gpu_vertex_count_LUT = gpu_vertex_count_LUT->host_address();
    for(int idx2 = 0; idx2 < 256; idx2++) {
      ptr_gpu_vertex_count_LUT[idx2] = mc_vc_lut[idx2];
    }
    gpu_vertex_count_LUT->unmap_memory();
    delete[] mc_vc_lut;
	  
    const uint2 EDGES[12] = {
      uint2{0, 1},
      uint2{1, 2},
      uint2{2, 3},
      uint2{3, 0},
      uint2{4, 5},
      uint2{5, 6},
      uint2{6, 7},
      uint2{7, 4},
      uint2{0, 4},
      uint2{1, 5},
      uint2{2, 6},
      uint2{3, 7},
    };
 
    const uint4 POINTS[8] = {
      uint4{0, 0, 0, 0},
      uint4{0, 0, 1, 0},
      uint4{1, 0, 1, 0},
      uint4{1, 0, 0, 0},
      uint4{0, 1, 0, 0},
      uint4{0, 1, 1, 0},
      uint4{1, 1, 1, 0},
      uint4{1, 1, 0, 0},
    };

    gpu_edges_triangle_assembly_lut =
      resource_manager->create_buffer<uint2>(
        sizeof(uint2)*12,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );
    memcpy(gpu_edges_triangle_assembly_lut->host_address(), EDGES, sizeof(uint2)*12);


    gpu_points_triangle_assembly_lut =
      resource_manager->create_buffer<uint4>(
        sizeof(uint4)*8,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );
    memcpy(gpu_points_triangle_assembly_lut->host_address(), POINTS, sizeof(uint4)*8);

    gpu_ptr_table =
      resource_manager->create_buffer<McPtrTable>(
        sizeof(McPtrTable),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );
    McPtrTable table{
      .pConfigurations = SHADER_CAST(gpu_LUT->device_address()),
      .pVertexCounts = SHADER_CAST(gpu_vertex_count_LUT->device_address()),
      .pEdges = SHADER_CAST(gpu_edges_triangle_assembly_lut->device_address()),
      .pPoints = SHADER_CAST(gpu_points_triangle_assembly_lut->device_address()),
    };
    memcpy(gpu_ptr_table->host_address(), &table, sizeof(McPtrTable));

    /***********************************/
    /***********************************/
	  
    gpu_indirect_cmds =
      resource_manager->create_buffer<VkDrawIndirectCommand>(
        INT16_MAX*sizeof(VkDrawIndirectCommand),
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );

    gpu_voxels =
      resource_manager->create_buffer<f32>(
        INT32_MAX-1,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );


    gpu_vertices =
      resource_manager->create_buffer<float4>(
        INT32_MAX-1,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );

    gpu_allocator =
      resource_manager->create_buffer<Allocator>(
        // lock       free (ALLOCATOR_MAX_ALLOCATIONS)
        sizeof(u32) + sizeof(u32)*ALLOCATOR_MAX_ALLOCATIONS,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );
    for(u32 i = 0; i < ALLOCATOR_MAX_ALLOCATIONS; i++) {
      gpu_allocator->host_address()->free[i] = 0;
    }
    gpu_allocator->host_address()->locked = 0;


    gpu_chunk_draw_info =
      resource_manager->create_buffer<uint2>(
        sizeof(uint2)*COUNT_CHUNKS,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );
    memset(gpu_chunk_draw_info->host_address(), 0, sizeof(uint2)*COUNT_CHUNKS);

    gpu_globals =
      resource_manager->create_buffer<GpuGlobals>(
        sizeof(GpuGlobals),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        TMX_MEMORY_PROPERTY_UNIFIED,
        TMX_BUFFER_CREATE_MAPPED_BIT
      );
    memset(gpu_globals->host_address(), 0, sizeof(GpuGlobals));

    std::cout << "Initialized terrain system." << std::endl;

  }

  ~TerrainManager(void) = default;

  void generate_isosurface(const std::any &e) {
    const auto &event = std::any_cast<const IsosurfaceGenerationEvent &>(e);
    isosurface_chunks_progress = chunks_per_axis;
    return;

    VkCommandBuffer command_buffer = vk_context->begin_command_buffers<1>();
    
    isosurface_generation_pipeline.cmd_bind_pipeline(command_buffer);

    for(i32 chunk_z = isosurface_chunks_progress.z;
        chunk_z < chunks_per_axis.z;
        chunk_z++
       ) {
    for(i32 chunk_y = isosurface_chunks_progress.y;
        chunk_y < chunks_per_axis.y;
        chunk_y++
       ) {
    for(i32 chunk_x = isosurface_chunks_progress.x;
        chunk_x < chunks_per_axis.x;
        chunk_x++
       ) {

      IsosurfaceGenerationPush isosurface_generation_push {
        .pVoxels = SHADER_CAST(gpu_voxels->device_address()),
        .pChunkDrawInfo = SHADER_CAST(gpu_chunk_draw_info->device_address()),
        .pIndirect = SHADER_CAST(gpu_indirect_cmds->device_address()),
        .pGpuGlobals = SHADER_CAST(gpu_globals->device_address()),
        .chunk_pos = int4{chunk_x, chunk_y, chunk_z, 0},
      };

      isosurface_generation_pipeline.cmd_dispatch(
        command_buffer,
        1,
        1,
        1,
        &isosurface_generation_push
      );

    }
    }
    }

    isosurface_chunks_progress.x = chunks_per_axis.x;
    isosurface_chunks_progress.y = chunks_per_axis.y;
    isosurface_chunks_progress.z = chunks_per_axis.z;
	
    vk_context->end_command_buffer(command_buffer);
    vk_context->queue_submit(command_buffer, TmxSubmitInfo{compute_queue, 0, 0, 0, 0});
    vk_context->queue_wait_idle(compute_queue);
    vk_context->free_command_buffers<1>(&command_buffer);
	
    if(isosurface_chunks_progress == chunks_per_axis) {
      std::cout << "ISOSURFACE all finished\n" << std::endl;
    }
    else {
      std::cout << "ISOSURFACE one finished" << std::endl;
      event_bus->notify(IsosurfaceGenerationEvent{isosurface_chunks_progress});
    }

  }
  
  void mesh_isosurface(const std::any &e) {
    const auto &event = std::any_cast<const IsosurfaceMeshingEvent &>(e);

    for(i32 chunk_z = meshing_chunks_progress.z;
        chunk_z < chunks_per_axis.z;
        chunk_z++
       ) {
    for(i32 chunk_y = meshing_chunks_progress.y;
        chunk_y < chunks_per_axis.y;
        chunk_y++
       ) {

    VkCommandBuffer command_buffer = vk_context->begin_command_buffers<1>();
    
    isosurface_meshing_pipeline.cmd_bind_pipeline(command_buffer);

    for(i32 chunk_x = meshing_chunks_progress.x;
        chunk_x < chunks_per_axis.x;
        chunk_x++
       ) {

      IsosurfaceMeshingPush isosurface_meshing_push {
        .pAllocator = SHADER_CAST(gpu_allocator->device_address()),
        .pMcPtrTable = SHADER_CAST(gpu_ptr_table->device_address()),
        .pVertices = SHADER_CAST(gpu_vertices->device_address()),
        .pVoxels = SHADER_CAST(gpu_voxels->device_address()),
		    .pIndirect = SHADER_CAST(gpu_indirect_cmds->device_address()),
		    .pGpuGlobals = SHADER_CAST(gpu_globals->device_address()),
        .chunk_pos = int4{chunk_x, chunk_y, chunk_z, 0},
      };

      isosurface_meshing_pipeline.cmd_dispatch(
        command_buffer,
        1,
        1,
        1,
        &isosurface_meshing_push
      );

    }
      vk_context->end_command_buffer(command_buffer);
      vk_context->queue_submit(
        command_buffer,
        TmxSubmitInfo{
          .queue = compute_queue,
          .waitSemaphoreInfoCount = 0,
          .pWaitSemaphoreInfos = nullptr,
          .signalSemaphoreInfoCount = 0,
          .pSignalSemaphoreInfos = nullptr,
        }
      );
      vk_context->queue_wait_idle(compute_queue);
      vk_context->free_command_buffers<1>(&command_buffer);

    }
      std::cout << chunk_z << std::endl;
    }

    meshing_chunks_progress.x = chunks_per_axis.x;
    meshing_chunks_progress.y = chunks_per_axis.y;
    meshing_chunks_progress.z = chunks_per_axis.z;
    
    if(meshing_chunks_progress == chunks_per_axis) {
      std::cout << "MESHING all finished\n" << std::endl;
      
      VkCommandBuffer cmd_buf = vk_context->begin_command_buffers<1>();
      isosurface_dc_pipeline.cmd_bind_pipeline(cmd_buf);

      IsosurfaceDrawCollectionPush push{
        .chunk_draw_info = SHADER_CAST(gpu_chunk_draw_info->device_address()),
        .indirect = SHADER_CAST(gpu_indirect_cmds->device_address()),
        .gpu_globals = SHADER_CAST(gpu_globals->device_address()),
      };

      isosurface_dc_pipeline.cmd_dispatch(
        cmd_buf,
        ((COUNT_CHUNKS_X+7)/8) * ((COUNT_CHUNKS_Y+7)/8) * ((COUNT_CHUNKS_Z+7)/8),
        1,
        1,
        &push
      );

      vk_context->end_command_buffer(cmd_buf);
      vk_context->queue_submit(
        cmd_buf,
        TmxSubmitInfo{
          .queue = compute_queue,
          .waitSemaphoreInfoCount = 0,
          .pWaitSemaphoreInfos = nullptr,
          .signalSemaphoreInfoCount = 0,
          .pSignalSemaphoreInfos = nullptr,
        }
      );
      vk_context->queue_wait_idle(compute_queue);
      vk_context->free_command_buffers<1>(&cmd_buf);
      
    }
    else {
      std::cout << "MESHING one finished" << std::endl;
      event_bus->notify(IsosurfaceMeshingEvent{meshing_chunks_progress});
    }

  }


  void modify_isosurface(const std::any &e) {
    std::abort();
    const auto &event = std::any_cast<const IsosurfaceModificationEvent &>(e);
  }


  [[nodiscard]] inline
  u32 get_chunk_render_count(void) const {
	  return gpu_globals->host_address()->mc_chunks_indirect_cmd_count;
  }
  
  [[nodiscard]] inline
  VkDrawIndirectCommand *get_indirect_cmds_host_address(void) const {
	  return gpu_indirect_cmds->host_address();
  }

  [[nodiscard]] inline
  float4* get_terrain_vertex_buffer_address(void) {
    return gpu_vertices->device_address();
  }

  [[nodiscard]] inline
  VkBuffer get_indirect_cmds_buffer(void) const {
	  return gpu_indirect_cmds->vk_buffer();
  }


  private:
  Context* vk_context;
  EventBus* event_bus;
  ResourceManager* resource_manager;
  VkQueue compute_queue;
  
  GraphicsPipeline* pipeline;
  ComputePipeline isosurface_generation_pipeline
  {
    "isosurface_generation",
    sizeof(IsosurfaceGenerationPush),
    vk_context->get_device()
  };
  ComputePipeline isosurface_meshing_pipeline
  {
    "isosurface_meshing",
    sizeof(IsosurfaceMeshingPush),
    vk_context->get_device()
  };

  int3 chunks_per_axis{COUNT_CHUNKS_X, COUNT_CHUNKS_Y, COUNT_CHUNKS_Z};
  int3 isosurface_chunks_progress{0, 0, 0};
  int3 meshing_chunks_progress{0, 0, 0};
  
  std::unique_ptr< DeviceBuffer<i32> >                     gpu_LUT;
  std::unique_ptr< DeviceBuffer<u32> >                     gpu_vertex_count_LUT;
  std::unique_ptr< DeviceBuffer<uint2> >                   gpu_edges_triangle_assembly_lut;
  std::unique_ptr< DeviceBuffer<uint4> >                   gpu_points_triangle_assembly_lut;
  std::unique_ptr< DeviceBuffer<McPtrTable> >              gpu_ptr_table;

  std::unique_ptr< DeviceBuffer<f32> >                     gpu_voxels;
  std::unique_ptr< DeviceBuffer<float4> >                  gpu_vertices;
  std::unique_ptr< DeviceBuffer<Allocator> >               gpu_allocator;
  std::unique_ptr< DeviceBuffer<uint2> >                   gpu_chunk_draw_info;
  std::unique_ptr< DeviceBuffer<VkDrawIndirectCommand> >   gpu_indirect_cmds; 
  std::unique_ptr< DeviceBuffer<GpuGlobals> >              gpu_globals;
};

}

#undef MAX_DISPATCHES_PER_AXIS_PER_FRAME