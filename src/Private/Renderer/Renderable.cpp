#include "Renderer/Renderable.h"
#include "Renderer/RenderObject.h"
#include "Renderer/Utility/VkLoader.h"

namespace Renderer
{
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
