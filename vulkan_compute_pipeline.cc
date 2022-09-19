#include "vulkan_compute_pipeline.h"

VulkanComputePipeline::~VulkanComputePipeline() {
    if (vk_resource_) vkDestroyPipeline(device_, vk_resource_, nullptr);
    if (pipeline_layout_) vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    if (descriptor_set_layout_) vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
    if (descriptor_set_) vkFreeDescriptorSets(device_, descriptor_pool_, 1, &descriptor_set_);
    if (descriptor_pool_) vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
}

void VulkanComputePipeline::Bind(uint32_t bind_index, VkDescriptorType desc_type, const void* desc_info) {
    while (layout_bindings_.size() <= bind_index) {
        layout_bindings_.emplace_back();
        binding_flags_.emplace_back();
        write_desc_sets_.emplace_back();
    }
    auto& binding = layout_bindings_[bind_index];
    binding.descriptorType = desc_type;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    binding.binding = bind_index;
    binding.descriptorCount = 1;
    binding.pImmutableSamplers = nullptr;

    // require vulkan driver >= 1.2
    binding_flags_[bind_index] = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

    auto& write_desc = write_desc_sets_[bind_index];
    write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_desc.pNext = nullptr;
    write_desc.dstArrayElement = 0;
    // write_desc.dstSet = descriptor_set_;
    write_desc.descriptorType = desc_type;
    write_desc.dstBinding = bind_index;
    write_desc.pTexelBufferView = nullptr;
    write_desc.pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(desc_info);
    write_desc.pBufferInfo = nullptr;
    write_desc.descriptorCount = 1;
}

int VulkanComputePipeline::CreatePipeline(const std::shared_ptr<VulkanShader>& shader) {
    CHECK_RETURN_IF(nullptr != vk_resource_, -1);
    CHECK_RETURN_IF(layout_bindings_.empty(), -1);
    // config shader stage
    constexpr VkSpecializationMapEntry entries[] = {
        { 1, 0 * sizeof(uint32_t), sizeof(uint32_t) },
        { 2, 1 * sizeof(uint32_t), sizeof(uint32_t) },
        { 3, 2 * sizeof(uint32_t), sizeof(uint32_t) }
    };
    const uint32_t data[] = { local_size_x_, local_size_y_, local_size_z_ };

    VkSpecializationInfo special_info = {};
    special_info.mapEntryCount = 3;
    special_info.pMapEntries = entries;
    special_info.dataSize = sizeof(data);
    special_info.pData = data;

    VkPipelineShaderStageCreateInfo shader_stage = {};
    shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage.module = shader->VkType();
    shader_stage.pName = "main";
    shader_stage.pSpecializationInfo = &special_info;

    // create descriptor set Layout info
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_createInfo = {};
    descriptor_set_layout_createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_createInfo.pBindings = layout_bindings_.data();
    descriptor_set_layout_createInfo.bindingCount = static_cast<uint32_t>(layout_bindings_.size());

    // require vulkan driver >= 1.2
    VkDescriptorSetLayoutBindingFlagsCreateInfo binging_flag_createInfo = {};
    binging_flag_createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    binging_flag_createInfo.bindingCount = binding_flags_.size();
    binging_flag_createInfo.pBindingFlags = binding_flags_.data();
    descriptor_set_layout_createInfo.pNext = &binging_flag_createInfo;

    VK_CALL(vkCreateDescriptorSetLayout(device_, &descriptor_set_layout_createInfo, nullptr, &descriptor_set_layout_));
    // create pipeline layout info
    VkPipelineLayoutCreateInfo pipeline_layout_createInfo = {};
    pipeline_layout_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_createInfo.setLayoutCount = 1;
    pipeline_layout_createInfo.pSetLayouts = &descriptor_set_layout_;
    // if (0 != push_constant_range_.size) {
    //     pipeline_layout_createInfo.pushConstantRangeCount = 1;
    //     pipeline_layout_createInfo.pPushConstantRanges = &push_constant_range_;
    // }
    VK_CALL(vkCreatePipelineLayout(device_, &pipeline_layout_createInfo, nullptr, &pipeline_layout_));
    // create pipeline
    VkComputePipelineCreateInfo computePipelineCreateInfo = {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = pipeline_layout_;
    computePipelineCreateInfo.flags = 0;
    computePipelineCreateInfo.stage = shader_stage;
    VK_CALL(vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &vk_resource_));

    // create DescriptorSet
    std::vector<VkDescriptorPoolSize> pool_sizes(layout_bindings_.size());
    for (uint32_t i = 0; i < layout_bindings_.size(); ++i) { 
        pool_sizes[i].type = layout_bindings_[i].descriptorType;
        pool_sizes[i].descriptorCount = 1;
    }
    VkDescriptorPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_create_info.pPoolSizes = pool_sizes.data();
    pool_create_info.maxSets = 1;
    VK_CALL(vkCreateDescriptorPool(device_, &pool_create_info, nullptr, &descriptor_pool_));

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptor_pool_;
    descriptorSetAllocateInfo.pSetLayouts = &descriptor_set_layout_;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    VK_CALL(vkAllocateDescriptorSets(device_, &descriptorSetAllocateInfo, &descriptor_set_));

    // update and write DescriptorSets info to descriptor_set_
    for (auto& write_desc : write_desc_sets_) {
        write_desc.dstSet = descriptor_set_;
    }
    vkUpdateDescriptorSets(device_, write_desc_sets_.size(), write_desc_sets_.data(), 0, nullptr);
    
    return 0;
}

void VulkanComputePipeline::UpdateBindings() {
    vkUpdateDescriptorSets(device_, write_desc_sets_.size(), write_desc_sets_.data(), 0, nullptr);
}