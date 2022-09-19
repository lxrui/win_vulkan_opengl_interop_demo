#include "vulkan_sampler.h"

VulkanSampler::~VulkanSampler() {
    if (vk_resource_ != VK_NULL_HANDLE) vkDestroySampler(device_, vk_resource_, nullptr);
}

int VulkanSampler::CreateSampler(VkFilter min_filter, VkFilter mag_filter, VkSamplerAddressMode address_mode, VkBool32 unnorm_pos) {
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = min_filter;
    sampler_info.magFilter = mag_filter;
    sampler_info.addressModeU = address_mode;
    sampler_info.addressModeV = address_mode;
    sampler_info.addressModeW = address_mode;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = unnorm_pos;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VK_CALL(vkCreateSampler(device_, &sampler_info, nullptr, &vk_resource_));
    desc_image_info_.sampler = vk_resource_;
    return 0;
}