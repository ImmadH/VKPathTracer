#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
#include <string>
#include "device.h"
#include "swapchain.h"
#include "renderpass.h"
class VulkanPipeline
{
  public:
  void createDescriptorSetLayouts(const VulkanDevice& device);
  void createDisplayPipeline(const VulkanDevice& device,
                             const VulkanRenderPass& renderPass,
                             const char* vertSpvPath = "shaders/vert.spv",
                             const char* fragSpvPath = "shaders/frag.spv");
  void createComputePipeline(const VulkanDevice& device,
                             const char* computeSpvPath = "shaders/pathtrace.comp.spv");

  void destroy(const VulkanDevice& device);
  VkPipeline getDisplayPipeline() const { return displayPipeline; }
  VkPipelineLayout getDisplayLayout() const { return displayPipelineLayout; }
  VkDescriptorSetLayout getDisplayDescriptorSetLayout() const { return displayDescriptorSetLayout; }
  VkPipeline getComputePipeline() const { return computePipeline; }
  VkPipelineLayout getComputeLayout() const { return computePipelineLayout; }
  VkDescriptorSetLayout getComputeDescriptorSetLayout() const { return computeDescriptorSetLayout; }
private:
  VkShaderModule createShaderModule(const VulkanDevice& device, const std::vector<char>& code) const;

  VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout displayDescriptorSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout displayPipelineLayout = VK_NULL_HANDLE;
  VkPipeline computePipeline = VK_NULL_HANDLE;
  VkPipeline displayPipeline = VK_NULL_HANDLE;
};
