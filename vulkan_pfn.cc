#include "vulkan_pfn.h"

#include <iostream>
//#include <thread>
#include <mutex>

#if defined(WIN32)
#include <windows.h>
#define VUNKAN_LBIRARY_PATH "vulkan-1.dll"
#define LOAD_VULKAN_LIBRARY(path) LoadLibrary(path)
#define FREE_VULKAN_LIBRARY(handle) FreeLibrary(handle)
#define VULKAN_SYM(handle, function) GetProcAddress(handle, function)
static HMODULE handle = nullptr;
#else 
#include <dlfcn.h>
#define VUNKAN_LBIRARY_PATH "libvulkan.so"
#define LOAD_VULKAN_LIBRARY(path) dlopen(path, RTLD_NOW | RTLD_LOCAL)
#define FREE_VULKAN_LIBRARY(handle) dlclose(handle)
#define VULKAN_SYM(handle, function) dlsym(handle, function)
static void *handle = nullptr;
#endif


//static std::once_flag init_flag;
static std::mutex mtx;

int InitVulkanRuntimeImpl();

int InitVulkanRuntime() {
   /* try {
        std::call_once(init_flag, InitVulkanRuntimeImpl);
    } catch (...) {
        std::cout << "vulkan runtime init fail" << std::endl;
        return -1;
    }*/

    std::lock_guard<std::mutex> lock(mtx);
    return InitVulkanRuntimeImpl();
}

