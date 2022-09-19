#include "vulkan_command_buffer.h"

VulkanCommandBuffer::~VulkanCommandBuffer() {
    if (vk_resource_) vkFreeCommandBuffers(device_, cmd_pool_, 1, &vk_resource_);
    if (fence_) vkDestroyFence(device_, fence_, nullptr);
    if (cmd_pool_) vkDestroyCommandPool(device_, cmd_pool_, nullptr);
}

int VulkanCommandBuffer::Create() {
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CALL(vkCreateFence(device_, &fenceInfo, nullptr, &fence_));

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = ctx_->QueueIndex();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CALL(vkCreateCommandPool(device_, &poolInfo, nullptr, &cmd_pool_));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cmd_pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_CALL(vkAllocateCommandBuffers(device_, &allocInfo, &vk_resource_));
    return 0;
}

int VulkanCommandBuffer::Begin() {
    VkCommandBufferBeginInfo cmdBufferBeginInfo {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CALL(vkBeginCommandBuffer(vk_resource_, &cmdBufferBeginInfo));
    return 0;
}

int VulkanCommandBuffer::End() {    
    VK_CALL(vkEndCommandBuffer(vk_resource_));
    return 0;
}

void VulkanCommandBuffer::BindPipeline(const std::shared_ptr<VulkanComputePipeline>& pipeline) {
    CHECK_RETURN_IF(!pipeline, CHECK_VOID);
    pl_ = pipeline;
    vkCmdBindPipeline(vk_resource_, VK_PIPELINE_BIND_POINT_COMPUTE, pl_->VkType());
    vkCmdBindDescriptorSets(vk_resource_, VK_PIPELINE_BIND_POINT_COMPUTE, pl_->pipeline_layout_, 0, 1, &pl_->descriptor_set_, 0, 0);
}

void VulkanCommandBuffer::Dispatch(int x, int y) {
    CHECK_RETURN_IF(x * y == 0, CHECK_VOID);
    CHECK_RETURN_IF(!pl_, CHECK_VOID);
    uint32_t group_count_x = (x + pl_->local_size_x_ - 1) / pl_->local_size_x_;
    uint32_t group_count_y = (y + pl_->local_size_y_ - 1) / pl_->local_size_y_;
    vkCmdDispatch(vk_resource_, group_count_x, group_count_y, 1);
}

int VulkanCommandBuffer::Submit() {
    VkSubmitInfo submitInfo = {};
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vk_resource_;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = 0;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

	vkResetFences(device_, 1, &fence_);
    VK_CALL(vkQueueSubmit(ctx_->Queue(), 1, &submitInfo, fence_));
    return 0;
}

int VulkanCommandBuffer::WaitComplete() {
    VK_CALL(vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX));
    return 0;
}