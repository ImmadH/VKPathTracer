#include "commands.h"
#include <stdexcept>
#include "pipeline.h"
#include <iostream>
void VulkanCommands::createCommandPool(const VulkanDevice& device)
{
  const auto& qfi = device.getQueueFamilyIndices();

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = qfi.graphicsFamily.value();
  poolInfo.flags  = 0;

  if (vkCreateCommandPool(device.getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    throw std::runtime_error("failed to create command pool!");

  std::cout << "Created Command pool\n";
}

void VulkanCommands::createCommandBuffers(const VulkanDevice& device,
                                          const VulkanSwapchain& swapchain,
                                          const VulkanRenderPass& renderPass,
                                          const VulkanPipeline& pipeline,
                                          const std::vector<VkFramebuffer>& framebuffers)
{
  commandBuffers.resize(framebuffers.size());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = commandPool;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("failed to allocate command buffers!");

  for (uint32_t i = 0; i < commandBuffers.size(); ++i)
  {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
      throw std::runtime_error("failed to begin recording command buffer!");

    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass        = renderPass.getRenderPass();
    rpBegin.framebuffer       = framebuffers[i];
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent = swapchain.getExtent();

    VkClearValue clearColor = { {0.f, 0.f, 0.f, 1.f} };
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues    = &clearColor;

    vkCmdBeginRenderPass(commandBuffers[i], &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getPipeline());

    VkViewport viewport{};
    viewport.x  = 0.0f;
    viewport.y  = 0.0f;
    viewport.width = static_cast<float>(swapchain.getExtent().width);
    viewport.height = static_cast<float>(swapchain.getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain.getExtent();
    vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

    vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffers[i]);

    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("failed to record command buffer!");
  }
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