int InitVulkanDeviceExtensions(VkDevice device) {
#ifdef WIN32
    vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");
    vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleNV");
    if ((!vkGetMemoryWin32HandleNV) && (!vkGetMemoryWin32HandleKHR)) return -1;
    vkGetPhysicalDeviceExternalImageFormatPropertiesNV = (PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV)vkGetDeviceProcAddr(device, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV");
#endif
    return 0;
}

int InitVulkanRuntimeImpl() {
    handle = LOAD_VULKAN_LIBRARY(VUNKAN_LBIRARY_PATH);
    if (!handle) {
        std::cout << "vulkan runtime init error" << std::endl;
        return -1;
    }

    std::cout << "vulkan runtime init success" << std::endl;

#define GET_VULKAN_PROC(fun) fun = reinterpret_cast<PFN_##fun>(VULKAN_SYM(handle, #fun))
    // Vulkan supported, set function addresses
    GET_VULKAN_PROC(vkCreateInstance);
    GET_VULKAN_PROC(vkDestroyInstance);
    GET_VULKAN_PROC(vkEnumerateInstanceVersion); //version 1.1
    GET_VULKAN_PROC(vkEnumeratePhysicalDevices);
    GET_VULKAN_PROC(vkGetPhysicalDeviceFeatures);
    GET_VULKAN_PROC(vkGetPhysicalDeviceFormatProperties);
    GET_VULKAN_PROC(vkGetPhysicalDeviceImageFormatProperties);
    GET_VULKAN_PROC(vkGetPhysicalDeviceProperties);
    GET_VULKAN_PROC(vkGetPhysicalDeviceProperties2); //version 1.1
    GET_VULKAN_PROC(vkGetPhysicalDeviceQueueFamilyProperties);
    GET_VULKAN_PROC(vkGetPhysicalDeviceMemoryProperties);
    GET_VULKAN_PROC(vkGetInstanceProcAddr);
    GET_VULKAN_PROC(vkGetDeviceProcAddr);
    GET_VULKAN_PROC(vkCreateDevice);
    GET_VULKAN_PROC(vkDestroyDevice);
    GET_VULKAN_PROC(vkEnumerateInstanceExtensionProperties);
    GET_VULKAN_PROC(vkEnumerateDeviceExtensionProperties);
    GET_VULKAN_PROC(vkEnumerateInstanceLayerProperties);
    GET_VULKAN_PROC(vkEnumerateDeviceLayerProperties);
    GET_VULKAN_PROC(vkGetDeviceQueue);
    GET_VULKAN_PROC(vkQueueSubmit);
    GET_VULKAN_PROC(vkQueueWaitIdle);
    GET_VULKAN_PROC(vkDeviceWaitIdle);
    GET_VULKAN_PROC(vkAllocateMemory);
    GET_VULKAN_PROC(vkFreeMemory);
    GET_VULKAN_PROC(vkMapMemory);
    GET_VULKAN_PROC(vkUnmapMemory);
    GET_VULKAN_PROC(vkFlushMappedMemoryRanges);
    GET_VULKAN_PROC(vkInvalidateMappedMemoryRanges);
    GET_VULKAN_PROC(vkGetDeviceMemoryCommitment);
    GET_VULKAN_PROC(vkBindBufferMemory);
    GET_VULKAN_PROC(vkBindImageMemory);
    GET_VULKAN_PROC(vkGetBufferMemoryRequirements);
    GET_VULKAN_PROC(vkGetImageMemoryRequirements);
    GET_VULKAN_PROC(vkGetImageSparseMemoryRequirements);
    GET_VULKAN_PROC(vkGetPhysicalDeviceSparseImageFormatProperties);
    GET_VULKAN_PROC(vkQueueBindSparse);
    GET_VULKAN_PROC(vkCreateFence);
    GET_VULKAN_PROC(vkDestroyFence);
    GET_VULKAN_PROC(vkResetFences);
    GET_VULKAN_PROC(vkGetFenceStatus);
    GET_VULKAN_PROC(vkWaitForFences);
    GET_VULKAN_PROC(vkCreateSemaphore);
    GET_VULKAN_PROC(vkDestroySemaphore);
    GET_VULKAN_PROC(vkCreateEvent);
    GET_VULKAN_PROC(vkDestroyEvent);
    GET_VULKAN_PROC(vkGetEventStatus);
    GET_VULKAN_PROC(vkSetEvent);
    GET_VULKAN_PROC(vkResetEvent);
    GET_VULKAN_PROC(vkCreateQueryPool);
    GET_VULKAN_PROC(vkDestroyQueryPool);
    GET_VULKAN_PROC(vkGetQueryPoolResults);
    GET_VULKAN_PROC(vkCreateBuffer);
    GET_VULKAN_PROC(vkDestroyBuffer);
    GET_VULKAN_PROC(vkCreateBufferView);
    GET_VULKAN_PROC(vkDestroyBufferView);
    GET_VULKAN_PROC(vkCreateImage);
    GET_VULKAN_PROC(vkDestroyImage);
    GET_VULKAN_PROC(vkGetImageSubresourceLayout);
    GET_VULKAN_PROC(vkCreateImageView);
    GET_VULKAN_PROC(vkDestroyImageView);
    GET_VULKAN_PROC(vkCreateShaderModule);
    GET_VULKAN_PROC(vkDestroyShaderModule);
    GET_VULKAN_PROC(vkCreatePipelineCache);
    GET_VULKAN_PROC(vkDestroyPipelineCache);
    GET_VULKAN_PROC(vkGetPipelineCacheData);
    GET_VULKAN_PROC(vkMergePipelineCaches);
    GET_VULKAN_PROC(vkCreateGraphicsPipelines);
    GET_VULKAN_PROC(vkCreateComputePipelines);
    GET_VULKAN_PROC(vkDestroyPipeline);
    GET_VULKAN_PROC(vkCreatePipelineLayout);
    GET_VULKAN_PROC(vkDestroyPipelineLayout);
    GET_VULKAN_PROC(vkCreateSampler);
    GET_VULKAN_PROC(vkDestroySampler);
    GET_VULKAN_PROC(vkCreateDescriptorSetLayout);
    GET_VULKAN_PROC(vkDestroyDescriptorSetLayout);
    GET_VULKAN_PROC(vkCreateDescriptorPool);
    GET_VULKAN_PROC(vkDestroyDescriptorPool);
    GET_VULKAN_PROC(vkResetDescriptorPool);
    GET_VULKAN_PROC(vkAllocateDescriptorSets);
    GET_VULKAN_PROC(vkFreeDescriptorSets);
    GET_VULKAN_PROC(vkUpdateDescriptorSets);
    GET_VULKAN_PROC(vkCreateFramebuffer);
    GET_VULKAN_PROC(vkDestroyFramebuffer);
    GET_VULKAN_PROC(vkCreateRenderPass);
    GET_VULKAN_PROC(vkDestroyRenderPass);
    GET_VULKAN_PROC(vkGetRenderAreaGranularity);
    GET_VULKAN_PROC(vkCreateCommandPool);
    GET_VULKAN_PROC(vkDestroyCommandPool);
    GET_VULKAN_PROC(vkResetCommandPool);
    GET_VULKAN_PROC(vkAllocateCommandBuffers);
    GET_VULKAN_PROC(vkFreeCommandBuffers);
    GET_VULKAN_PROC(vkBeginCommandBuffer);
    GET_VULKAN_PROC(vkEndCommandBuffer);
    GET_VULKAN_PROC(vkResetCommandBuffer);
    GET_VULKAN_PROC(vkCmdBindPipeline);
    GET_VULKAN_PROC(vkCmdSetViewport);
    GET_VULKAN_PROC(vkCmdSetScissor);
    GET_VULKAN_PROC(vkCmdSetLineWidth);
    GET_VULKAN_PROC(vkCmdSetDepthBias);
    GET_VULKAN_PROC(vkCmdSetBlendConstants);
    GET_VULKAN_PROC(vkCmdSetDepthBounds);
    GET_VULKAN_PROC(vkCmdSetStencilCompareMask);
    GET_VULKAN_PROC(vkCmdSetStencilWriteMask);
    GET_VULKAN_PROC(vkCmdSetStencilReference);
    GET_VULKAN_PROC(vkCmdBindDescriptorSets);
    GET_VULKAN_PROC(vkCmdBindIndexBuffer);
    GET_VULKAN_PROC(vkCmdBindVertexBuffers);
    GET_VULKAN_PROC(vkCmdDraw);
    GET_VULKAN_PROC(vkCmdDrawIndexed);
    GET_VULKAN_PROC(vkCmdDrawIndirect);
    GET_VULKAN_PROC(vkCmdDrawIndexedIndirect);
    GET_VULKAN_PROC(vkCmdDispatch);
    GET_VULKAN_PROC(vkCmdDispatchIndirect);
    GET_VULKAN_PROC(vkCmdCopyBuffer);
    GET_VULKAN_PROC(vkCmdCopyImage);
    GET_VULKAN_PROC(vkCmdBlitImage);
    GET_VULKAN_PROC(vkCmdCopyBufferToImage);
    GET_VULKAN_PROC(vkCmdCopyImageToBuffer);
    GET_VULKAN_PROC(vkCmdUpdateBuffer);
    GET_VULKAN_PROC(vkCmdFillBuffer);
    GET_VULKAN_PROC(vkCmdClearColorImage);
    GET_VULKAN_PROC(vkCmdClearDepthStencilImage);
    GET_VULKAN_PROC(vkCmdClearAttachments);
    GET_VULKAN_PROC(vkCmdResolveImage);
    GET_VULKAN_PROC(vkCmdSetEvent);
    GET_VULKAN_PROC(vkCmdResetEvent);
    GET_VULKAN_PROC(vkCmdWaitEvents);
    GET_VULKAN_PROC(vkCmdPipelineBarrier);
    GET_VULKAN_PROC(vkCmdBeginQuery);
    GET_VULKAN_PROC(vkCmdEndQuery);
    GET_VULKAN_PROC(vkCmdResetQueryPool);
    GET_VULKAN_PROC(vkCmdWriteTimestamp);
    GET_VULKAN_PROC(vkCmdCopyQueryPoolResults);
    GET_VULKAN_PROC(vkCmdPushConstants);
    GET_VULKAN_PROC(vkCmdBeginRenderPass);
    GET_VULKAN_PROC(vkCmdNextSubpass);
    GET_VULKAN_PROC(vkCmdEndRenderPass);
    GET_VULKAN_PROC(vkCmdExecuteCommands);
    GET_VULKAN_PROC(vkDestroySurfaceKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceSurfaceSupportKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    GET_VULKAN_PROC(vkCreateSwapchainKHR);
    GET_VULKAN_PROC(vkDestroySwapchainKHR);
    GET_VULKAN_PROC(vkGetSwapchainImagesKHR);
    GET_VULKAN_PROC(vkAcquireNextImageKHR);
    GET_VULKAN_PROC(vkQueuePresentKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceDisplayPropertiesKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
    GET_VULKAN_PROC(vkGetDisplayPlaneSupportedDisplaysKHR);
    GET_VULKAN_PROC(vkGetDisplayModePropertiesKHR);
    GET_VULKAN_PROC(vkCreateDisplayModeKHR);
    GET_VULKAN_PROC(vkGetDisplayPlaneCapabilitiesKHR);
    GET_VULKAN_PROC(vkCreateDisplayPlaneSurfaceKHR);
    GET_VULKAN_PROC(vkCreateSharedSwapchainsKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceImageFormatProperties2);

#ifdef VK_USE_PLATFORM_XLIB_KHR
    GET_VULKAN_PROC(vkCreateXlibSurfaceKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceXlibPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    GET_VULKAN_PROC(vkCreateXcbSurfaceKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceXcbPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    GET_VULKAN_PROC(vkCreateWaylandSurfaceKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_MIR_KHR
    GET_VULKAN_PROC(vkCreateMirSurfaceKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceMirPresentationSupportKHR);
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    GET_VULKAN_PROC(vkCreateAndroidSurfaceKHR);
    GET_VULKAN_PROC(vkGetMemoryAndroidHardwareBufferANDROID);
    GET_VULKAN_PROC(vkGetAndroidHardwareBufferPropertiesANDROID);
    GET_VULKAN_PROC(vkBindImageMemory2KHR);
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    GET_VULKAN_PROC(vkCreateWin32SurfaceKHR);
    GET_VULKAN_PROC(vkGetPhysicalDeviceWin32PresentationSupportKHR);
#endif

#ifdef USE_DEBUG_EXTENTIONS
    GET_VULKAN_PROC(vkCreateDebugReportCallbackEXT);
    GET_VULKAN_PROC(vkDestroyDebugReportCallbackEXT);
    GET_VULKAN_PROC(vkDebugReportMessageEXT);
#endif

#undef GET_VULKAN_PROC
    return 0;
}

// No Vulkan support, do not set function addresses
PFN_vkCreateInstance vkCreateInstance;
PFN_vkDestroyInstance vkDestroyInstance;
PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
PFN_vkCreateDevice vkCreateDevice;
PFN_vkDestroyDevice vkDestroyDevice;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
PFN_vkGetDeviceQueue vkGetDeviceQueue;
PFN_vkQueueSubmit vkQueueSubmit;
PFN_vkQueueWaitIdle vkQueueWaitIdle;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
PFN_vkAllocateMemory vkAllocateMemory;
PFN_vkFreeMemory vkFreeMemory;
PFN_vkMapMemory vkMapMemory;
PFN_vkUnmapMemory vkUnmapMemory;
PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment;
PFN_vkBindBufferMemory vkBindBufferMemory;
PFN_vkBindImageMemory vkBindImageMemory;
PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements;
PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties;
PFN_vkQueueBindSparse vkQueueBindSparse;
PFN_vkCreateFence vkCreateFence;
PFN_vkDestroyFence vkDestroyFence;
PFN_vkResetFences vkResetFences;
PFN_vkGetFenceStatus vkGetFenceStatus;
PFN_vkWaitForFences vkWaitForFences;
PFN_vkCreateSemaphore vkCreateSemaphore;
PFN_vkDestroySemaphore vkDestroySemaphore;
PFN_vkCreateEvent vkCreateEvent;
PFN_vkDestroyEvent vkDestroyEvent;
PFN_vkGetEventStatus vkGetEventStatus;
PFN_vkSetEvent vkSetEvent;
PFN_vkResetEvent vkResetEvent;
PFN_vkCreateQueryPool vkCreateQueryPool;
PFN_vkDestroyQueryPool vkDestroyQueryPool;
PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
PFN_vkCreateBuffer vkCreateBuffer;
PFN_vkDestroyBuffer vkDestroyBuffer;
PFN_vkCreateBufferView vkCreateBufferView;
PFN_vkDestroyBufferView vkDestroyBufferView;
PFN_vkCreateImage vkCreateImage;
PFN_vkDestroyImage vkDestroyImage;
PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout;
PFN_vkCreateImageView vkCreateImageView;
PFN_vkDestroyImageView vkDestroyImageView;
PFN_vkCreateShaderModule vkCreateShaderModule;
PFN_vkDestroyShaderModule vkDestroyShaderModule;
PFN_vkCreatePipelineCache vkCreatePipelineCache;
PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
PFN_vkMergePipelineCaches vkMergePipelineCaches;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
PFN_vkCreateComputePipelines vkCreateComputePipelines;
PFN_vkDestroyPipeline vkDestroyPipeline;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
PFN_vkCreateSampler vkCreateSampler;
PFN_vkDestroySampler vkDestroySampler;
PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
PFN_vkResetDescriptorPool vkResetDescriptorPool;
PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
PFN_vkCreateFramebuffer vkCreateFramebuffer;
PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
PFN_vkCreateRenderPass vkCreateRenderPass;
PFN_vkDestroyRenderPass vkDestroyRenderPass;
PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity;
PFN_vkCreateCommandPool vkCreateCommandPool;
PFN_vkDestroyCommandPool vkDestroyCommandPool;
PFN_vkResetCommandPool vkResetCommandPool;
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
PFN_vkEndCommandBuffer vkEndCommandBuffer;
PFN_vkResetCommandBuffer vkResetCommandBuffer;
PFN_vkCmdBindPipeline vkCmdBindPipeline;
PFN_vkCmdSetViewport vkCmdSetViewport;
PFN_vkCmdSetScissor vkCmdSetScissor;
PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds;
PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask;
PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask;
PFN_vkCmdSetStencilReference vkCmdSetStencilReference;
PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
PFN_vkCmdDraw vkCmdDraw;
PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
PFN_vkCmdDrawIndirect vkCmdDrawIndirect;
PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect;
PFN_vkCmdDispatch vkCmdDispatch;
PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect;
PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
PFN_vkCmdCopyImage vkCmdCopyImage;
PFN_vkCmdBlitImage vkCmdBlitImage;
PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer;
PFN_vkCmdFillBuffer vkCmdFillBuffer;
PFN_vkCmdClearColorImage vkCmdClearColorImage;
PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
PFN_vkCmdClearAttachments vkCmdClearAttachments;
PFN_vkCmdResolveImage vkCmdResolveImage;
PFN_vkCmdSetEvent vkCmdSetEvent;
PFN_vkCmdResetEvent vkCmdResetEvent;
PFN_vkCmdWaitEvents vkCmdWaitEvents;
PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
PFN_vkCmdBeginQuery vkCmdBeginQuery;
PFN_vkCmdEndQuery vkCmdEndQuery;
PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults;
PFN_vkCmdPushConstants vkCmdPushConstants;
PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
PFN_vkCmdNextSubpass vkCmdNextSubpass;
PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
PFN_vkQueuePresentKHR vkQueuePresentKHR;
PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR;
PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR;
PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR;
PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR;
PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR;
PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR;
PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR;
PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR;
PFN_vkGetPhysicalDeviceImageFormatProperties2 vkGetPhysicalDeviceImageFormatProperties2;

#ifdef VK_USE_PLATFORM_XLIB_KHR
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_MIR_KHR
PFN_vkCreateMirSurfaceKHR vkCreateMirSurfaceKHR;
PFN_vkGetPhysicalDeviceMirPresentationSupportKHR vkGetPhysicalDeviceMirPresentationSupportKHR;
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
PFN_vkGetMemoryAndroidHardwareBufferANDROID vkGetMemoryAndroidHardwareBufferANDROID;
PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID;
PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR;
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR;
PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR;
PFN_vkGetMemoryWin32HandleNV vkGetMemoryWin32HandleNV;
PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV;
#endif

PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;

