#include "commands.h"
#include <cstdint>
#include <stdexcept>
#include "pipeline.h"
#include <iostream>
#include "sync.h"

void VulkanCommands::createCommandPool(const VulkanDevice& device)
{
  const auto& qfi = device.getQueueFamilyIndices();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = qfi.graphicsFamily.value();
  poolInfo.flags  = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device.getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    throw std::runtime_error("failed to create command pool!");

  std::cout << "Created Command pool\n";
}

void VulkanCommands::createCommandBuffers(const VulkanDevice& device, uint32_t count)
{
  commandBuffers.resize(count);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool   = commandPool;
  allocInfo.level   = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

  if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");
}

void VulkanCommands::recordCommandBuffer(uint32_t imageIndex,
                                         const VulkanSwapchain& swapchain,
                                         const VulkanRenderPass& renderPass,
                                         const VulkanPipeline& pipeline,
                                         VkFramebuffer framebuffer,
                                         VkDescriptorSet computeDescriptorSet,
                                         VkDescriptorSet displayDescriptorSet,
                                         VkImage accumulationReadImage,
                                         VkImage accumulationWriteImage)
{
  if (vkResetCommandBuffer(commandBuffers[imageIndex], 0) != VK_SUCCESS)
    throw std::runtime_error("failed to reset command buffer!");

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("failed to begin recording command buffer!");

  VkImageMemoryBarrier accumulationBarriers[2]{};
  VkImage accumulationImages[2] = { accumulationReadImage, accumulationWriteImage };
  for (int i = 0; i < 2; ++i)
  {
    accumulationBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    accumulationBarriers[i].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    accumulationBarriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
    accumulationBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    accumulationBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    accumulationBarriers[i].image = accumulationImages[i];
    accumulationBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    accumulationBarriers[i].subresourceRange.baseMipLevel = 0;
    accumulationBarriers[i].subresourceRange.levelCount = 1;
    accumulationBarriers[i].subresourceRange.baseArrayLayer = 0;
    accumulationBarriers[i].subresourceRange.layerCount = 1;
    accumulationBarriers[i].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    accumulationBarriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  }

  vkCmdPipelineBarrier(commandBuffers[imageIndex],
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       2,
                       accumulationBarriers);

  vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.getComputePipeline());
  vkCmdBindDescriptorSets(commandBuffers[imageIndex],
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipeline.getComputeLayout(),
                          0,
                          1,
                          &computeDescriptorSet,
                          0,
                          nullptr);

  const uint32_t workgroupSize = 8;
  const uint32_t groupCountX = (swapchain.getExtent().width + workgroupSize - 1) / workgroupSize;
  const uint32_t groupCountY = (swapchain.getExtent().height + workgroupSize - 1) / workgroupSize;
  vkCmdDispatch(commandBuffers[imageIndex], groupCountX, groupCountY, 1);

  VkImageMemoryBarrier displayBarrier{};
  displayBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  displayBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
  displayBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  displayBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  displayBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  displayBarrier.image = accumulationWriteImage;
  displayBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  displayBarrier.subresourceRange.baseMipLevel = 0;
  displayBarrier.subresourceRange.levelCount = 1;
  displayBarrier.subresourceRange.baseArrayLayer = 0;
  displayBarrier.subresourceRange.layerCount = 1;
  displayBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  displayBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffers[imageIndex],
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &displayBarrier);

  VkRenderPassBeginInfo rpBegin{};
  rpBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpBegin.renderPass        = renderPass.getRenderPass();
  rpBegin.framebuffer       = framebuffer;
  rpBegin.renderArea.offset = {0, 0};
  rpBegin.renderArea.extent = swapchain.getExtent();

  VkClearValue clearColor = { {0.f, 0.f, 0.f, 1.f} };
  rpBegin.clearValueCount = 1;
  rpBegin.pClearValues    = &clearColor;

  vkCmdBeginRenderPass(commandBuffers[imageIndex], &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getDisplayPipeline());
  vkCmdBindDescriptorSets(commandBuffers[imageIndex],
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.getDisplayLayout(),
                          0,
                          1,
                          &displayDescriptorSet,
                          0,
                          nullptr);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapchain.getExtent().width);
  viewport.height = static_cast<float>(swapchain.getExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapchain.getExtent();
  vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

  vkCmdDraw(commandBuffers[imageIndex], 3, 1, 0, 0);
  vkCmdEndRenderPass(commandBuffers[imageIndex]);

  if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
    throw std::runtime_error("failed to record command buffer!");
}

void VulkanCommands::destroy(const VulkanDevice& device)
{
  if (commandPool) {
    vkDestroyCommandPool(device.getDevice(), commandPool, nullptr);
    commandPool = VK_NULL_HANDLE;
  }
  commandBuffers.clear();
  std::cerr << "Command Pool Destroyed\n";
}
