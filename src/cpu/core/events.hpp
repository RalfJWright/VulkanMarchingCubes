#pragma once

#include <push.inl>

struct IsosurfaceGenerationEvent {
  int3 progress;
};

struct IsosurfaceMeshingEvent {
  int3 progress;
};

struct IsosurfaceModificationInitialEvent {
  double2 cursor_pos;
};

struct Ray {
  float3 pos;
  float3 dir;
};

struct IsosurfaceModificationEvent {
  Ray ray;
};