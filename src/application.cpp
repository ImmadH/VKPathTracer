#include "application.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>
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
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  if (glfwRawMouseMotionSupported())
  {
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  }
  glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
}

void VulkanApp::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
  (void)width;
  (void)height;
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
  pipeline.createDescriptorSetLayouts(device);
  pipeline.createComputePipeline(device);
  pipeline.createDisplayPipeline(device, renderPass);
  createFrameBuffers();

  commands.createCommandPool(device);
  createAccumulationResources();
  transitionAccumulationImagesToGeneral();
  commands.createCommandBuffers(device, static_cast<uint32_t>(swapChainFramebuffers.size()));
  createCameraBuffers();
  createDescriptorPool();
  createDescriptorSets();
  sync.createSyncObjects(device, static_cast<uint32_t>(swapchain.getImages().size()));
  imagesInFlight.resize(swapchain.getImages().size(), VK_NULL_HANDLE);
  
}

void VulkanApp::createBuffer(VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VkBuffer& buffer,
                             VmaAllocation& allocation,
                             void** mappedData)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocationInfo allocationInfo{};
  if (vmaCreateBuffer(device.getAllocator(),
                      &bufferInfo,
                      &allocInfo,
                      &buffer,
                      &allocation,
                      &allocationInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create VMA buffer!");
  }

  *mappedData = allocationInfo.pMappedData;
}

void VulkanApp::createCameraBuffers()
{
  const VkDeviceSize bufferSize = sizeof(CameraUBO);

  cameraBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  cameraBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);
  cameraBufferMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 cameraBuffers[i],
                 cameraBufferAllocations[i],
                 &cameraBufferMapped[i]);
  }
}

void VulkanApp::destroyCameraBuffers()
{
  for (size_t i = 0; i < cameraBuffers.size(); ++i)
  {
    if (cameraBuffers[i] != VK_NULL_HANDLE && cameraBufferAllocations[i] != nullptr)
    {
      vmaDestroyBuffer(device.getAllocator(), cameraBuffers[i], cameraBufferAllocations[i]);
    }
    cameraBufferMapped[i] = nullptr;
  }

  cameraBuffers.clear();
  cameraBufferAllocations.clear();
  cameraBufferMapped.clear();
}

void VulkanApp::createAccumulationImage(const VkExtent2D& extent,
                                        VkImage& image,
                                        VmaAllocation& allocation,
                                        VkImageView& imageView)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = extent.width;
  imageInfo.extent.height = extent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = ACCUMULATION_FORMAT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocationInfo{};
  allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

  if (vmaCreateImage(device.getAllocator(), &imageInfo, &allocationInfo, &image, &allocation, nullptr) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create accumulation image!");
  }

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = ACCUMULATION_FORMAT;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
  {
    vmaDestroyImage(device.getAllocator(), image, allocation);
    image = VK_NULL_HANDLE;
    allocation = nullptr;
    throw std::runtime_error("failed to create accumulation image view!");
  }
}

void VulkanApp::createAccumulationResources()
{
  accumulationImages.resize(2, VK_NULL_HANDLE);
  accumulationImageAllocations.resize(2, nullptr);
  accumulationImageViews.resize(2, VK_NULL_HANDLE);

  const VkExtent2D extent = swapchain.getExtent();
  for (size_t i = 0; i < accumulationImages.size(); ++i)
  {
    createAccumulationImage(extent,
                            accumulationImages[i],
                            accumulationImageAllocations[i],
                            accumulationImageViews[i]);
  }

  accumulationReadIndex = 0;
  accumulationWriteIndex = 1;
}

void VulkanApp::destroyAccumulationResources()
{
  for (size_t i = 0; i < accumulationImages.size(); ++i)
  {
    if (accumulationImageViews[i] != VK_NULL_HANDLE)
    {
      vkDestroyImageView(device.getDevice(), accumulationImageViews[i], nullptr);
      accumulationImageViews[i] = VK_NULL_HANDLE;
    }
    if (accumulationImages[i] != VK_NULL_HANDLE && accumulationImageAllocations[i] != nullptr)
    {
      vmaDestroyImage(device.getAllocator(), accumulationImages[i], accumulationImageAllocations[i]);
      accumulationImages[i] = VK_NULL_HANDLE;
      accumulationImageAllocations[i] = nullptr;
    }
  }

  accumulationImages.clear();
  accumulationImageAllocations.clear();
  accumulationImageViews.clear();
}

