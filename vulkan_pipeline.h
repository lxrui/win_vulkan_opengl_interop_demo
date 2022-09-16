#ifndef __VULKAN_PIPELINE_H__
#define __VULKAN_PIPELINE_H__

#include "vulkan_context.h"

class VulkanPipeline : public VulkanResourceOnDevice {
public:
    virtual ~VulkanPipeline() override;
    int CreateComputePipeline();
private:
};

#endif