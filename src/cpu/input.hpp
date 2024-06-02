#pragma once

#include "core/utils.hpp"
#include "core/event_bus.hpp"
#include "core/events.hpp"

#include "window.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <memory>
#include <limits>

namespace tmx {
  struct Input {
    public:
    static constexpr i32 ACTION_RELEASE = GLFW_RELEASE;
    static constexpr i32 ACTION_PRESS   = GLFW_PRESS;
    static constexpr i32 ACTION_REPEAT  = GLFW_REPEAT;
    static constexpr i32 ACTION_NONE    = 3;

    Input(GLFWwindow* pWindow, EventBus* pEventBus, f64* pdt) : pWindow{pWindow}, pEventBus{pEventBus}, pdelta{pdt} {
      glfwSetWindowUserPointer(pWindow, this);
      glfwSetCursorPosCallback(
        pWindow,
        [](GLFWwindow* pWindow, f64 x, f64 y)
        {
          auto &input = *reinterpret_cast<Input*>(glfwGetWindowUserPointer(pWindow));
          input.cursor_pos_callback(x, y);
        }
      );

      glfwSetMouseButtonCallback(
        pWindow,
        [](GLFWwindow* pWindow, i32 button, i32 action, i32 mods)
        {
          auto &input = *reinterpret_cast<Input*>(glfwGetWindowUserPointer(pWindow));
          input.mouse_button_callback(button, action);
        }
      );

      glfwSetKeyCallback(
        pWindow,
        [](GLFWwindow* pWindow, i32 key, i32 scancode, i32 action, i32 mods)
        {
          auto &input = *reinterpret_cast<Input*>(glfwGetWindowUserPointer(pWindow));
          input.key_callback(key, scancode, action, mods);
        }
      );
    }
    ~Input() = default;

    void cursor_pos_callback(const f64 x, const f64 y) {
      cursor_pos = {x, y};
    }

    void process_events(void) {
      glfwPollEvents();
      for(i32 i = 0; i < 3; i++){
        if(buttons[GLFW_MOUSE_BUTTON_LEFT] == ACTION_PRESS) {
          mouse_repeat_frequency += *pdelta;
          if(mouse_repeat_frequency > 225.0) {

            mouse_repeat_frequency = 0.0;
            pEventBus->notify<IsosurfaceModificationInitialEvent>(
              IsosurfaceModificationInitialEvent{cursor_pos});

          }
        }
      }
    }

    void key_callback(i32 key, i32 scancode, i32 action, i32 mods) {
      switch(key) {
        case GLFW_KEY_UNKNOWN:
        default:
          break;
      }
    }

    void mouse_button_callback(i32 button, i32 action) {
      buttons[button] = action;
    }

    bool get_key(const i32 key, const i32 action) {
      return glfwGetKey(pWindow, key) == action;
    }

    private:
    GLFWwindow* pWindow;
    EventBus* pEventBus;
    f64* pdelta;
    double2 cursor_pos;
    i32 buttons[3]{ACTION_NONE, ACTION_NONE, ACTION_NONE};
    f32 mouse_repeat_frequency{0.0};
  };
}