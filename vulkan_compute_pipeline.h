#ifndef __VULKAN_COMPUTE_PIPELINE_H__
#define __VULKAN_COMPUTE_PIPELINE_H__

#include "vulkan_context.h"
#include "vulkan_shader.h"

class VulkanCommandBuffer;

class VulkanComputePipeline : public VulkanResourceOnDevice<VkPipeline> {
public:
    friend class VulkanCommandBuffer;
    using VulkanResourceOnDevice::VulkanResourceOnDevice;
    virtual ~VulkanComputePipeline() override;
    void SetLocalSize(uint32_t x, uint32_t y, uint32_t z) {
        local_size_x_ = x;
        local_size_y_ = y;
        local_size_z_ = z;
    }
    void Bind(uint32_t bind_index, VkDescriptorType desc_type, const void* desc_info);
    int CreatePipeline(const std::shared_ptr<VulkanShader>& shader);
    void UpdateBindings();
private:
    uint32_t local_size_x_ = 16;
    uint32_t local_size_y_ = 16;
    uint32_t local_size_z_ = 1;
    std::shared_ptr<VulkanShader> vk_shader_ = nullptr;
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings_;
    std::vector<VkDescriptorBindingFlags> binding_flags_;
    std::vector<VkWriteDescriptorSet> write_desc_sets_;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
};

#endif // __VULKAN_COMPUTE_PIPELINE_H__