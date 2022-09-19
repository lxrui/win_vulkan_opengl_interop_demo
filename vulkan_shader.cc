#include "vulkan_shader.h"

VulkanShader::~VulkanShader() {
    if (vk_resource_) vkDestroyShaderModule(device_, vk_resource_, nullptr);
}

int VulkanShader::CreateShader(const uint8_t* spv_code, size_t spv_size) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spv_size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spv_code);
    VK_CALL(vkCreateShaderModule(device_, &createInfo, nullptr, &vk_resource_));
    return 0;
}