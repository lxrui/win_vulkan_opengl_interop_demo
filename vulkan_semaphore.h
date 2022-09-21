#ifndef __VULKAN_SEMAPHORE_H__
#define __VULKAN_SEMAPHORE_H__

#include "vulkan_context.h"

class VulkanSemaphore : public VulkanResourceOnDevice<VkSemaphore> {
public:
    using VulkanResourceOnDevice::VulkanResourceOnDevice;
    virtual ~VulkanSemaphore() override;
    int CreateWin32ExternalSemaphore(HANDLE &ext_semaphore_handle);

private:
#ifdef WIN32
    HANDLE win32_ext_semaphore_handle_ = nullptr;
#endif
};

#endif