void VulkanApp::transitionAccumulationImagesToGeneral()
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commands.getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate transition command buffer!");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to begin transition command buffer!");
  }

  std::array<VkImageMemoryBarrier, 2> barriers{};
  for (size_t i = 0; i < accumulationImages.size(); ++i)
  {
    barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[i].image = accumulationImages[i];
    barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[i].subresourceRange.baseMipLevel = 0;
    barriers[i].subresourceRange.levelCount = 1;
    barriers[i].subresourceRange.baseArrayLayer = 0;
    barriers[i].subresourceRange.layerCount = 1;
    barriers[i].srcAccessMask = 0;
    barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  }

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       static_cast<uint32_t>(barriers.size()),
                       barriers.data());

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to end transition command buffer!");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to submit accumulation transition commands!");
  }

  vkQueueWaitIdle(device.getGraphicsQueue());
  vkFreeCommandBuffers(device.getDevice(), commands.getCommandPool(), 1, &commandBuffer);
}

void VulkanApp::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  poolSizes[1].descriptorCount = 3 * MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = 2 * MAX_FRAMES_IN_FLIGHT;

  if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void VulkanApp::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> computeLayouts(MAX_FRAMES_IN_FLIGHT, pipeline.getComputeDescriptorSetLayout());
  VkDescriptorSetAllocateInfo computeAllocInfo{};
  computeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  computeAllocInfo.descriptorPool = descriptorPool;
  computeAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
  computeAllocInfo.pSetLayouts = computeLayouts.data();

  computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device.getDevice(), &computeAllocInfo, computeDescriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate compute descriptor sets!");
  }

  std::vector<VkDescriptorSetLayout> displayLayouts(MAX_FRAMES_IN_FLIGHT, pipeline.getDisplayDescriptorSetLayout());
  VkDescriptorSetAllocateInfo displayAllocInfo{};
  displayAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  displayAllocInfo.descriptorPool = descriptorPool;
  displayAllocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
  displayAllocInfo.pSetLayouts = displayLayouts.data();

  displayDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device.getDevice(), &displayAllocInfo, displayDescriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate display descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    updateComputeDescriptorSet(static_cast<uint32_t>(i));
    updateDisplayDescriptorSet(static_cast<uint32_t>(i));
  }
}

void VulkanApp::destroyDescriptorPool()
{
  if (descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device.getDevice(), descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;
  }
  computeDescriptorSets.clear();
  displayDescriptorSets.clear();
}

void VulkanApp::updateComputeDescriptorSet(uint32_t frameSlot)
{
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = cameraBuffers[frameSlot];
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(CameraUBO);

  VkDescriptorImageInfo accumulationReadInfo{};
  accumulationReadInfo.imageView = accumulationImageViews[accumulationReadIndex];
  accumulationReadInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkDescriptorImageInfo accumulationWriteInfo{};
  accumulationWriteInfo.imageView = accumulationImageViews[accumulationWriteIndex];
  accumulationWriteInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet = computeDescriptorSets[frameSlot];
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo = &bufferInfo;

  descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet = computeDescriptorSets[frameSlot];
  descriptorWrites[1].dstBinding = 1;
  descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pImageInfo = &accumulationReadInfo;

  descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[2].dstSet = computeDescriptorSets[frameSlot];
  descriptorWrites[2].dstBinding = 2;
  descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrites[2].descriptorCount = 1;
  descriptorWrites[2].pImageInfo = &accumulationWriteInfo;

  vkUpdateDescriptorSets(device.getDevice(),
                         static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(),
                         0,
                         nullptr);
}

