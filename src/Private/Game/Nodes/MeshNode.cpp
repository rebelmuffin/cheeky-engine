#include "Game/Nodes/MeshNode.h"

#include "Game/GameLogging.h"
#include "Game/GameScene.h"
#include "Game/Node.h"
#include "Renderer/Renderable.h"
#include "Renderer/Utility/VkLoader.h"
#include <memory>

namespace Game
{
    MeshNode::MeshNode(std::string_view name, const Renderer::MeshHandle& mesh) :
        Node(name, false, true),
        m_mesh_asset(mesh)
    {
    }

    void MeshNode::OnAdded() {}

    void MeshNode::OnRemoved() {}

    void MeshNode::Draw(Renderer::DrawContext& ctx)
    {
        for (const Renderer::GeoSurface& surface : m_mesh_asset->surfaces)
        {
            Renderer::RenderObject obj{};
            obj.index_buffer = m_mesh_asset->buffers.index_buffer->buffer;
            obj.vertex_buffer_address = m_mesh_asset->buffers.vertex_buffer_address;

            obj.first_index = surface.first_index;
            obj.index_count = surface.index_count;
            obj.material = &surface.material->material;
            obj.transform = WorldTransform().ToMatrix();

            ctx.render_objects.emplace_back(obj);
        }
    }
} // namespace Game