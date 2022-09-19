#ifndef __VULKAN_SHADER_H__
#define __VULKAN_SHADER_H__

#include "vulkan_context.h"

class VulkanShader : public VulkanResourceOnDevice<VkShaderModule> {
public:
    using VulkanResourceOnDevice::VulkanResourceOnDevice;
    virtual ~VulkanShader() override;
    int CreateShader(const uint8_t* spv_code, size_t spv_size);
};


#endif //#ifndef __VULKAN_SHADER_H__