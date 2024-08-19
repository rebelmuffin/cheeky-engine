#include "Utility/VkDescriptors.h"

namespace Utils
{
    void DescriptorLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType descriptor_type)
    {
        VkDescriptorSetLayoutBinding layout_binding{};
        layout_binding.binding = binding;
        layout_binding.descriptorCount = 1;
        layout_binding.descriptorType = descriptor_type;

        m_bindings.push_back(layout_binding);
    }

    void DescriptorLayoutBuilder::Clear()
    {
        m_bindings.clear();
    }

    VkDescriptorSetLayout DescriptorLayoutBuilder::Build(vkb::DispatchTable device_dispatch,
                                                         VkShaderStageFlags shader_stages,
                                                         VkDescriptorSetLayoutCreateFlags flags)
    {
        for (VkDescriptorSetLayoutBinding& binding : m_bindings)
        {
            binding.stageFlags |= shader_stages;
        }

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = nullptr;
        info.pBindings = m_bindings.data();
        info.bindingCount = uint32_t(m_bindings.size());
        info.flags = flags;

        VkDescriptorSetLayout layout_set;
        device_dispatch.createDescriptorSetLayout(&info, nullptr, &layout_set);
        return layout_set;
    }

    void DescriptorAllocator::InitPool(vkb::DispatchTable device_dispatch, uint32_t max_sets,
                                       std::span<PoolSizeRatio> pool_ratios)
    {
        std::vector<VkDescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(pool_ratios.size());
        for (PoolSizeRatio ratio : pool_ratios)
        {
            pool_sizes.push_back(
                VkDescriptorPoolSize{.type = ratio.type, .descriptorCount = uint32_t(ratio.ratio * float(max_sets))});
        }

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.pNext = nullptr;
        info.maxSets = max_sets;
        info.poolSizeCount = uint32_t(pool_sizes.size());
        info.pPoolSizes = pool_sizes.data();

        VK_CHECK(device_dispatch.createDescriptorPool(&info, nullptr, &m_pool));
    }

    void DescriptorAllocator::ClearDescriptors(vkb::DispatchTable device_dispatch)
    {
        device_dispatch.resetDescriptorPool(m_pool, 0);
    }

    void DescriptorAllocator::DestroyPool(vkb::DispatchTable device_dispatch)
    {
        device_dispatch.destroyDescriptorPool(m_pool, nullptr);
    }

    VkDescriptorSet DescriptorAllocator::Allocate(vkb::DispatchTable device_dispatch, VkDescriptorSetLayout layout_set)
    {
        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.pNext = nullptr;
        info.descriptorPool = m_pool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &layout_set;

        VkDescriptorSet descriptor_set;
        VK_CHECK(device_dispatch.allocateDescriptorSets(&info, &descriptor_set));

        return descriptor_set;
    }

} // namespace Utils