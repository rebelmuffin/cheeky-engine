#include "Utility/VkDescriptors.h"
#include "VkBootstrapDispatch.h"
#include "VkTypes.h"

#include <vulkan/vulkan_core.h>

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
        m_pool = VK_NULL_HANDLE;
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

    void DescriptorAllocatorDynamic::Init(vkb::DispatchTable device_dispatch, uint32_t initial_max_sets,
                                          std::span<PoolSizeRatio> pool_ratios)
    {
        // create the first pool
        VkDescriptorPool new_pool = AllocateNewPool(device_dispatch, initial_max_sets, pool_ratios);
        m_ready_pools.push_back(new_pool);
        m_size_ratios = std::vector<PoolSizeRatio>(pool_ratios.begin(), pool_ratios.end());
    }

    void DescriptorAllocatorDynamic::ClearDescriptors(vkb::DispatchTable device_dispatch)
    {
        for (VkDescriptorPool pool : m_ready_pools)
        {
            device_dispatch.resetDescriptorPool(pool, 0);
        }

        for (VkDescriptorPool pool : m_full_pools)
        {
            device_dispatch.resetDescriptorPool(pool, 0);
            m_ready_pools.push_back(pool);
        }
        m_full_pools.clear();
    }

    void DescriptorAllocatorDynamic::DestroyPools(vkb::DispatchTable device_dispatch)
    {
        for (VkDescriptorPool pool : m_ready_pools)
        {
            device_dispatch.destroyDescriptorPool(pool, nullptr);
        }
        m_ready_pools.clear();

        for (VkDescriptorPool pool : m_full_pools)
        {
            device_dispatch.destroyDescriptorPool(pool, nullptr);
        }
        m_full_pools.clear();
    }

    VkDescriptorSet DescriptorAllocatorDynamic::Allocate(vkb::DispatchTable device_dispatch,
                                                         VkDescriptorSetLayout layout_set)
    {
        VkDescriptorPool pool = GetPool(device_dispatch);

        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.pNext = nullptr;
        info.descriptorPool = pool;
        info.descriptorSetCount = 1;
        info.pSetLayouts = &layout_set;

        VkDescriptorSet descriptor_set;
        VkResult result = device_dispatch.allocateDescriptorSets(&info, &descriptor_set);
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            m_full_pools.emplace_back(pool);
            std::erase_if(m_ready_pools, [pool](VkDescriptorPool& p) { return p == pool; });
            // try again
            return Allocate(device_dispatch, layout_set);
        }

        VK_CHECK(result);
        return descriptor_set;
    }

    VkDescriptorPool DescriptorAllocatorDynamic::GetPool(vkb::DispatchTable device_dispatch)
    {
        if (m_ready_pools.empty() == false) // if any ready pools, get that
        {
            return m_ready_pools[0];
        }

        VkDescriptorPool new_pool = AllocateNewPool(device_dispatch, m_sets_per_pool, m_size_ratios);
        m_ready_pools.push_back(new_pool);
        return new_pool;
    }

    VkDescriptorPool DescriptorAllocatorDynamic::AllocateNewPool(vkb::DispatchTable device_dispatch, uint32_t max_sets,
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

        m_sets_per_pool = max_sets * 2; // next time we will double the pool size

        VkDescriptorPool new_pool;
        VK_CHECK(device_dispatch.createDescriptorPool(&info, nullptr, &new_pool));

        return new_pool;
    }

} // namespace Utils