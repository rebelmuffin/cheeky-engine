#include "Renderer/Utility/VkDescriptors.h"
#include "Renderer/VkTypes.h"

#include <VkBootstrapDispatch.h>
#include <vulkan/vulkan_core.h>

namespace Renderer::Utils
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
                                                         VkDescriptorSetLayoutCreateFlags flags,
                                                         VkDescriptorSetLayoutBindingFlagsCreateInfo* binding_flags)
    {
        for (VkDescriptorSetLayoutBinding& binding : m_bindings)
        {
            binding.stageFlags |= shader_stages;
        }

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.pNext = binding_flags; // if there is bindings flags in pNext, it is used
        info.pBindings = m_bindings.data();
        info.bindingCount = uint32_t(m_bindings.size());
        info.flags = flags;

        VkDescriptorSetLayout layout_set;
        device_dispatch.createDescriptorSetLayout(&info, nullptr, &layout_set);
        return layout_set;
    }

    void DescriptorAllocator::InitPool(vkb::DispatchTable device_dispatch, uint32_t max_sets,
                                       std::span<DescriptorPoolSizeRatio> pool_ratios,
                                       VkDescriptorPoolCreateFlags pool_flags)
    {
        std::vector<VkDescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(pool_ratios.size());
        for (DescriptorPoolSizeRatio ratio : pool_ratios)
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
        info.flags = pool_flags;

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
                                          std::span<DescriptorPoolSizeRatio> pool_ratios,
                                          VkDescriptorPoolCreateFlags pool_flags)
    {
        // create the first pool
        m_pool_flags = pool_flags;
        VkDescriptorPool new_pool = AllocateNewPool(device_dispatch, initial_max_sets, pool_ratios);
        m_ready_pools.push_back(new_pool);
        m_size_ratios = std::vector<DescriptorPoolSizeRatio>(pool_ratios.begin(), pool_ratios.end());
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
        while (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
        {
            m_full_pools.emplace_back(pool);
            // we can avoid this search if we pop the pool from GetPool and add it back if allocation succeeds,
            // look into doing that if it ever becomes a performance issue. I doubt it (28/08/2025)
            std::erase_if(m_ready_pools, [pool](VkDescriptorPool& p) { return p == pool; });

            pool = GetPool(device_dispatch);
            info.descriptorPool = pool;
            result = device_dispatch.allocateDescriptorSets(&info, &descriptor_set);
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
                                                                 std::span<DescriptorPoolSizeRatio> pool_ratios)
    {
        std::vector<VkDescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(pool_ratios.size());
        for (DescriptorPoolSizeRatio ratio : pool_ratios)
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
        info.flags = m_pool_flags;

        m_sets_per_pool = max_sets * 2; // next time we will double the pool size
        if (m_sets_per_pool > 4092)
        {
            m_sets_per_pool = 4092;
        }

        VkDescriptorPool new_pool;
        VK_CHECK(device_dispatch.createDescriptorPool(&info, nullptr, &new_pool));

        return new_pool;
    }

    void DescriptorWriter::WriteImage(uint32_t binding, VkImageView image_view, VkImageLayout layout, VkSampler sampler,
                                      VkDescriptorType descriptor_type)
    {
        VkDescriptorImageInfo& image_info = image_infos.emplace_back();
        image_info.imageLayout = layout;
        image_info.imageView = image_view;
        image_info.sampler = sampler;

        VkWriteDescriptorSet write_set{};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.pNext = nullptr;
        write_set.descriptorCount = 1;
        write_set.descriptorType = descriptor_type;
        write_set.pImageInfo = &image_info; // deque ptrs are stable
        write_set.dstBinding = binding;
        // write_set.dstSet will be filled in UpdateSet

        writes.push_back(write_set);
    }

    void DescriptorWriter::WriteBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset,
                                       VkDescriptorType descriptor_type)
    {
        VkDescriptorBufferInfo& buffer_info = buffer_infos.emplace_back();
        buffer_info.buffer = buffer;
        buffer_info.offset = offset;
        buffer_info.range = size;

        VkWriteDescriptorSet write_set{};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.pNext = nullptr;
        write_set.descriptorCount = 1;
        write_set.descriptorType = descriptor_type;
        write_set.pBufferInfo = &buffer_info; // deque ptrs are stable
        write_set.dstBinding = binding;
        // write_set.dstSet will be filled in UpdateSet

        writes.push_back(write_set);
    }

    void DescriptorWriter::Clear()
    {
        image_infos.clear();
        buffer_infos.clear();
        writes.clear();
    }

    void DescriptorWriter::UpdateSet(vkb::DispatchTable device_dispatch, VkDescriptorSet set)
    {
        for (VkWriteDescriptorSet& write : writes)
        {
            write.dstSet = set;
        }

        device_dispatch.updateDescriptorSets(uint32_t(writes.size()), writes.data(), 0, nullptr);
        Clear();
    }

} // namespace Renderer::Utils