#ifndef _PUSH_INL_
#define _PUSH_INL_

#include "types.inl"

#if defined(__cplusplus)
#include <vulkan/vulkan.h>
#endif

#define COUNT_VOXELS_X (8)
#define COUNT_VOXELS_Y (8)
#define COUNT_VOXELS_Z (8)

#define COUNT_CHUNKS_X (8)
#define COUNT_CHUNKS_Y (8)
#define COUNT_CHUNKS_Z (8)

#define COUNT_CHUNKS (COUNT_CHUNKS_X*COUNT_CHUNKS_Y*COUNT_CHUNKS_Z)

// Refactoring...

BDA(Vertex) {
  float4 value;
};

BDA(Voxel) {
  float value;
};

BDA(CameraMatrices) {
  float4x4 projection_matrix;
  float4x4 view_matrix;
};

BDA(McConfigurationLUT) {
  i32 configurations[1];
};

BDA(McVertexCountLUT) {
  u32 vertex_counts[1];
};

BDA(McEdgesTriangleAssemblyLUT) {
  uint2 edges[1];
};

BDA(McPointsTriangleAssemblyLUT) {
  uint4 points[1];
};

BDA(McPtrTable) {
  PTR(McConfigurationLUT)          pConfigurations;
  PTR(McVertexCountLUT)            pVertexCounts;
  PTR(McEdgesTriangleAssemblyLUT)  pEdges;
  PTR(McPointsTriangleAssemblyLUT) pPoints;
};


BDA(GpuGlobals) {
  u32 mc_chunks_indirect_cmd_count;
};

BDA(ChunkDrawInfo) {
  uint2 value;
};

#define ALLOCATOR_PAGE_SIZE 8192
#define ALLOCATOR_MAX_ALLOCATIONS 2147483647/ALLOCATOR_PAGE_SIZE

#define LOCKED 0
#define UNLOCKED 1

struct Allocator {
  u32 locked;
  u32 free[1];
};


#if defined(GRAPHICS_PUSH_CONSTANT) || defined(__cplusplus)
PUSH(GraphicsPush) {
  PTR(Vertex)         pVertices[1];
  PTR(CameraMatrices) pMatrices;
};
#endif
push_assert(GraphicsPush);


#if defined(GRAPHICS_DRAW_COLLECTION_CONSTANT) || defined(__cplusplus)
PUSH(IsosurfaceDrawCollectionPush) {
  PTR(ChunkDrawInfo)        pChunkDrawInfo[1];
  VkDrawIndirectCommand     pIndirect;
  PTR(GpuGlobals)           pGpuGlobals;
};
#endif
push_assert(IsosurfaceDrawCollectionPush);


#if defined(ISOSURFACE_GENERATION_PUSH_CONSTANT) || defined(__cplusplus)
PUSH(IsosurfaceGenerationPush) {
  PTR(Voxel)                 pVoxels[1];
  PTR(ChunkDrawInfo)         pChunkDrawInfo;
  PTR(VkDrawIndirectCommand) pIndirect;
  PTR(GpuGlobals)            pGpuGlobals;
  int4                       chunk_pos;
};
push_assert(IsosurfaceGenerationPush);
#endif


#if defined(ISOSURFACE_MESHING_PUSH_CONSTANT) || defined(__cplusplus)
PUSH(IsosurfaceMeshingPush) {
  PTR(Allocator)             pAllocator;
  PTR(McPtrTable)            pMcPtrTable;
              
  PTR(Vertex)                pVertices;
  PTR(Voxel)                 pVoxels;

  PTR(VkDrawIndirectCommand) pIndirect;
  PTR(GpuGlobals)            pGpuGlobals;

  int4                       chunk_pos;
};
#endif
push_assert(IsosurfaceMeshingPush);

/***************************************************************/

#ifndef __cplusplus
#define static
#define inline
#endif

inline static u32 fromcoord(i32 x, i32 y, i32 z, i32 cx, i32 cy, i32 cz) {
  return x+y*COUNT_VOXELS_X+z*COUNT_VOXELS_X*COUNT_VOXELS_Y
         +
         cx*COUNT_VOXELS_X*COUNT_VOXELS_Y*COUNT_VOXELS_Z
         +cy*COUNT_VOXELS_X*COUNT_VOXELS_Y*COUNT_VOXELS_Z*COUNT_CHUNKS_X
         +cz*COUNT_VOXELS_X*COUNT_VOXELS_Y*COUNT_VOXELS_Z*COUNT_CHUNKS_X*COUNT_CHUNKS_Y;
}

inline static u32 fromcoord(int3 voxel_pos, int3 chunk_pos) {
  return fromcoord(voxel_pos.x, voxel_pos.y, voxel_pos.z, chunk_pos.x, chunk_pos.y, chunk_pos.z);
}

inline static u32 voxel2idx(int3 voxel_pos) {
  return voxel_pos.x+voxel_pos.y*COUNT_VOXELS_X+voxel_pos.z*COUNT_VOXELS_X*COUNT_VOXELS_Y;
}

inline static u32 chunk2idx(int3 chunk_pos) {
  return chunk_pos.x+chunk_pos.y*COUNT_CHUNKS_X+chunk_pos.z*COUNT_CHUNKS_X*COUNT_CHUNKS_Y;
}

inline static u32 flatten(int3 pos, int dimensions) {
  return pos.x+pos.y*dimensions+pos.z*dimensions*dimensions;
}

#endif // #ifndef _PUSH_INL_
