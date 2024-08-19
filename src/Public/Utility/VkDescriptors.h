#pragma once

#include "VkTypes.h"
#include <VkBootstrapDispatch.h>

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
        VkDescriptorPool m_pool;
    };
} // namespace Utils