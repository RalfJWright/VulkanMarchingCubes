#pragma once
#include <push.inl>

#include "core/components.hpp"
#include "core/event_bus.hpp"

#include "systems/resource_manager.hpp"

#include "input.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace tmx {
  struct Camera {
    public:
    struct KeyMapping {
      int move_up = GLFW_KEY_E;
      int move_down = GLFW_KEY_Q;
      int move_left = GLFW_KEY_A;
      int move_right = GLFW_KEY_D;
      int move_forward = GLFW_KEY_W;
      int move_backward = GLFW_KEY_S;
      int look_up = GLFW_KEY_UP;
      int look_down = GLFW_KEY_DOWN;
      int look_left = GLFW_KEY_LEFT;
      int look_right = GLFW_KEY_RIGHT;
      int decrease_move_sensitivity = GLFW_KEY_MINUS;
      int increase_move_sensitivity = GLFW_KEY_EQUAL;
    };

    Camera(TransformComponent transform, f32 near, f32 far, Window* window, Input* input, EventBus* event_bus, ResourceManager* resource_manager, f64* pdt)
        : transform{transform}, near{near}, far{far}, window{window}, input{input}, event_bus{event_bus}, resource_manager{resource_manager}, pdelta{pdt} {

      for(int i = 0; i < RENDERER_FRAMES_IN_FLIGHT; i++) {
        gpu_matrices[i] =
          resource_manager->create_buffer<CameraMatrices>(
            sizeof(CameraMatrices),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            TMX_MEMORY_PROPERTY_UNIFIED,
            TMX_BUFFER_CREATE_MAPPED_BIT
        );
      }

          event_bus->add<IsosurfaceModificationInitialEvent>(this, &Camera::initial_response);

    }
    ~Camera(void) = default;

    void initial_response(const std::any &event) {
      const auto &e = std::any_cast<const IsosurfaceModificationInitialEvent &>(event);

      // Note: clip is not ndc
      float4 mouse_clip = 
      {
        e.cursor_pos.x * 2.0 / window_dim.x - 1.0,
        1.0 - e.cursor_pos.y * 2.0 / window_dim.y,
        0.0,
        1.0,
      };
      
      float3 mouse_worldspace =
        glm::inverse(mat4{transform}) * glm::inverse(matrices.projection_matrix) * mouse_clip;

      float3 ray_pos = transform.translation;
      float3 ray_dir = glm::normalize(mouse_worldspace - ray_pos);

      std::cout << "MOUSE RAY " <<
      ray_pos.x << " " <<
      ray_pos.y << " " <<
      ray_pos.z << " " <<
      std::endl;

      event_bus->notify<IsosurfaceModificationEvent>(
        IsosurfaceModificationEvent{.ray = Ray{ray_pos, ray_dir}}
      );
    }

    void update_perspective(f32 fovy, f32 aspect) {
      this->fovy = fovy;
      this->aspect = aspect;

      const f32 tan_half_fovy = tan(fovy / 2.f);
      auto projection_matrix = glm::mat4{0.0};
      projection_matrix[0][0] = 1.f / (aspect * tan_half_fovy);
      projection_matrix[1][1] = 1.f / (tan_half_fovy);
      projection_matrix[2][2] = far / (far - near);
      projection_matrix[2][3] = 1.f;
      projection_matrix[3][2] = -(far * near) / (far - near);
      matrices.projection_matrix = projection_matrix;
    }

    void update_view(void) {
      auto position = transform.translation;
      auto rotation = transform.rotation;
      const f32 c1 = glm::cos(rotation.y);
      const f32 s1 = glm::sin(rotation.y);
      const f32 c2 = glm::cos(rotation.x);
      const f32 s2 = glm::sin(rotation.x);
      const f32 c3 = glm::cos(rotation.z);
      const f32 s3 = glm::sin(rotation.z);
      const float3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
      const float3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
      const float3 w{(c2 * s1), (-s2), (c1 * c2)};
      mat4 view_matrix{1.0};
      view_matrix[0][0] = u.x;
      view_matrix[1][0] = u.y;
      view_matrix[2][0] = u.z;
      view_matrix[0][1] = v.x;
      view_matrix[1][1] = v.y;
      view_matrix[2][1] = v.z;
      view_matrix[0][2] = w.x;
      view_matrix[1][2] = w.y;
      view_matrix[2][2] = w.z;
      view_matrix[3][0] = -glm::dot(u, position);
      view_matrix[3][1] = -glm::dot(v, position);
      view_matrix[3][2] = -glm::dot(w, position);
      matrices.view_matrix = view_matrix;
    }

    void process_input() {
      float3 rotate{0};
      if(input->get_key(mapping.look_up,    Input::ACTION_PRESS)) rotate.x -= 1.0;
      if(input->get_key(mapping.look_down,  Input::ACTION_PRESS)) rotate.x += 1.0;
      if(input->get_key(mapping.look_right, Input::ACTION_PRESS)) rotate.y += 1.0;
      if(input->get_key(mapping.look_left,  Input::ACTION_PRESS)) rotate.y -= 1.0;

      if(glm::dot(rotate, rotate) > std::numeric_limits<f32>::epsilon()) {
        transform.rotation += glm::normalize(rotate) * static_cast<f32>(look_sensitivity * (*pdelta));
      }

      transform.rotation.x = glm::clamp(transform.rotation.x, -1.5f, 1.5f);
      transform.rotation.y = glm::mod(transform.rotation.y, glm::two_pi<f32>());

      const f32 yaw = transform.rotation.y;
      const float3 up{0.0, 1.0, 0.0};
      const float3 forward{glm::sin(yaw), 0.0, glm::cos(yaw)};
      const float3 right{glm::cross(up, forward)};

      float3 move_direction{};
      if(input->get_key(mapping.move_up      , Input::ACTION_PRESS)) move_direction += up;
      if(input->get_key(mapping.move_down    , Input::ACTION_PRESS)) move_direction -= up;
      if(input->get_key(mapping.move_right   , Input::ACTION_PRESS)) move_direction += right;
      if(input->get_key(mapping.move_left    , Input::ACTION_PRESS)) move_direction -= right;
      if(input->get_key(mapping.move_forward , Input::ACTION_PRESS)) move_direction += forward;
      if(input->get_key(mapping.move_backward, Input::ACTION_PRESS)) move_direction -= forward;

      if(glm::dot(move_direction, move_direction) > std::numeric_limits<f32>::epsilon()) {
        transform.translation += glm::normalize(move_direction) * static_cast<f32>(move_sensitivity * (*pdelta));
      }
    }

    inline void update_self_data(const u32 frame, const f32 fovy, const f32 aspect, const float2 w_dim) {
      update_perspective(fovy, aspect);
      update_view();
      window_dim = w_dim;
	  
      *(gpu_matrices[frame]->host_address()) = matrices;
    }
	
    [[nodiscard]] inline
    CameraMatrices* matrices_device_address(const u32 frame) {
      return gpu_matrices[frame]->device_address();
    }

    [[nodiscard]] inline
    CameraMatrices& get_matrices(void) {
      return matrices;
    }

    TransformComponent transform;
    
    private:
    CameraMatrices matrices{};
    std::unique_ptr< DeviceBuffer<CameraMatrices> > gpu_matrices[RENDERER_FRAMES_IN_FLIGHT];
    Window* window;
    Input* input;
    EventBus* event_bus;
    ResourceManager* resource_manager;
    f64* pdelta;
    f32 fovy, aspect, near, far;
    f64 move_sensitivity{0.01};
    f64 look_sensitivity{0.0015};
    KeyMapping mapping{};
    float2 window_dim;
  };
}
