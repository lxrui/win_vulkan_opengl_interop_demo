#ifndef __VULKAN_COMMAND_BUFFER_H__
#define __VULKAN_COMMAND_BUFFER_H__

#include "vulkan_context.h"
#include "vulkan_compute_pipeline.h"

class VulkanCommandBuffer : public VulkanResourceOnDevice<VkCommandBuffer> {
public:
    using VulkanResourceOnDevice::VulkanResourceOnDevice;
    virtual ~VulkanCommandBuffer() override;
    int Create();
    int Begin();
    int End();
    void BindPipeline(const std::shared_ptr<VulkanComputePipeline>& pipeline);
    void Dispatch(int x, int y);
    int Submit();
    int WaitComplete();
private:
    VkCommandPool cmd_pool_ = nullptr;
    VkFence fence_ = nullptr;
    std::shared_ptr<VulkanComputePipeline> pl_;
};

#endif