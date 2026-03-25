#include "pipeline.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

static std::vector<char> readFile(const std::string& filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("failed to open file: " + filename);

  size_t size = (size_t)file.tellg();
  std::vector<char> buffer(size);
  file.seekg(0);
  file.read(buffer.data(), size);
  return buffer;
}

VkShaderModule VulkanPipeline::createShaderModule(const VulkanDevice& device, const std::vector<char>& code) const
{
  VkShaderModuleCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  info.codeSize = code.size();
  info.pCode    = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule module;
  if (vkCreateShaderModule(device.getDevice(), &info, nullptr, &module) != VK_SUCCESS)
    throw std::runtime_error("failed to create shader module!");
  return module;
}

void VulkanPipeline::createDescriptorSetLayouts(const VulkanDevice& device)
{
  VkDescriptorSetLayoutBinding computeCameraBinding{};
  computeCameraBinding.binding = 0;
  computeCameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  computeCameraBinding.descriptorCount = 1;
  computeCameraBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutBinding computeAccumulationReadBinding{};
  computeAccumulationReadBinding.binding = 1;
  computeAccumulationReadBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  computeAccumulationReadBinding.descriptorCount = 1;
  computeAccumulationReadBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutBinding computeAccumulationWriteBinding{};
  computeAccumulationWriteBinding.binding = 2;
  computeAccumulationWriteBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  computeAccumulationWriteBinding.descriptorCount = 1;
  computeAccumulationWriteBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkDescriptorSetLayoutBinding computeBindings[] = {
    computeCameraBinding,
    computeAccumulationReadBinding,
    computeAccumulationWriteBinding
  };

  VkDescriptorSetLayoutCreateInfo computeLayoutInfo{};
  computeLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  computeLayoutInfo.bindingCount = 3;
  computeLayoutInfo.pBindings = computeBindings;

  if (vkCreateDescriptorSetLayout(device.getDevice(), &computeLayoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create compute descriptor set layout!");
  }

  VkDescriptorSetLayoutBinding displayAccumulationBinding{};
  displayAccumulationBinding.binding = 0;
  displayAccumulationBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  displayAccumulationBinding.descriptorCount = 1;
  displayAccumulationBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo displayLayoutInfo{};
  displayLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  displayLayoutInfo.bindingCount = 1;
  displayLayoutInfo.pBindings = &displayAccumulationBinding;

  if (vkCreateDescriptorSetLayout(device.getDevice(), &displayLayoutInfo, nullptr, &displayDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create display descriptor set layout!");
  }

  std::cout << "Created Descriptor Set Layouts\n";
}


void VulkanPipeline::createDisplayPipeline(const VulkanDevice& device,
                                           const VulkanRenderPass& renderPass,
                                           const char* vertSpvPath,
                                           const char* fragSpvPath)
{
  // shader modules
  auto vertCode = readFile(vertSpvPath);
  auto fragCode = readFile(fragSpvPath);

  VkShaderModule vertModule = createShaderModule(device, vertCode);
  VkShaderModule fragModule = createShaderModule(device, fragCode);

  VkPipelineShaderStageCreateInfo vertStage{};
  vertStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vertStage.module = vertModule;
  vertStage.pName  = "main";

  VkPipelineShaderStageCreateInfo fragStage{};
  fragStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStage.module = fragModule;
  fragStage.pName  = "main";

  VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount   = 0;
  vertexInput.vertexAttributeDescriptionCount = 0;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblerInfo{};
  inputAssemblerInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblerInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblerInfo.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo vpState{};
  vpState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vpState.viewportCount = 1;
  vpState.scissorCount  = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType                 = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE; 
  rasterizer.depthBiasEnable         = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multiSampling{};
  multiSampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multiSampling.rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT;
  multiSampling.sampleShadingEnable    = VK_FALSE;

  VkPipelineColorBlendAttachmentState cbAtt{};
  cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  cbAtt.blendEnable    = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo cblend{};
  cblend.sType          = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  cblend.logicOpEnable   = VK_FALSE;
  cblend.attachmentCount = 1;
  cblend.pAttachments    = &cbAtt;

  VkDynamicState dynamics[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dyn{};
  dyn.sType            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dyn.dynamicStateCount = 2;
  dyn.pDynamicStates    = dynamics;

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &displayDescriptorSetLayout;
  if (vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &displayPipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create display pipeline layout!");

  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.stageCount = 2;
  pipelineCreateInfo.pStages = stages;
  pipelineCreateInfo.pVertexInputState  = &vertexInput;
  pipelineCreateInfo.pInputAssemblyState = &inputAssemblerInfo;
  pipelineCreateInfo.pViewportState = &vpState;
  pipelineCreateInfo.pRasterizationState = &rasterizer;
  pipelineCreateInfo.pMultisampleState = &multiSampling;
  pipelineCreateInfo.pColorBlendState = &cblend;
  pipelineCreateInfo.pDynamicState = &dyn;
  pipelineCreateInfo.layout = displayPipelineLayout;
  pipelineCreateInfo.renderPass = renderPass.getRenderPass();
  pipelineCreateInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &displayPipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create display pipeline!");

  // shader modules go after pipeline creation
  vkDestroyShaderModule(device.getDevice(), fragModule, nullptr);
  vkDestroyShaderModule(device.getDevice(), vertModule, nullptr);

  std::cout << "Created Display Pipeline\n";
}

void VulkanPipeline::createComputePipeline(const VulkanDevice& device, const char* computeSpvPath)
{
  auto computeCode = readFile(computeSpvPath);
  VkShaderModule computeModule = createShaderModule(device, computeCode);

  VkPipelineShaderStageCreateInfo computeStage{};
  computeStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  computeStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  computeStage.module = computeModule;
  computeStage.pName = "main";

  VkPipelineLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutInfo.setLayoutCount = 1;
  layoutInfo.pSetLayouts = &computeDescriptorSetLayout;
  if (vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("failed to create compute pipeline layout!");

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.stage = computeStage;
  pipelineInfo.layout = computePipelineLayout;

  if (vkCreateComputePipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create compute pipeline!");

  vkDestroyShaderModule(device.getDevice(), computeModule, nullptr);
  std::cout << "Created Compute Pipeline\n";
}

void VulkanPipeline::destroy(const VulkanDevice& device)
{
  if (computePipeline)
  {
    vkDestroyPipeline(device.getDevice(), computePipeline, nullptr);
    computePipeline = VK_NULL_HANDLE;
  }
  if (displayPipeline) 
  {
    vkDestroyPipeline(device.getDevice(), displayPipeline, nullptr);
    displayPipeline = VK_NULL_HANDLE;
  }
  if (computePipelineLayout)
  {
    vkDestroyPipelineLayout(device.getDevice(), computePipelineLayout, nullptr);
    computePipelineLayout = VK_NULL_HANDLE;
  }
  if (displayPipelineLayout) 
  {
    vkDestroyPipelineLayout(device.getDevice(), displayPipelineLayout, nullptr);
    displayPipelineLayout = VK_NULL_HANDLE;
  }
  if (computeDescriptorSetLayout)
  {
    vkDestroyDescriptorSetLayout(device.getDevice(), computeDescriptorSetLayout, nullptr);
    computeDescriptorSetLayout = VK_NULL_HANDLE;
  }
  if (displayDescriptorSetLayout)
  {
    vkDestroyDescriptorSetLayout(device.getDevice(), displayDescriptorSetLayout, nullptr);
    displayDescriptorSetLayout = VK_NULL_HANDLE;
  }
  std::cerr << "Pipelines Destroyed\n";
}

