#pragma once

#include "VkTypes.h"
#include <VkBootstrapDispatch.h>
#include <span>

namespace Utils
{
    class DescriptorLayoutBuilder
    {
      public:
        void AddBinding(uint32_t binding, VkDescriptorType descriptor_type);
        void Clear();
        VkDescriptorSetLayout Build(vkb::DispatchTable device_dispatch, VkShaderStageFlags shader_stages,
                                    VkDescriptorSetLayoutCreateFlags flags = 0);

      private:
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
    };

    // Simple descriptor allocator that allocates from a single pool. Will crash if you run out of pool space.
    class DescriptorAllocator
    {
      public:
        struct PoolSizeRatio
        {
            VkDescriptorType type;
            float ratio;
        };

        void InitPool(vkb::DispatchTable device_dispatch, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
        void ClearDescriptors(vkb::DispatchTable device_dispatch);
        void DestroyPool(vkb::DispatchTable device_dispatch);
        VkDescriptorSet Allocate(vkb::DispatchTable device_dispatch, VkDescriptorSetLayout layout_set);

      private:
        VkDescriptorPool m_pool = VK_NULL_HANDLE;
    };

    // Dynamic descriptor allocator that can grow the pool if needed.
    class DescriptorAllocatorDynamic
    {
      public:
        struct PoolSizeRatio
        {
            VkDescriptorType type;
            float ratio;
        };

        void Init(vkb::DispatchTable device_dispatch, uint32_t initial_max_sets, std::span<PoolSizeRatio> pool_ratios);
        void ClearDescriptors(vkb::DispatchTable device_dispatch);
        void DestroyPools(vkb::DispatchTable device_dispatch);
        VkDescriptorSet Allocate(vkb::DispatchTable device_dispatch, VkDescriptorSetLayout layout_set);

      private:
        VkDescriptorPool GetPool(vkb::DispatchTable device_dispatch);
        VkDescriptorPool AllocateNewPool(vkb::DispatchTable device_dispatch, uint32_t max_sets,
                                         std::span<PoolSizeRatio> pool_ratios);

        std::vector<VkDescriptorPool> m_ready_pools;
        std::vector<VkDescriptorPool> m_full_pools;
        std::vector<PoolSizeRatio> m_size_ratios;
        uint32_t m_sets_per_pool;
    };
} // namespace Utils