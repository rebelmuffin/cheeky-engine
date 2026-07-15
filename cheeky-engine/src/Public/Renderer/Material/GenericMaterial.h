#pragma once

#include "MaterialDefinition.h"
#include "MaterialInstance.h"
#include "Renderer/Utility/VkDescriptors.h"

#include <VkBootstrapDispatch.h>

#include <optional>
#include <variant>

namespace Renderer
{
    struct MaterialEngineInterface;

    struct BufferParameter
    {
        BufferHandle uniform_buffer{};
        uint32_t buffer_offset{};
    };

    struct ImageParameter
    {
        ImageHandle image{};
        VkSampler sampler{};
    };

    struct GenericMaterialParameter
    {
        std::variant<BufferParameter, ImageParameter> parameter{};
    };

    struct GenericMaterialParameters
    {
        std::vector<GenericMaterialParameter> parameters{};
    };

    class GenericMaterial
    {
      public:
        static std::optional<GenericMaterial> CreateMaterial(
            std::string_view name, MaterialEngineInterface& interface, const MaterialDefinition& definition
        );

        void DestroyResources();

        std::optional<MaterialInstance> CreateInstance(
            vkb::DispatchTable& device_dispatch,
            MaterialPass pass,
            const GenericMaterialParameters& params,
            Utils::DescriptorAllocatorDynamic& descriptor_allocator
        ) const;

      private:
        GenericMaterial(
            std::string_view name, MaterialEngineInterface& interface, const MaterialDefinition& definition
        );
        bool InitPipelines(MaterialEngineInterface& interface);

        std::string m_name;
        vkb::DispatchTable* m_device_dispatch{};
        MaterialDefinition m_definition{};
        std::unordered_map<MaterialPass, MaterialPipeline> m_pipelines;
        VkDescriptorSetLayout m_descriptor_layout;
    };
} // namespace Renderer