#ifndef __VULKAN_SAMPLER_H__
#define __VULKAN_SAMPLER_H__

#include "vulkan_context.h"

class VulkanSampler : public VulkanResourceOnDevice<VkSampler> {
public:
    using VulkanResourceOnDevice::VulkanResourceOnDevice;
    virtual ~VulkanSampler() override;
    int CreateSampler(VkFilter min_filter,
                      VkFilter mag_filter,
                      VkSamplerAddressMode address_mode,
                      VkBool32 unnorm_pos = VK_FALSE);

    const void* DescriptorInfo() { return reinterpret_cast<void*>(&desc_image_info_); }

private:
    VkDescriptorImageInfo desc_image_info_ = {};
};

#endif