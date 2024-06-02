#ifndef _TYPES_INL_SHARED
#define _TYPES_INL_SHARED

#ifdef __cplusplus

#include <glm/glm.hpp>
using namespace glm;

#include <cstdint>
typedef float f32;
typedef double f64;
typedef std::int8_t   i8;
typedef std::uint8_t  u8;
typedef std::int16_t  i16;
typedef std::uint16_t u16;
typedef std::int32_t  i32;
typedef std::uint32_t u32;
typedef std::int64_t  i64;
typedef std::uint64_t u64;

typedef glm::mat4  float4x4;
typedef glm::vec2  float2;
typedef glm::vec3  float3;
typedef glm::vec4  float4;
typedef glm::dvec2 double2;
typedef glm::dvec3 double3;
typedef glm::dvec4 double4;

typedef glm::ivec2 int2;
typedef glm::ivec3 int3;
typedef glm::ivec4 int4;
typedef glm::uvec2 uint2;
typedef glm::uvec3 uint3;
typedef glm::uvec4 uint4;

#define PUSH(T) struct T
#define BDA(T) struct T
#define PTR(T) u64
#define push_assert(T) static_assert(sizeof(T) <= 128)

#else

// NOTE: Do not use Slang instead of GLSL.
// Although it is much nicer to use, and lets me avoid some macros,
// the code does not produce the same results as GLSL if the
// shader is more complex.

#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8  : require 
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_shader_atomic_int64 : require

#define VkDrawIndirectCommand uint4

#define i8  int8_t
#define u8  uint8_t
#define i16 int16_t
#define u16 uint16_t
#define i32 int
#define u32 uint
#define i64 int64_t
#define u64 uint64_t

#define float4x4 mat4
#define float2   vec2
#define float3   vec3
#define float4   vec4
#define double2  dvec2
#define double3  dvec3
#define double4  dvec4

#define int2  ivec2
#define int3  ivec3
#define int4  ivec4
#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define PUSH(T) layout(push_constant) uniform T
#define BDA(T) layout(buffer_reference, scalar, buffer_reference_align = 16) buffer T
#define PTR(T) u64
#define deref(var) var.value
#define push_assert(expr)
#define numthreads(sz_x, sz_y, sz_z) layout(local_size_x = sz_x, local_size_y = sz_y, local_size_z = sz_z) in;

#endif// #ifdef __cplusplus

#endif// #ifndef _TYPES_INL_SHARED
