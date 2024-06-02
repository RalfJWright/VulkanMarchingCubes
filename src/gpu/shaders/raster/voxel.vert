#version 460

#define GRAPHICS_PUSH_CONSTANT
#include "../../../../src/shared/push.inl"

layout(location = 0) out float vcolor;

void main(){
  u32 vertexID = gl_VertexIndex;
  float4 vertex = deref(Vertex(pVertices[vertexID]));

  float4 mvVert = CameraMatrices(pMatrices).view_matrix * float4(vertex.xyz, 1.0);

  gl_Position = CameraMatrices(pMatrices).projection_matrix * mvVert;
  vcolor = vertex.w;
}