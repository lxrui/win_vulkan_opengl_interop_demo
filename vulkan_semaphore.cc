#include "vulkan_semaphore.h"

VulkanSemaphore::~VulkanSemaphore() {
    if (vk_resource_) {
        vkDestroySemaphore(device_, vk_resource_, nullptr);
    }
#ifdef WIN32
    if (win32_ext_semaphore_handle_) {
        CloseHandle(win32_ext_semaphore_handle_);
    }
#endif
}

int VulkanSemaphore::CreateWin32ExternalSemaphore(HANDLE& ext_semaphore_handle) {
#ifdef WIN32
    CHECK_RETURN_IF(!vkGetPhysicalDeviceExternalSemaphoreProperties, -1);
    CHECK_RETURN_IF(!vkGetSemaphoreWin32HandleKHR, -1);
    constexpr auto handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT; 

    VkPhysicalDeviceExternalSemaphoreInfo ext_semaph_info = {};
    ext_semaph_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO;
    ext_semaph_info.handleType = handle_type;

    VkExternalSemaphoreProperties ext_semaph_props = {};

    vkGetPhysicalDeviceExternalSemaphoreProperties(ctx_->PhysicalDevice(), &ext_semaph_info, &ext_semaph_props);
    CHECK_RETURN_IF(!(ext_semaph_props.compatibleHandleTypes & handle_type), -1);

    VkExportSemaphoreCreateInfo ext_create_info = {};
    ext_create_info.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
    ext_create_info.handleTypes = handle_type;

    VkSemaphoreCreateInfo semaph_create_info = {};
    semaph_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaph_create_info.pNext = &ext_create_info;

    VK_CALL(vkCreateSemaphore(device_, &semaph_create_info, nullptr, &vk_resource_));

    VkSemaphoreGetWin32HandleInfoKHR get_handle_info = {};
    get_handle_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
    get_handle_info.handleType = handle_type;
    get_handle_info.semaphore = vk_resource_;
    VK_CALL(vkGetSemaphoreWin32HandleKHR(device_, &get_handle_info, &ext_semaphore_handle));
    win32_ext_semaphore_handle_ = ext_semaphore_handle;
#endif // #ifdef WIN32
    return 0;
}