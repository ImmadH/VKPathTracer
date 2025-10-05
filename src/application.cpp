#include "application.h"
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
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "VKRenderer - A Vulkan 1.3 Renderer", nullptr, nullptr);
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
}

void VulkanApp::mainLoop()
{
  while (!glfwWindowShouldClose(window)) 
  {
    glfwPollEvents();
  }
}

//destroy all related vulkan objects
void VulkanApp::cleanup()
{
  std::cout << " " << '\n';
  device.destroy();
  vkDestroySurfaceKHR(instance.getInstance(), surface, nullptr);
  instance.destroy();
  glfwDestroyWindow(window);
  glfwTerminate();
}

