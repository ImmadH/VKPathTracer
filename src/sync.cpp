#include "sync.h"
#include <stdexcept>
#include <iostream>
void VulkanSync::createSyncObjects(const VulkanDevice& device)
{
  VkSemaphoreCreateInfo semInfo{ };
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{ };
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 

  if (vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(device.getDevice(), &semInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
      vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create synchronization objects!");
  }
  std::cout << "Created Synchronization primitives\n";
}

void VulkanSync::destroy(const VulkanDevice& device)
{
  if (renderFinishedSemaphore) {
    vkDestroySemaphore(device.getDevice(), renderFinishedSemaphore, nullptr);
    renderFinishedSemaphore = VK_NULL_HANDLE;
  }
  if (imageAvailableSemaphore) {
    vkDestroySemaphore(device.getDevice(), imageAvailableSemaphore, nullptr);
    imageAvailableSemaphore = VK_NULL_HANDLE;
  }
  if (inFlightFence) {
    vkDestroyFence(device.getDevice(), inFlightFence, nullptr);
    inFlightFence = VK_NULL_HANDLE;
  }
  std::cerr << "Synchronization Primitives Destroyed\n";
}
