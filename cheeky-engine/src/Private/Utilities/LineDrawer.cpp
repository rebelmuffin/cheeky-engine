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
        Renderer::MeshAsset mesh_asset;
        mesh_asset.name = "debug lines";

        std::vector<Renderer::Vertex> vertices;
        std::vector<uint32_t> indices;

        // WE NEED THE REAL DEBUG SHADER HERE!
        Renderer::GenericMaterial_GLTF_PBR::MaterialParameters default_mat_params;
        default_mat_params.colour = glm::vec4(1.0f);
        default_mat_params.metal_roughness = glm::vec4(1.0f);
        Renderer::BufferHandle default_mat_uniform = renderer.CreateBuffer(
            &default_mat_params,
            sizeof(default_mat_params),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            "default material uniform buffer"
        );
        Renderer::GenericMaterial_GLTF_PBR::Resources default_mat_resources;
        default_mat_resources.colour_image = renderer.PlaceholderImage();
        default_mat_resources.colour_sampler = renderer.Sampler();
        default_mat_resources.metal_roughness_image = renderer.PlaceholderImage();
        default_mat_resources.metal_roughness_sampler = renderer.Sampler();
        default_mat_resources.uniform_buffer = default_mat_uniform;
        default_mat_resources.buffer_offset = 0;

        std::shared_ptr<Renderer::GLTFMaterial> default_material = std::make_shared<Renderer::GLTFMaterial>();
        default_material->material =
            renderer.PBRMaterial()
                ->CreateInstance(
                    renderer.DeviceDispatchTable(), Renderer::MaterialPass::MainColour, default_mat_resources
                )
                .value();

        for (const DebugLine& line : m_lines)
        {
            Renderer::GeoSurface surface{};
            surface.first_index = indices.size();
            surface.index_count = 4;
            surface.material = default_material;

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
} // namespace Debug