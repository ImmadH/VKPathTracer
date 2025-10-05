#include "application.h"
#include "instance.h"

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
  window = glfwCreateWindow(WIDTH, HEIGHT, "VKRenderer - A Vulkan 1.0 Renderer", nullptr, nullptr);
}

//Here we will call the creation of vulkan objects
void VulkanApp::initVulkan()
{
  instance.create(enableValidation);
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
  instance.destroy();
  glfwDestroyWindow(window);
  glfwTerminate();
}

