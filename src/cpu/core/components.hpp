#pragma once

#include <types.inl>

#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace tmx {

  struct TransformComponent {
    TransformComponent(const float3& translation, const float3& rotation, const float3 scale)
      : translation{translation}, rotation{rotation}, scale{scale} {}
	
	~TransformComponent() = default;


    operator mat4() const {
      mat4 transform = glm::translate(mat4{1.f}, translation);
      transform = glm::rotate(transform, rotation.y, {0.0, 1.0, 0.0});
      transform = glm::rotate(transform, rotation.x, {1.0, 0.0, 0.0});
      transform = glm::rotate(transform, rotation.z, {0.0, 0.0, 1.0});
      return glm::scale(transform, scale);
    }
    
    float3 translation{0.0, 0.0, 0.0};
    float3 rotation{0.0, 0.0, 0.0};
    float3 scale{1.0, 1.0, 1.0};
  };
}