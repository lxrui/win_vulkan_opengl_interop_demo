#ifndef __VULKAN_CONTEXT_H__
#define __VULKAN_CONTEXT_H__

#include <vector>
#include <assert.h>
#include <string>
#include <unordered_map>
#include <map>
#include <list>
#include <iostream>
#include <fstream>
#include <utility>
#include <memory>
#if VK_NO_PROTOTYPES
#include "vulkan_pfn.h"
#endif
#include "vulkan_resources.h"

class VulkanContext {
public:
    VulkanContext() = default;
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    ~VulkanContext();
    int Init();
    int FindMemoryType(uint32_t memory_requirement_bits, VkFlags requirement_flags);

    VkDevice Device() { return device_; }
    VkPhysicalDevice PhysicalDevice() { return physical_device_; }
    VkQueue Queue() { return queue_; }
    uint32_t QueueIndex() { return queue_index_; }
    
private:
    int CreateInstance();
    int CreateDevice();
    VkInstance instance_ = nullptr;
    VkPhysicalDevice physical_device_ = nullptr;
    VkDevice device_ = nullptr;
    VkQueue queue_ = nullptr;
    std::vector<const char*> instance_exts_;
    std::vector<const char*> device_exts_;
    uint32_t queue_index_ = -1;

    VkPhysicalDeviceProperties device_props_ = {};
    VkPhysicalDeviceMemoryProperties physical_device_memory_props_ = {};
};

template <typename T>
class VulkanResourceOnDevice {
public:
    explicit VulkanResourceOnDevice(const std::shared_ptr<VulkanContext>& ctx) : ctx_(ctx) {
        device_ = ctx_->Device();
    }
    VulkanResourceOnDevice() = delete;
	VulkanResourceOnDevice(const VulkanResourceOnDevice&) = delete;
    VulkanResourceOnDevice& operator=(const VulkanResourceOnDevice&) = delete;
    virtual ~VulkanResourceOnDevice() {};
    const T VkType() { return vk_resource_; }
    
protected:	
    std::shared_ptr<VulkanContext> ctx_;
    VkDevice device_;
    T vk_resource_ = VK_NULL_HANDLE;
};

#endif // __VULKAN_CONTEXT_H__
