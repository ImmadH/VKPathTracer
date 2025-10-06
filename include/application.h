#pragma once
#include <stdlib.h>
#include <iostream>
#include "instance.h"
#include "device.h"
#include "swapchain.h"
#include "renderpass.h"
#include "pipeline.h"
#include "commands.h"
#include "sync.h"
#include "GLFW/glfw3.h"
#include <vulkan/vulkan_core.h>

//Application layer that will hold all vulkan related objects and windowing 
//run will kinda be like vulkan tutorial
class VulkanApp 
{
public:
  void run();
  void createFrameBuffers();
  void destroyFrameBuffers();

  void drawFrame();


private:
  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();

  GLFWwindow* window = nullptr;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  std::vector<VkFramebuffer> swapChainFramebuffers;

  //VULKAN OBJECTS!
  VulkanInstance instance;
  VulkanDevice device;
  VulkanSwapchain swapchain;
  VulkanRenderPass renderPass;
  VulkanPipeline pipeline;
  VulkanCommands commands;
  VulkanSync sync;

};
