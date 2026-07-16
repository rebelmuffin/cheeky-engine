#include "Utilities/LineDrawer.h"

#include "Renderer/Viewport.h"
#include "Renderer/VkEngine.h"

namespace Debug
{
    std::unique_ptr<LineDrawer> LineDrawer::s_line_drawer = nullptr;

    LineDrawer& LineDrawer::Instance()
    {
        if (s_line_drawer == nullptr)
        {
            s_line_drawer = std::make_unique<LineDrawer>();
        }

        return *s_line_drawer;
    }

    void LineDrawer::AddLine(
        glm::vec3 start, glm::vec3 end, DrawDuration duration, Colour colour, bool z_depth
    )
    {
        m_lines.emplace_back(duration, start, end, colour, z_depth);
    }

    void LineDrawer::OnRender(
        Renderer::VulkanEngine& renderer, Renderer::Viewport& viewport, double time_delta_s
    )
    {
        if (m_depth_material == nullptr)
        {
            m_depth_material = CreateMaterial(renderer, false);
        }

        if (m_no_depth_material == nullptr)
        {
            m_no_depth_material = CreateMaterial(renderer, true);
        }

        if (m_lines.empty())
        {
            return;
        }

        Renderer::MeshAsset mesh_asset;
        mesh_asset.name = "debug lines";

        std::vector<Renderer::Vertex> vertices;
        std::vector<uint32_t> indices;

        for (const DebugLine& line : m_lines)
        {
            Renderer::GeoSurface surface{};
            surface.first_index = indices.size();
            surface.index_count = 2;
            surface.material = line.z_depth ? m_depth_material : m_no_depth_material;

            vertices.push_back(Renderer::Vertex(line.start, 0.0f, { 0.0, 1.0f, 0.0f }, 0.0f, line.colour));
            vertices.push_back(Renderer::Vertex(line.end, 0.0f, { 0.0, 1.0f, 0.0f }, 0.0f, line.colour));
            indices.emplace_back(vertices.size() - 1);
            indices.emplace_back(vertices.size() - 2);

            mesh_asset.surfaces.emplace_back(surface);
        }

        mesh_asset.buffers = renderer.UploadMesh(indices, vertices);
        static Renderer::MeshHandle created_mesh =
            renderer.RegisterMeshAsset(std::move(mesh_asset), mesh_asset.name);

        for (const Renderer::GeoSurface& surface : created_mesh->surfaces)
        {
            Renderer::RenderObject obj{};
            obj.index_buffer = created_mesh->buffers.index_buffer->buffer;
            obj.vertex_buffer_address = created_mesh->buffers.vertex_buffer_address;

            obj.first_index = surface.first_index;
            obj.index_count = surface.index_count;
            obj.material = &surface.material->material;
            obj.transform = glm::identity<glm::mat4>();

            viewport.frame_context.render_objects.emplace_back(obj);
        }

        UpdateLineDurations(time_delta_s);
    }

    void LineDrawer::UpdateLineDurations(double time_delta_s)
    {
        auto it = m_lines.end();
        while (--it != m_lines.begin())
        {
            if (m_lines.back().duration.seconds <= 0.0f)
            {
                it = m_lines.erase(it);
            }

            it->duration.seconds -= time_delta_s;
        }
    }

    std::shared_ptr<Renderer::GLTFMaterial> LineDrawer::CreateMaterial(
        Renderer::VulkanEngine& renderer, bool depth
    )
    {
        Renderer::MaterialPass pass =
            depth ? Renderer::MaterialPass::MainColour : Renderer::MaterialPass::NoDepth;
        std::shared_ptr<Renderer::GLTFMaterial> material = std::make_shared<Renderer::GLTFMaterial>();
        material->material =
            renderer.DebugLineMaterial()->CreateInstance(renderer.DeviceDispatchTable(), pass).value();

        return material;
    }
} // namespace Debug