#include "vulkan_texture.h"

VulkanTexture::~VulkanTexture() {
    if (image_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, image_view_, nullptr);
    }
    if (image_ != VK_NULL_HANDLE) {
        vkDestroyImage(device_, image_, nullptr);
    }
    if (memory_) {
        vkFreeMemory(device_, memory_, nullptr);
    }
#ifdef WIN32
    if (win32_ext_mem_handle_) {
        CloseHandle(win32_ext_mem_handle_);
    }
#endif
}

int VulkanTexture::CreateWinGlSharedTexture(VkFormat format,
                                            uint32_t width,
                                            uint32_t height,
                                            VkImageUsageFlags usage,
                                            VkImageTiling tiling,
                                            VkMemoryPropertyFlags mem_flag,
                                            VkImageLayout init_layout,
                                            HANDLE &win32_ext_mem_handle,
                                            uint64_t &memory_size)
{
    constexpr VkExternalMemoryHandleTypeFlagBits ext_mem_handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    uint32_t queue_idx = ctx_->QueueIndex();

    VkPhysicalDeviceExternalImageFormatInfo external_image_format_info = {};
    external_image_format_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
    external_image_format_info.handleType = ext_mem_handle_type;
    
    VkPhysicalDeviceImageFormatInfo2 image_format_info = {};
    image_format_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
    image_format_info.format = format;
    image_format_info.tiling = tiling;
    image_format_info.type = VK_IMAGE_TYPE_2D;
    image_format_info.usage = usage;
    image_format_info.pNext = &external_image_format_info;

    VkExternalImageFormatProperties external_image_format_props = {};
    external_image_format_props.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;

    VkImageFormatProperties2 vk_image_format_props2 = {};
    vk_image_format_props2.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
    vk_image_format_props2.pNext = &external_image_format_props;

    // 使用GPU专用内存来创建vulkan-gl共享texture，可以修复AMD显卡结果不正确的问题
    bool enable_dedicated_mem = true;
    VK_CALL(vkGetPhysicalDeviceImageFormatProperties2(ctx_->PhysicalDevice(), &image_format_info, &vk_image_format_props2));
    auto& external_props = external_image_format_props.externalMemoryProperties;
    CHECK_RETURN_IF(!(external_props.compatibleHandleTypes & ext_mem_handle_type), -1);
    CHECK_RETURN_IF(!(external_props.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT), -1);
    enable_dedicated_mem &= (external_props.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT);

    VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo = {};
    externalMemoryImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
    externalMemoryImageCreateInfo.handleTypes = ext_mem_handle_type;
    
    /**
     * If the pNext chain includes a VkExternalMemoryImageCreateInfo or VkExternalMemoryImageCreateInfoNV structure 
     * whose handleTypes member is not 0, it is as if VK_IMAGE_CREATE_ALIAS_BIT is set to VkImageCreateInfo.flags
     */
    VkImageCreateInfo image_createInfo = {};
    image_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_createInfo.pNext = &externalMemoryImageCreateInfo;
    image_createInfo.imageType = VK_IMAGE_TYPE_2D;
    image_createInfo.extent = {width, height, 1u};
    image_createInfo.format = format;
    image_createInfo.mipLevels = 1;
    image_createInfo.arrayLayers = 1;
    image_createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    image_createInfo.tiling = tiling;
    image_createInfo.usage = usage;
    image_createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_createInfo.queueFamilyIndexCount = 1;
    image_createInfo.pQueueFamilyIndices = &queue_idx;
    image_createInfo.initialLayout = init_layout;

    VK_CALL(vkCreateImage(device_, &image_createInfo, nullptr, &image_));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device_, image_, &memory_requirements);

    int memory_type_index = ctx_->FindMemoryType(memory_requirements.memoryTypeBits, mem_flag);
    CHECK_RETURN_IF(memory_type_index < 0, -1);
    
    VkExportMemoryAllocateInfo export_mem_alloc_info = {};
    export_mem_alloc_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    export_mem_alloc_info.handleTypes = ext_mem_handle_type;
    
    VkMemoryDedicatedAllocateInfo dedicate_mem_info = {};
    if (enable_dedicated_mem) {
        dedicate_mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        dedicate_mem_info.image = image_;
        export_mem_alloc_info.pNext = &dedicate_mem_info;
    }

    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = uint32_t(memory_type_index);
    memory_allocate_info.pNext = &export_mem_alloc_info;
    VK_CALL(vkAllocateMemory(device_, &memory_allocate_info, nullptr, &memory_));
    VK_CALL(vkBindImageMemory(device_, image_, memory_, 0));

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = image_;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = format;
    image_view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                                         VK_COMPONENT_SWIZZLE_IDENTITY,
                                         VK_COMPONENT_SWIZZLE_IDENTITY,
                                         VK_COMPONENT_SWIZZLE_IDENTITY};
    image_view_create_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u};

    VK_CALL(vkCreateImageView(device_, &image_view_create_info, nullptr, &image_view_));

    VkMemoryGetWin32HandleInfoKHR get_win32_handle_info = {};
    get_win32_handle_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    get_win32_handle_info.memory = memory_;
    get_win32_handle_info.handleType = ext_mem_handle_type;
    VK_CALL(vkGetMemoryWin32HandleKHR(device_, &get_win32_handle_info, &win32_ext_mem_handle));
    win32_ext_mem_handle_ = win32_ext_mem_handle;

    texture_image_info_.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    texture_image_info_.imageView = image_view_;
    texture_image_info_.sampler = VK_NULL_HANDLE;

    tex_w_ = width;
    tex_h_ = height;
    mem_size_ = memory_requirements.size;
    memory_size = mem_size_;

    return 0;
}

