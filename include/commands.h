#pragma once
#include <vulkan/vulkan_core.h>
#include <vector>
#include "device.h"
#include "swapchain.h"
#include "renderpass.h"
#include "pipeline.h"

class VulkanCommands
{
public:
  void createCommandPool(const VulkanDevice& device);

  void createCommandBuffers(const VulkanDevice& device, uint32_t count);
  void recordCommandBuffer(uint32_t imageIndex,
                           const VulkanSwapchain& swapchain,
                           const VulkanRenderPass& renderPass,
                           const VulkanPipeline& pipeline,
                           VkFramebuffer framebuffer,
                           VkDescriptorSet computeDescriptorSet,
                           VkDescriptorSet displayDescriptorSet,
                           VkImage accumulationReadImage,
                           VkImage accumulationWriteImage);

  void destroy(const VulkanDevice& device);

  VkCommandPool getCommandPool()   const {return commandPool;}
  const std::vector<VkCommandBuffer>& getCommandBuffers() const {return commandBuffers;}

private:
  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> commandBuffers;
};
