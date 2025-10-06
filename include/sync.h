#pragma once
#include <vulkan/vulkan_core.h>
#include "device.h"

class VulkanSync
{
public:
  void createSyncObjects(const VulkanDevice& device);

  void destroy(const VulkanDevice& device);
  VkSemaphore getImageAvailableSemaphore() const { return imageAvailableSemaphore; }
  VkSemaphore getRenderFinishedSemaphore() const { return renderFinishedSemaphore; }
  VkFence     getInFlightFence()           const { return inFlightFence; }

private:
  VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
  VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
  VkFence     inFlightFence = VK_NULL_HANDLE;
};
