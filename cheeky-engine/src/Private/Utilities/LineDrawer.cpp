#include "Utilities/LineDrawer.h"

#include "Debug/DebuggerRegistry.h"
#include "Renderer/Viewport.h"
#include "Renderer/VkEngine.h"
#include "Utilities/LineDrawerDebugger.h"

namespace Debug
{
    std::unique_ptr<LineDrawer> LineDrawer::s_line_drawer = nullptr;

    LineDrawer::LineDrawer() { DebuggerRegistry::Instance().AddDebugger(new LineDrawerDebugger(*this)); }

    LineDrawer& LineDrawer::Instance()
    {
        if (s_line_drawer == nullptr)
        {
            s_line_drawer = std::make_unique<LineDrawer>();
        }

        return *s_line_drawer;
    }

    void LineDrawer::AddLine(
        glm::vec3 start, glm::vec3 end, DrawDuration duration, Cheeky::Colour colour, bool z_depth
    )
    {
        m_lines.emplace_back(duration, start, end, colour, z_depth);
    }

    void LineDrawer::AddCircle(
        const glm::vec3 origin,
        const float radius,
        const glm::quat rotation,
        const size_t segments,
        const DrawDuration duration,
        const Cheeky::Colour colour,
        const bool z_depth
    )
    {
        // x = r * cos(angle)
        // y = r * sin(angle)
        const float angle_per_seg = (2.0f * glm::pi<float>()) / static_cast<float>(segments);
        for (size_t i = 0; i < segments; ++i)
        {
            const float angle1 = static_cast<float>(i) * angle_per_seg;
            const float x1 = radius * glm::cos(angle1);
            const float y1 = radius * glm::sin(angle1);

            const float angle2 = static_cast<float>(i + 1) * angle_per_seg;
            const float x2 = radius * glm::cos(angle2);
            const float y2 = radius * glm::sin(angle2);

            AddLine(
                origin + glm::vec3{ x1, .0f, y1 } * rotation,
                origin + glm::vec3{ x2, .0f, y2 } * rotation,
                duration,
                colour,
                z_depth
            );
        }
    }

    void LineDrawer::OnRender(
        Renderer::VulkanEngine& renderer, Renderer::Viewport& viewport, double time_delta_s
    )
    {
        if (m_depth_material == nullptr)
        {
            m_depth_material = CreateMaterial(renderer, true);
        }

        if (m_no_depth_material == nullptr)
        {
            m_no_depth_material = CreateMaterial(renderer, false);
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

            vertices.push_back(
                Renderer::Vertex(line.start, 0.0f, { 0.0, 1.0f, 0.0f }, 0.0f, line.colour.ToVec4())
            );
            vertices.push_back(
                Renderer::Vertex(line.end, 0.0f, { 0.0, 1.0f, 0.0f }, 0.0f, line.colour.ToVec4())
            );
            indices.emplace_back(vertices.size() - 1);
            indices.emplace_back(vertices.size() - 2);

            mesh_asset.surfaces.emplace_back(surface);
        }

        mesh_asset.buffers = renderer.UploadMesh(indices, vertices);
        m_draw_mesh = renderer.RegisterMeshAsset(std::move(mesh_asset), mesh_asset.name);

        for (const Renderer::GeoSurface& surface : m_draw_mesh->surfaces)
        {
            Renderer::RenderObject obj{};
            obj.index_buffer = m_draw_mesh->buffers.index_buffer->buffer;
            obj.vertex_buffer_address = m_draw_mesh->buffers.vertex_buffer_address;

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