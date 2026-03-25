#include "sync.h"
#include <stdexcept>
#include <iostream>
void VulkanSync::createSyncObjects(const VulkanDevice& device, uint32_t swapchainImageCount)
{

  imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphore.resize(swapchainImageCount);
  inFlightFence.resize(MAX_FRAMES_IN_FLIGHT);
  
  VkSemaphoreCreateInfo semInfo{};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 

  for (size_t i = 0; i < imageAvailableSemaphore.size(); i++) 
  {
    if (vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
        vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &inFlightFence[i]) != VK_SUCCESS) 
    {
      throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
  }

  for (size_t i = 0; i < renderFinishedSemaphore.size(); i++)
  {
    if (vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create render-finished semaphore for a swapchain image!");
    }
  }

  std::cout << "Created Synchronization primitives\n";
}

void VulkanSync::destroy(const VulkanDevice& device)
{
  for (VkSemaphore semaphore : renderFinishedSemaphore)
  {
    vkDestroySemaphore(device.getDevice(), semaphore, nullptr);
  }

  for (VkSemaphore semaphore : imageAvailableSemaphore)
  {
    vkDestroySemaphore(device.getDevice(), semaphore, nullptr);
  }

  for (VkFence fence : inFlightFence)
  {
    vkDestroyFence(device.getDevice(), fence, nullptr);
  }

  renderFinishedSemaphore.clear();
  imageAvailableSemaphore.clear();
  inFlightFence.clear();
  std::cerr << "Synchronization Primitives Destroyed\n";

}