void VulkanApp::updateDisplayDescriptorSet(uint32_t frameSlot)
{
  VkDescriptorImageInfo accumulationInfo{};
  accumulationInfo.imageView = accumulationImageViews[accumulationWriteIndex];
  accumulationInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = displayDescriptorSets[frameSlot];
  descriptorWrite.dstBinding = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &accumulationInfo;

  vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanApp::updateCameraBuffer(uint32_t frameSlot)
{
  const VkExtent2D extent = swapchain.getExtent();
  const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
  const CameraUBO cameraUbo = camera.buildUBO(aspect, frameCounter);
  std::memcpy(cameraBufferMapped[frameSlot], &cameraUbo, sizeof(cameraUbo));
}

bool VulkanApp::updateCameraFromInput(float deltaTimeSeconds)
{
  bool moved = false;
  const float moveStep = cameraMoveSpeed * deltaTimeSeconds;
  glm::vec3 localDelta(0.0f);

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    localDelta.z += moveStep;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    localDelta.z -= moveStep;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    localDelta.x -= moveStep;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    localDelta.x += moveStep;
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
  {
    localDelta.y += moveStep;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
  {
    localDelta.y -= moveStep;
  }

  if (localDelta != glm::vec3(0.0f))
  {
    camera.moveLocal(localDelta);
    moved = true;
  }

  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(window, &mouseX, &mouseY);
  if (firstMouseSample)
  {
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    firstMouseSample = false;
  }

  const float deltaX = static_cast<float>(mouseX - lastMouseX);
  const float deltaY = static_cast<float>(lastMouseY - mouseY);
  lastMouseX = mouseX;
  lastMouseY = mouseY;

  const float yawDelta = deltaX * mouseSensitivity;
  const float pitchDelta = deltaY * mouseSensitivity;
  if (yawDelta != 0.0f || pitchDelta != 0.0f)
  {
    camera.rotate(yawDelta, pitchDelta);
    moved = true;
  }

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }

  return moved;
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
  destroyAccumulationResources();
  sync.destroy(device);
  swapchain.cleanSwapChain(device);
  swapchain.createSwapChain(device, surface, window);
  swapchain.createImageViews(device);
  createFrameBuffers();
  commands.destroy(device);                // or free/reset CBs
  commands.createCommandPool(device);      // with RESET flag
  createAccumulationResources();
  transitionAccumulationImagesToGeneral();
  commands.createCommandBuffers(device, static_cast<uint32_t>(swapChainFramebuffers.size()));
  sync.createSyncObjects(device, static_cast<uint32_t>(swapchain.getImages().size()));
  imagesInFlight.assign(swapchain.getImages().size(), VK_NULL_HANDLE);
  currentFrame = 0;
  frameCounter = 0;
  accumulationReadIndex = 0;
  accumulationWriteIndex = 1;
}


void VulkanApp::mainLoop()
{
  double lastFrameTime = glfwGetTime();
  while (!glfwWindowShouldClose(window)) 
  {
    glfwPollEvents();
    const double currentTime = glfwGetTime();
    const float deltaTimeSeconds = static_cast<float>(currentTime - lastFrameTime);
    lastFrameTime = currentTime;

    if (updateCameraFromInput(deltaTimeSeconds))
    {
      frameCounter = 0;
    }

    drawFrame();
  }
  vkDeviceWaitIdle(device.getDevice());
}

void VulkanApp::drawFrame()
{
  VkFence frameFence = sync.getInFlightFence()[currentFrame];
  vkWaitForFences(device.getDevice(), 1, &frameFence, VK_TRUE, UINT64_MAX);
  if (frameCounter > 0 && MAX_FRAMES_IN_FLIGHT > 1)
  {
    const uint32_t previousFrame = (currentFrame + MAX_FRAMES_IN_FLIGHT - 1) % MAX_FRAMES_IN_FLIGHT;
    VkFence previousFence = sync.getInFlightFence()[previousFrame];
    vkWaitForFences(device.getDevice(), 1, &previousFence, VK_TRUE, UINT64_MAX);
  }

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

  if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
  {
    vkWaitForFences(device.getDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
  }
  imagesInFlight[imageIndex] = frameFence;

  updateCameraBuffer(currentFrame);
  updateComputeDescriptorSet(currentFrame);
  updateDisplayDescriptorSet(currentFrame);
  commands.recordCommandBuffer(imageIndex,
                               swapchain,
                               renderPass,
                               pipeline,
                               swapChainFramebuffers[imageIndex],
                               computeDescriptorSets[currentFrame],
                               displayDescriptorSets[currentFrame],
                               accumulationImages[accumulationReadIndex],
                               accumulationImages[accumulationWriteIndex]);

  vkResetFences(device.getDevice(), 1, &frameFence);

  VkSemaphore waitSemaphores[]  = {sync.imageAvailable(currentFrame)};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {sync.renderFinished(imageIndex)};

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

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;
  VkSwapchainKHR scs[]           = { swapchain.getSwapChain() };
  presentInfo.swapchainCount     = 1;
  presentInfo.pSwapchains        = scs;
  presentInfo.pImageIndices      = &imageIndex;

  result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) 
  {
    framebufferResized = false;
    recreateSwapChain();
    return;
  } 
  else if (result != VK_SUCCESS) 
  {
    throw std::runtime_error("failed to present swap chain image!");
  }

  ++frameCounter;
  std::swap(accumulationReadIndex, accumulationWriteIndex);
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}


//destroy all related vulkan objects
void VulkanApp::cleanup()
{
  std::cout << " " << '\n';
  destroyFrameBuffers();
  sync.destroy(device);
  commands.destroy(device);
  destroyDescriptorPool();
  destroyCameraBuffers();
  destroyAccumulationResources();
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

