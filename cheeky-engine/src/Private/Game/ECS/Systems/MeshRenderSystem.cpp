#include "Game/ECS/Systems/MeshRenderSystem.h"

#include "Game/ECS/Components/MeshComponent.h"
#include "Game/ECS/Components/RenderContext.h"
#include "Game/ECS/Components/TransformComponent.h"

namespace Game::ECS::Systems
{
    MeshRenderSystem::MeshRenderSystem(World& world) : GameSystem(world) {}

    void MeshRenderSystem::OnInitialise()
    {
        m_world->system<const WorldTransform, const MeshComponent>().each(
            [](const Entity e, const WorldTransform& transform, const MeshComponent& mesh_component)
            {
                if (mesh_component.mesh_asset.IsValid() == false)
                {
                    return;
                }

                const World& world = e.world();
                const RenderContext& ctx = world.get<RenderContext>();
                const Renderer::MeshAsset& mesh_asset = *mesh_component.mesh_asset;
                const glm::mat4& xform_mat = transform.Transform().ToMatrix();
                for (const Renderer::GeoSurface& surface : mesh_asset.surfaces)
                {
                    Renderer::RenderObject render_obj{};
                    render_obj.transform = xform_mat;
                    render_obj.index_buffer = mesh_asset.buffers.index_buffer->buffer;
                    render_obj.vertex_buffer_address = mesh_asset.buffers.vertex_buffer_address;

                    render_obj.material = &surface.material->material;
                    render_obj.first_index = surface.first_index;
                    render_obj.index_count = surface.index_count;

                    ctx.draw_context->render_objects.emplace_back(render_obj);
                }
            }
        );
    }
} // namespace Game::ECS::Systems