#pragma once

#include <types.inl>
#include <vulkan/vk_enum_string_helper.h>
#include <iostream>

#pragma diag_suppress 20
#define VK_CHECK(FN) \
do { \
  VkResult result = FN; \
  if (result != VK_SUCCESS) { \
    std::string color = "\n\033[1;31m"; \
    std::string colour = "\033[0m\n"; \
    std::string str = "\n[[FATAL ERROR]]\n"; \
    throw std::runtime_error( \
      color + str + std::string{string_VkResult(result)} + colour \
    ); \
  } \
} while(0)
#pragma diag_default 20

#define SHADER_CAST(ptr) ((u64)(ptr))