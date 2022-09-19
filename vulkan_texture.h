#ifndef __VULKAN_TEXTURE_H__
#define __VULKAN_TEXTURE_H__

#include "vulkan_context.h"

class VulkanTexture : public VulkanResourceOnDevice<VkImage> {
public:
    using VulkanResourceOnDevice::VulkanResourceOnDevice;
    virtual ~VulkanTexture() override;
    int CreateWinGlSharedTexture(VkFormat format,
                                 uint32_t width,
                                 uint32_t height,
                                 VkImageUsageFlags usage,
                                 VkImageTiling tiling,
                                 VkMemoryPropertyFlags mem_flag,
                                 VkImageLayout init_layout,
                                 HANDLE &win32_ext_mem_handle,
                                 uint64_t &memory_size);

    uint32_t Width() { return tex_w_; }
    uint32_t Height() { return tex_h_; }
    const void* DescriptorInfo() { return reinterpret_cast<void*>(&desc_image_info_); }

private:
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView image_view_ = VK_NULL_HANDLE;
    VkDescriptorImageInfo desc_image_info_ = {};
    uint32_t tex_w_ = 0;
    uint32_t tex_h_ = 0;
    uint64_t mem_size_ = 0;
    HANDLE win32_ext_mem_handle_ = nullptr;
};

#endif