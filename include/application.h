#pragma once
#include <stdlib.h>
#include <iostream>
#include "instance.h"
#include "GLFW/glfw3.h"

//Application layer that will hold all vulkan related objects and windowing 
//run will kinda be like vulkan tutorial
class VulkanApp 
{
public:
  void run();

private:
  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();

  GLFWwindow* window = nullptr;
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;

  //VULKAN OBJECTS!
  VulkanInstance instance;
  //VulkanDevice device;
  //VulkanSwapchain swapchain;
  //VulkanPipeline pipeline;
  //VulkanCommand command;
  //VulkanSync syncObjects;

};
