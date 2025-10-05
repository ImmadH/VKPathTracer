#pragma once
#include "instance.h"
#include <vulkan/vulkan_core.h>
class VulkanDevice 
{
public:
  void create(VulkanInstance& instance, VkSurfaceKHR surface);
  void destroy();

private:
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;  
  VkDevice device = VK_NULL_HANDLE;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
};
