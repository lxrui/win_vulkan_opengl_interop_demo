#ifndef __VULKAN_COMMAND_BUFFER_H__
#define __VULKAN_COMMAND_BUFFER_H__

#include "vulkan_context.h"
#include "vulkan_compute_pipeline.h"
#include "vulkan_semaphore.h"

class VulkanCommandBuffer : public VulkanResourceOnDevice<VkCommandBuffer> {
public:
    using VulkanResourceOnDevice::VulkanResourceOnDevice;
    virtual ~VulkanCommandBuffer() override;
    int Create();
    int Begin();
    int End();
    void BindPipeline(const std::shared_ptr<VulkanComputePipeline>& pipeline);
    void Dispatch(uint32_t x, uint32_t y);
    /**
     * @brief 调用vkQueueSubmit提交命令时，VkSubmitInfo结构需要填入pWaitSemaphores、pWaitDstStageMask。
     * 表示此次提交的命令在执行到pWaitDstStageMask时，要停下，
     * 必须要等待pWaitSemaphores所指向的Semaphore的状态变成signaled时才可以继续执行
     * @param semaphore 要等待的Semaphore
     * @param pipeline_stage command buffer中，需要在semaphore signal后才能继续往下执行的pipeline stage
     */
    void AddWaitSemaphore(const std::shared_ptr<VulkanSemaphore>& semaphore, const VkPipelineStageFlags pipeline_stage) {
        sp_wait_semaphores_.push_back(semaphore);
        vk_wait_semaphores_.emplace_back(semaphore->VkType());
        wait_pipeline_stages_.push_back(pipeline_stage);
    }
    void AddSignalSemaphore(const std::shared_ptr<VulkanSemaphore>& semaphore) {
        sp_signal_semaphores_.push_back(semaphore);
        vk_signal_semaphores_.emplace_back(semaphore->VkType());
    }
    int Submit();
    int WaitComplete();
private:
    VkCommandPool cmd_pool_ = nullptr;
    VkFence fence_ = nullptr;
    std::vector<std::shared_ptr<VulkanComputePipeline>> pipelines_;
    std::vector<std::shared_ptr<VulkanSemaphore>> sp_wait_semaphores_;
    std::vector<std::shared_ptr<VulkanSemaphore>> sp_signal_semaphores_;
    std::vector<VkPipelineStageFlags> wait_pipeline_stages_;
    std::vector<VkSemaphore> vk_wait_semaphores_;
    std::vector<VkSemaphore> vk_signal_semaphores_;
};

#endif