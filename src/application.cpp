#include "application.h"
#include <cstdint>
#include <iostream>
#include <vulkan/vulkan_core.h>

void VulkanApp::run()
{
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}


void VulkanApp::initWindow() 
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(WIDTH, HEIGHT, "VKRenderer - A Vulkan 1.3 Renderer", nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanApp::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
  auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

//Here we will call the creation of vulkan objects
void VulkanApp::initVulkan()
{
  instance.create(enableValidation);

  //Window Surface Creation 
  if (glfwCreateWindowSurface(instance.getInstance(), window, nullptr, &surface) != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  std::cout << "Created Window Surface\n";

  device.create(instance, surface);
  swapchain.createSwapChain(device, surface, window);
  swapchain.createImageViews(device);
  renderPass.createRenderPass(device, swapchain.getImageFormat());
  pipeline.createGraphicsPipeline(device, swapchain, renderPass);
  createFrameBuffers();

  commands.createCommandPool(device);
  commands.createCommandBuffers(device, swapchain, renderPass, pipeline, swapChainFramebuffers);
  sync.createSyncObjects(device);
  
}

void VulkanApp::createFrameBuffers()
{
  const auto& views  = swapchain.getImageViews();
  const auto extent = swapchain.getExtent();

  swapChainFramebuffers.resize(views.size());

  for (size_t i = 0; i < views.size(); ++i)
  {
      VkImageView attachments[] = { views[i] };

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType     = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass  = renderPass.getRenderPass();
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = extent.width;
      framebufferInfo.height = extent.height;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(device.getDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("failed to create framebuffer!");
      }
  }
}

void VulkanApp::destroyFrameBuffers()
{
    for (auto frameBuffers : swapChainFramebuffers)
    {
      if (frameBuffers) vkDestroyFramebuffer(device.getDevice(), frameBuffers, nullptr);
    }
    swapChainFramebuffers.clear();
}

void VulkanApp::recreateSwapChain()
{
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
      glfwGetFramebufferSize(window, &width, &height);
      glfwWaitEvents();
  }
  vkDeviceWaitIdle(device.getDevice());
  destroyFrameBuffers();
  swapchain.cleanSwapChain(device);
  swapchain.createSwapChain(device, surface, window);
  swapchain.createImageViews(device);
  createFrameBuffers();
  commands.destroy(device);                // or free/reset CBs
  commands.createCommandPool(device);      // with RESET flag
  commands.createCommandBuffers(device, swapchain, renderPass, pipeline, swapChainFramebuffers);
}


void VulkanApp::mainLoop()
{
  while (!glfwWindowShouldClose(window)) 
  {
    glfwPollEvents();
    drawFrame();
  }
  vkDeviceWaitIdle(device.getDevice());
}

void VulkanApp::drawFrame()
{
  VkFence frameFence = sync.getInFlightFence()[currentFrame];
  vkWaitForFences(device.getDevice(), 1, &frameFence, VK_TRUE, UINT64_MAX);
  vkResetFences(device.getDevice(), 1, &frameFence);

  uint32_t imageIndex = 0;
  VkResult result = vkAcquireNextImageKHR(device.getDevice(),  swapchain.getSwapChain(), UINT64_MAX, sync.getImageAvailableSemaphore()[currentFrame], VK_NULL_HANDLE, &imageIndex); 
  if (result == VK_ERROR_OUT_OF_DATE_KHR) 
  {
    framebufferResized = false;
    recreateSwapChain();
    return;
  } 
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
  {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  VkSemaphore waitSemaphores[]  = {sync.imageAvailable(currentFrame)};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {sync.renderFinished(currentFrame)};

  const auto& cbs = commands.getCommandBuffers();

  VkSubmitInfo submitInfo{};
  submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount   = 1;
  submitInfo.pWaitSemaphores      = waitSemaphores;
  submitInfo.pWaitDstStageMask    = waitStages;
  submitInfo.commandBufferCount   = 1;
  submitInfo.pCommandBuffers      = &cbs[imageIndex];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, frameFence) != VK_SUCCESS)
    throw std::runtime_error("failed to submit draw command buffer!");

  VkSemaphore presentWaitSemaphores[] = { sync.renderFinished(currentFrame) };

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;
  VkSwapchainKHR scs[]           = { swapchain.getSwapChain() };
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = scs;
  presentInfo.pImageIndices      = &imageIndex;

  result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
  {
    framebufferResized = false;
    recreateSwapChain();
  } 
  else if (result != VK_SUCCESS) 
  {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


//destroy all related vulkan objects
void VulkanApp::cleanup()
{
  std::cout << " " << '\n';
  destroyFrameBuffers();
  sync.destroy(device);
  commands.destroy(device);
  pipeline.destroy(device);
  renderPass.destroy(device);
  swapchain.destroy(device);
  device.destroy();
  vkDestroySurfaceKHR(instance.getInstance(), surface, nullptr);
  std::cerr << "Window Surface Destroyed\n";
  instance.destroy();
  glfwDestroyWindow(window);
  glfwTerminate();
}

