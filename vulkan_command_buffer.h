#ifndef __VULKAN_COMMAND_BUFFER_H__
#define __VULKAN_COMMAND_BUFFER_H__

#include "vulkan_context.h"

class VulkanCommandBuffer : public VulkanResourceOnDevice {
public:
    virtual ~VulkanCommandBuffer() override;
    int Create();
    VkCommandBuffer CmdBuf() { return cmd_buf_; }
private:
    VkCommandPool cmd_pool_ = nullptr;
    VkFence fence_ = nullptr;
    VkCommandBuffer cmd_buf_ = nullptr;
};

#endif