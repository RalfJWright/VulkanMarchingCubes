#pragma once

#include "../../core/utils.hpp"
#include "../../vk/context.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <fstream>

namespace tmx {

  struct TmxGraphicsPipelineCreateInfo{
    const std::string &shader_file_name;
    const u32 push_constant_size;
    VkDevice &device;
    const VkFormat swapchain_format;
    const VkFormat depth_format;
    const VkBool32 enable_blending;
  };

  struct GraphicsPipeline {
    public:
    GraphicsPipeline(const TmxGraphicsPipelineCreateInfo &create_info) : device{create_info.device} {
      
      push_constant_size = create_info.push_constant_size;

      VkPushConstantRange push_constant_range{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = create_info.push_constant_size,
      };

      VkPipelineLayoutCreateInfo pipeline_layout_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
      };

      VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout));

      std::string vertex_shader_file = "../../../spv/" + create_info.shader_file_name + ".vert.spv";
      std::string fragment_shader_file = "../../../spv/" + create_info.shader_file_name + ".frag.spv";
      auto vertex_shader_code = read_file(vertex_shader_file);
      auto fragment_shader_code = read_file(fragment_shader_file);

      VkShaderModule vertex_shader_module = create_shader_module(vertex_shader_code);
      VkShaderModule fragment_shader_module = create_shader_module(fragment_shader_code);

      VkPipelineShaderStageCreateInfo vertex_shader_stage_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex_shader_module,
        .pName = "main",
      };

      VkPipelineShaderStageCreateInfo fragment_shader_stage_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment_shader_module,
        .pName = "main",
      };
      
      VkPipelineShaderStageCreateInfo shader_stages[] = {vertex_shader_stage_info, fragment_shader_stage_info};

      VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
      };

      VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
      };

      std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
      };

      VkPipelineDynamicStateCreateInfo dynamic_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
      };

      VkPipelineViewportStateCreateInfo viewport_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .scissorCount = 1,
      };

      VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0,
        .depthBiasClamp = 0.0,
        .depthBiasSlopeFactor = 0.0,
        .lineWidth = 1.0,
      };

      VkPipelineMultisampleStateCreateInfo multisample_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
      };

      VkPipelineColorBlendAttachmentState color_blend_attachment_state{
        .blendEnable = create_info.enable_blending,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT |
          VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT |
          VK_COLOR_COMPONENT_A_BIT
      };

      VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = {0.0, 0.0, 0.0, 0.0},
      };

      const VkPipelineRenderingCreateInfo pipeline_rendering_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &create_info.swapchain_format,
        .depthAttachmentFormat = create_info.depth_format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
      };

      VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
      };

      VkGraphicsPipelineCreateInfo pipeline_create_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_create_info,
        .flags = 0,
        .stageCount = 2,
        .pStages = shader_stages,

        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info,
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state_create_info,
        .pDynamicState = &dynamic_state_create_info,

        .layout = pipeline_layout,

        .renderPass = nullptr,
        .subpass = 0,
  
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
      };

      VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline));

      vkDestroyShaderModule(device, vertex_shader_module, nullptr);
      vkDestroyShaderModule(device, fragment_shader_module, nullptr);

    }

    ~GraphicsPipeline(void) {
      vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
      vkDestroyPipeline(device, pipeline, nullptr);
    }

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    VkPipelineLayout get_pipeline_layout() { return pipeline_layout; }
    VkPipeline get_pipeline() { return pipeline; }
    u32 get_push_constant_size() { return push_constant_size; }

    private:
    std::vector<char> read_file(const std::string &filename) {
      std::ifstream file(filename, std::ios::ate | std::ios::binary);

      if (!file.is_open()) {
        throw std::runtime_error(
          std::string{"Failed to open graphics shader file "} + std::string{filename} + std::string{" !"}
        );
      }

      u32 file_size = static_cast<u32>(file.tellg());
      std::vector<char> buffer(file_size);

      file.seekg(0);
      file.read(buffer.data(), file_size);
      file.close();

      return buffer;
    }

    VkShaderModule create_shader_module(const std::vector<char> &code) {
      VkShaderModuleCreateInfo shader_module_create_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const u32 *>(code.data()),
      };

      VkShaderModule shader_module;
      VK_CHECK(vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module));

      return shader_module;
    }

    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    u32 push_constant_size;
    VkDevice &device;
  };
}