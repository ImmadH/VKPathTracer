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

  void createCommandBuffers(const VulkanDevice& device,
                            const VulkanSwapchain& swapchain,
                            const VulkanRenderPass& renderPass,
                            const VulkanPipeline& pipeline,
                            const std::vector<VkFramebuffer>& framebuffers);

  void destroy(const VulkanDevice& device);

  VkCommandPool getCommandPool()   const {return commandPool;}
  const std::vector<VkCommandBuffer>& getCommandBuffers() const {return commandBuffers;}

private:
  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> commandBuffers;
};
