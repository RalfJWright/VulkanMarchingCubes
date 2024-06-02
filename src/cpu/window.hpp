#pragma once

#define RENDERER_FRAMES_IN_FLIGHT 2

#include "core/utils.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cassert>

namespace tmx {
  struct Window {
    public:
    Window(i32 width, i32 height, const char* window_name) : width{width}, height{height} {
      glfwInit();

      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

      pWindow = glfwCreateWindow(width, height, window_name, nullptr, nullptr);
    }
    ~Window(void) {
      glfwDestroyWindow(pWindow);
      glfwTerminate();
      std::cout << "glfwTerminate();" << std::endl;
    }

    [[nodiscard]] inline
    bool should_close(void) const { return glfwWindowShouldClose(pWindow); }
    
    [[nodiscard]] inline
    GLFWwindow* get_glfw_window(void) const { return pWindow; }
    
    [[nodiscard]] inline
    i32 get_width(void) const { return width; }
    
    [[nodiscard]] inline
    i32 get_height(void) const { return height; }
    
    [[nodiscard]] inline
    float2 get_dim_f32(void) const { return float2{static_cast<f32>(width), static_cast<f32>(height)}; }
    
    [[nodiscard]] inline
    f32 get_aspect_ratio(void) const { return static_cast<f32>(width)/static_cast<f32>(height); }

    private:
    GLFWwindow* pWindow;
    i32 width;
    i32 height;
    bool resized;
  };
};
