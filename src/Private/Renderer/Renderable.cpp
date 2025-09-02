#include "Renderer/Renderable.h"
#include "Renderer/RenderObject.h"
#include "Renderer/Utility/VkLoader.h"
#include <memory>

namespace Renderer
{
    std::unique_ptr<SceneItem> MeshSceneItem::Clone() const
    {
        std::unique_ptr<MeshSceneItem> item = std::make_unique<MeshSceneItem>();
        item->transform = transform;
        item->name = name;
        item->name.append("clone");
        item->asset = asset;

        return std::move(item);
    }

    void MeshSceneItem::Draw(DrawContext& ctx)
    {
        for (const GeoSurface& surface : asset->surfaces)
        {
            RenderObject obj{};
            obj.index_buffer = asset->buffers.index_buffer->buffer;
            obj.vertex_buffer_address = asset->buffers.vertex_buffer_address;

            obj.first_index = surface.first_index;
            obj.index_count = surface.index_count;
            obj.material = &surface.material->material;
            obj.transform = transform;

            ctx.render_objects.emplace_back(obj);
        }
    }
} // namespace Renderer
