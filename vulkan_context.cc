#include "vulkan_context.h"

inline std::string DriverVersionString(const VkPhysicalDeviceProperties& props) {
    int version = props.driverVersion;
    if (4318 == props.vendorID){
        return vulkanResources::versionToStringNv(version);
    } else {
        return vulkanResources::versionToString(version);
    }    
}

VulkanContext::~VulkanContext() {
    if (device_) vkDestroyDevice(device_, nullptr);
    if (instance_) vkDestroyInstance(instance_, nullptr);
}

int VulkanContext::Init() {
    CHECK_RETURN_IF(0 != InitVulkanRuntime(), -1);
    CHECK_RETURN_IF(0 != CreateInstance(), -1);
    CHECK_RETURN_IF(0 != CreateDevice(), -1);
    return 0;
}

int VulkanContext::CreateInstance() {
    VkResult vk_res;
    uint32_t instance_api_version;
    // Get the max. supported Vulkan Version if vkEnumerateInstanceVersion is available (loader version 1.1 and up)
    vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if (vkEnumerateInstanceVersion) {
        VK_CALL(vkEnumerateInstanceVersion(&instance_api_version));
    } else {
        instance_api_version = VK_API_VERSION_1_0;
    }
    std::cout << "Vulkan instance api version:" << vulkanResources::versionToString(instance_api_version) << std::endl;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vulkan_gl_interop";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "no_engine";
    app_info.engineVersion = 1;
    app_info.apiVersion = instance_api_version;

    // Get instance extensions
    std::vector<VkExtensionProperties> instance_extensions;
    do {
        uint32_t ext_count;
        vk_res = vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
        if (vk_res != VK_SUCCESS) {
            break;
        }
        std::vector<VkExtensionProperties> extensions(ext_count);
        vk_res = vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions.data());
        instance_extensions.insert(instance_extensions.end(), extensions.begin(), extensions.end());
    } while (vk_res == VK_INCOMPLETE);

    // Check support for new property and feature queries
    std::map<std::string, bool> check_instance_exts;
    check_instance_exts[VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME] = false;
    check_instance_exts[VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME]     = false;
    check_instance_exts[VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME] = false;
    check_instance_exts[VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME]  = false;

    for (auto& ext : instance_extensions) {
        auto check_ext = check_instance_exts.find(ext.extensionName);
        if (check_instance_exts.end() != check_ext) {
            check_ext->second = true;
            instance_exts_.push_back(ext.extensionName);
        }
    }

    for (auto& it : check_instance_exts) {
        if (!it.second) {
            std::cout << "instance not support extension: " << it.first << "\n";
        }
    }
    std::cout << std::endl;
    
    // Create Vulkan instance
    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.ppEnabledExtensionNames = instance_exts_.data();
    instance_create_info.enabledExtensionCount = (uint32_t)instance_exts_.size();

    // Create vulkan Instance
    VK_CALL(vkCreateInstance(&instance_create_info, nullptr, &instance_));

#define GET_INSTANCE_PROC(fun) fun = reinterpret_cast<PFN_##fun>(vkGetInstanceProcAddr(instance_, #fun))
    GET_INSTANCE_PROC(vkGetPhysicalDeviceExternalSemaphoreProperties);
#ifdef WIN32
    GET_INSTANCE_PROC(vkGetPhysicalDeviceExternalImageFormatPropertiesNV);
#endif // #ifdef WIN32
#undef GET_INSTANCE_PROC
    return 0;
}

int VulkanContext::CreateDevice() {
    VkResult vk_res;
    uint32_t num_GPUs = 0;
    VK_CALL(vkEnumeratePhysicalDevices(instance_, &num_GPUs, nullptr));
    
    std::vector<VkPhysicalDevice> vulkan_devices;
    vulkan_devices.resize(num_GPUs);

    VK_CALL(vkEnumeratePhysicalDevices(instance_, &num_GPUs, vulkan_devices.data()));

    CHECK_RETURN_IF(num_GPUs < 1, -1);

    std::cout << "-------vulkan physical device info--------\n";
    std::cout << "device count:" << num_GPUs << std::endl;

    physical_device_ = nullptr;
    size_t max_memory_size = 0;
    uint32_t select_index = 0;
    for (uint32_t i = 0; i < num_GPUs; ++i) {
        auto& phys_device = vulkan_devices[i];
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(phys_device, &props);
        VkPhysicalDeviceMemoryProperties mem_props = {};
        vkGetPhysicalDeviceMemoryProperties(phys_device, &mem_props);

        std::vector<VkMemoryHeap> heaps(mem_props.memoryHeaps, mem_props.memoryHeaps + mem_props.memoryHeapCount);
        size_t device_heap_size = 0;
        for (auto& heap : heaps) {
            if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                device_heap_size += heap.size;
            }
        }

        std::cout << "device " << i << " : " << props.deviceName
            << " type: " << vulkanResources::physicalDeviceTypeString(props.deviceType);
        std::cout << "\nAPI version: " << vulkanResources::versionToString(props.apiVersion)
            << " driver version:" << DriverVersionString(props)
            << " vendorID:" << props.vendorID << " deviceID:" << props.deviceID;
        std::cout << "\nMemory heap size: " << (device_heap_size >> 20) << " MB\n" << std::endl;

        bool b_update_device = false;
        if (!physical_device_) {
            b_update_device = true;
        } else if (device_props_.deviceType == props.deviceType) {
            if (device_heap_size > max_memory_size) {
                b_update_device = true;
            }
        } else if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == props.deviceType) {
            b_update_device = true;
        }

        if (b_update_device) {
            select_index = i;
            physical_device_ = phys_device;
            max_memory_size = device_heap_size;
            memcpy(&physical_device_memory_props_, &mem_props, sizeof(mem_props));
            memcpy(&device_props_, &props, sizeof(props));
        }
    }

    std::cout << "device index                      : "   << select_index;
    std::cout << "\ndevice name                       : " << device_props_.deviceName;
    std::cout << "\ndevice type                       : " << vulkanResources::physicalDeviceTypeString(device_props_.deviceType);
    std::cout << "\nvendor ID                         : " << device_props_.vendorID;
    std::cout << "\ndevice ID                         : " << device_props_.deviceID;
    std::cout << "\ndriver version                    : " << DriverVersionString(device_props_);
    std::cout << "\nAPI version                       : " << vulkanResources::versionToString(device_props_.apiVersion);
    std::cout << "\nmax push constan size             : " << device_props_.limits.maxPushConstantsSize << " B";
    std::cout << "\nmax uniform buffer size           : " << (device_props_.limits.maxUniformBufferRange >> 20) << " MB";
    std::cout << "\nmax compute work group Invocations: " << device_props_.limits.maxComputeWorkGroupInvocations << std::endl;

    std::vector<VkExtensionProperties> device_extensions;
    do {
        uint32_t ext_count;
        vk_res = vkEnumerateDeviceExtensionProperties(physical_device_, NULL, &ext_count, NULL);
        assert(!vk_res);
        std::vector<VkExtensionProperties> exts(ext_count);
        vk_res = vkEnumerateDeviceExtensionProperties(physical_device_, NULL, &ext_count, exts.data());
        device_extensions.insert(device_extensions.end(), exts.begin(), exts.end());
    } while (vk_res == VK_INCOMPLETE);
    assert(!vk_res);

    std::map<std::string, bool> check_device_exts;
    check_device_exts[VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME] = false;
    check_device_exts[VK_KHR_BIND_MEMORY_2_EXTENSION_NAME] = false;
    check_device_exts[VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME] = false;
    check_device_exts[VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME] = false;
    check_device_exts[VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME] = false;
    check_device_exts[VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME] = false;
    check_device_exts[VK_EXT_GLOBAL_PRIORITY_EXTENSION_NAME] = false;
    // update pipeline descriptor without reset command buffer
    check_device_exts[VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME] = false;
