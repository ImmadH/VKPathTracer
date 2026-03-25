#pragma once
#include <stdlib.h>
#include <iostream>
#include <vector>
#include "instance.h"
#include "device.h"
#include "swapchain.h"
#include "renderpass.h"
#include "pipeline.h"
#include "commands.h"
#include "sync.h"
#include "camera.h"
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
  void recreateSwapChain();
  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

  void drawFrame();


private:
  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();
  void createCameraBuffers();
  void destroyCameraBuffers();
  void createAccumulationResources();
  void destroyAccumulationResources();
  void createDescriptorPool();
  void createDescriptorSets();
  void destroyDescriptorPool();
  void updateComputeDescriptorSet(uint32_t frameSlot);
  void updateDisplayDescriptorSet(uint32_t frameSlot);
  void updateCameraBuffer(uint32_t frameSlot);
  bool updateCameraFromInput(float deltaTimeSeconds);
  void createBuffer(VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkBuffer& buffer,
                    VmaAllocation& allocation,
                    void** mappedData);
  void createAccumulationImage(const VkExtent2D& extent,
                               VkImage& image,
                               VmaAllocation& allocation,
                               VkImageView& imageView);
  void transitionAccumulationImagesToGeneral();

  GLFWwindow* window = nullptr;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  const uint32_t WIDTH = 800;
  const uint32_t HEIGHT = 600;
  const VkFormat ACCUMULATION_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
  uint32_t currentFrame = 0;
  uint32_t frameCounter = 0;
  bool framebufferResized = false;
  Camera camera;
  float cameraMoveSpeed = 4.0f;
  float mouseSensitivity = 0.10f;
  double lastMouseX = 0.0;
  double lastMouseY = 0.0;
  bool firstMouseSample = true;

  std::vector<VkFramebuffer> swapChainFramebuffers;
  std::vector<VkBuffer> cameraBuffers;
  std::vector<VmaAllocation> cameraBufferAllocations;
  std::vector<void*> cameraBufferMapped;
  std::vector<VkImage> accumulationImages;
  std::vector<VmaAllocation> accumulationImageAllocations;
  std::vector<VkImageView> accumulationImageViews;
  std::vector<VkDescriptorSet> computeDescriptorSets;
  std::vector<VkDescriptorSet> displayDescriptorSets;
  std::vector<VkFence> imagesInFlight;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  uint32_t accumulationReadIndex = 0;
  uint32_t accumulationWriteIndex = 1;

  //VULKAN OBJECTS!
  VulkanInstance instance;
  VulkanDevice device;
  VulkanSwapchain swapchain;
  VulkanRenderPass renderPass;
  VulkanPipeline pipeline;
  VulkanCommands commands;
  VulkanSync sync;

};
