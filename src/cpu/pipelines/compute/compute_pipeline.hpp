#pragma once

#include <push.inl>
#include "../../core/utils.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <fstream>

namespace tmx {

  struct ComputePipeline {
    public:
    ComputePipeline(const std::string &shader_file_name, const size_t push_constant_size, VkDevice &device)
                  : push_constant_size{push_constant_size}, device{device} {
      std::string f = "../../../spv/" + shader_file_name + ".comp.spv";
      auto compute_code = read_file(f);
      VkShaderModule shader_module;
      create_shader_module(compute_code, &shader_module);
      
      VkPipelineShaderStageRequiredSubgroupSizeCreateInfo required_subgroup_size_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO,
        .pNext = nullptr,
        .requiredSubgroupSize = static_cast<u32>(32),
      };

      VkPipelineShaderStageCreateInfo shader_stage_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &required_subgroup_size_info,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader_module,
        .pName = "main",
        .pSpecializationInfo = nullptr,
      };

      VkPushConstantRange range{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = static_cast<u32>(push_constant_size),
      };

      VkPipelineLayoutCreateInfo layout_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range,
      };

      VK_CHECK(vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipeline_layout));

      VkComputePipelineCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_PIPELINE_CREATE_DISPATCH_BASE,
        .stage = shader_stage_create_info,
        .layout = pipeline_layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
      };

      VK_CHECK(
        vkCreateComputePipelines(
                device,
                nullptr,
                1,
                &create_info,
                nullptr,
                &pipeline
        )
      );

      vkDestroyShaderModule(device, shader_module, nullptr);
    };

    ~ComputePipeline() {
      vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
      vkDestroyPipeline(device, pipeline, nullptr);
    }

    void cmd_bind_pipeline(VkCommandBuffer command_buffer) {
      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    }

    void cmd_dispatch(
      VkCommandBuffer command_buffer,
      const u32 groupCountX,
      const u32 groupCountY,
      const u32 groupCountZ,
      const void *push
    ) {

      vkCmdPushConstants(
        command_buffer,
        pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        push_constant_size,
        push
      );

      vkCmdDispatch(command_buffer, groupCountX, groupCountY, groupCountZ);
    }

    void cmd_dispatch_indirect(
      VkCommandBuffer command_buffer,
      VkBuffer buffer,
      VkDeviceSize offset,
      const void *push
    ) {
      
      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

      vkCmdPushConstants(
        command_buffer,
        pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        push_constant_size,
        push
      );

      vkCmdDispatchIndirect(command_buffer, buffer, offset);
    }

    void cmd_dispatch_base(
      VkCommandBuffer command_buffer,
      const u32 baseGroupX,
      const u32 baseGroupY,
      const u32 baseGroupZ,
      const u32 groupCountX,
      const u32 groupCountY,
      const u32 groupCountZ,
      const void *push
    ) {
      
      vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

      vkCmdPushConstants(
        command_buffer,
        pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        push_constant_size,
        push
      );

      vkCmdDispatchBase(command_buffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
    }

    private:
    std::vector<char> read_file(const std::string &filename) {
      std::ifstream file(filename, std::ios::ate | std::ios::binary);

      if (!file.is_open()) {
        throw std::runtime_error(
          std::string{"Failed to open compute shader file "} + std::string{filename} + std::string{" !"}
        );
      }

      size_t file_size = (size_t) file.tellg();
      std::vector<char> buffer(file_size);

      file.seekg(0);
      file.read(buffer.data(), file_size);

      file.close();

      return buffer;
    }

    void create_shader_module(const std::vector<char> &code, VkShaderModule *module) {
      VkShaderModuleCreateInfo shader_module_create_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const u32 *>(code.data()),
      };

      VK_CHECK(vkCreateShaderModule(device, &shader_module_create_info, nullptr, module));
    }

    VkDevice &device;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    const size_t push_constant_size;
  };
}