#ifdef ANDROID
    check_device_exts[VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME] = false;
#endif
#ifdef WIN32
    check_device_exts[VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME] = false;
    check_device_exts[VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME] = false;
    check_device_exts[VK_NV_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME] = false;
#endif

    for (auto& ext : device_extensions) {
        auto check_ext = check_device_exts.find(ext.extensionName);
        if (check_device_exts.end() != check_ext) {
            check_ext->second = true;
            device_exts_.push_back(ext.extensionName);
        }
    }

    for (auto& it : check_device_exts) {
        if (!it.second) {
            std::cout << "device not support extension: " << it.first << "\n";
        }
    }
    std::cout << std::endl;
    
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);
    CHECK_RETURN_IF(0 == queue_family_count, -1);

    std::vector<VkQueueFamilyProperties> queue_family_props(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_family_props.data());

    queue_index_ = 0;
    for (; queue_index_ < queue_family_count; ++queue_index_) {
        if (queue_family_props[queue_index_].queueFlags & VK_QUEUE_COMPUTE_BIT) break;
    }

    CHECK_RETURN_IF(queue_index_ >= queue_family_count, -1);
    std::cout << "queue family count:" << queue_family_count << " found index:" << queue_index_ << std::endl;

    VkDeviceQueueCreateInfo queue_create_info = {};
    float queue_priorities = 1.0f;
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_index_;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priorities;

    VkDeviceQueueGlobalPriorityCreateInfoEXT device_queue_global_priority_info_ext = {};
    if (check_device_exts[VK_EXT_GLOBAL_PRIORITY_EXTENSION_NAME]) {
        device_queue_global_priority_info_ext.pNext = nullptr;
        device_queue_global_priority_info_ext.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_EXT;
        device_queue_global_priority_info_ext.globalPriority = VK_QUEUE_GLOBAL_PRIORITY_HIGH_EXT;
        queue_create_info.pNext = &device_queue_global_priority_info_ext;
    }

    // Init device
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.enabledLayerCount = 0;
    if (device_exts_.size() > 0) {
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_exts_.size());
        device_create_info.ppEnabledExtensionNames = device_exts_.data();
    };

    VK_CALL(vkCreateDevice(physical_device_, &device_create_info, nullptr, &device_));

#define GET_DEVICE_PROC(fun) fun = reinterpret_cast<PFN_##fun>(vkGetDeviceProcAddr(device_, #fun))

#ifdef WIN32
    GET_DEVICE_PROC(vkGetSemaphoreWin32HandleKHR);
    GET_DEVICE_PROC(vkGetMemoryWin32HandleKHR);
    GET_DEVICE_PROC(vkGetMemoryWin32HandleNV);
#endif // #ifdef WIN32
#undef GET_DEVICE_PROC

    vkGetDeviceQueue(device_, queue_index_, 0, &queue_);

    return 0;
}

int VulkanContext::FindMemoryType(uint32_t memory_requirement_bits, VkFlags requirement_flags) {
    for (uint32_t mem_index = 0; mem_index < VK_MAX_MEMORY_TYPES; ++mem_index) {
        const uint32_t memory_type_bits = (1 << mem_index);
        const bool isRequiredMemoryType = memory_requirement_bits & memory_type_bits;
        const bool satisfiesFlags = (physical_device_memory_props_.memoryTypes[mem_index].propertyFlags & requirement_flags) == requirement_flags;
        if (isRequiredMemoryType && satisfiesFlags) {
            return static_cast<int>(mem_index);
        }
    }
    return -1;
}
