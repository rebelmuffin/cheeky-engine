#include "Game/Utility/SceneCreationUtils.h"
#include "Game/ECS/Components/EngineData.h"
#include "Game/ECS/Components/MeshComponent.h"
#include "Game/ECS/ECSNode.h"
#include "Game/GameScene.h"
#include "Game/Node.h"
#include "Game/Nodes/MeshNode.h"
#include "Renderer/Utility/VkLoader.h"

namespace
{
    Game::Node& CreateGameNodeFromGLTFNode(
        Game::Node& game_node_parent,
        const Renderer::GLTFNode& gltf_node,
        const Renderer::GLTFScene& gltf_scene
    )
    {
        Game::Node* created_node = nullptr;

        const fastgltf::Node& loaded_node = gltf_scene.scene_nodes[gltf_node.scene_node_idx];
        // create a mesh node if it's supposed to have a mesh
        if (loaded_node.meshIndex.has_value())
        {
            created_node = &game_node_parent.CreateChild<Game::MeshNode>(
                loaded_node.name, gltf_scene.loaded_meshes[loaded_node.meshIndex.value()]
            );
        }
        else
        {
            // empty node if not, the transform is still important.
            created_node = &game_node_parent.CreateChild<Game::Node>(loaded_node.name);
        }

        created_node->SetLocalTransform(Game::Transform::FromMatrix(gltf_node.transform));
        for (const Renderer::GLTFNode& child : gltf_node.children)
        {
            CreateGameNodeFromGLTFNode(*created_node, child, gltf_scene);
        }

        return *created_node;
    }

    Game::Node& CreateGameNodeFromGLTFNodeECS(
        Game::ECS::World& world,
        Game::Node& game_node_parent,
        const Renderer::GLTFNode& gltf_node,
        const Renderer::GLTFScene& gltf_scene
    )
    {
        Game::ECSNode* created_node = nullptr;

        const fastgltf::Node& loaded_node = gltf_scene.scene_nodes[gltf_node.scene_node_idx];
        created_node =
            &game_node_parent.CreateChild(std::make_unique<Game::ECSNode>(world, loaded_node.name));

        // create a mesh component if it's supposed to have a mesh
        if (loaded_node.meshIndex.has_value())
        {
            Renderer::MeshHandle mesh = gltf_scene.loaded_meshes[loaded_node.meshIndex.value()];
            created_node->Entity().insert(
                [mesh](Game::ECS::MeshComponent& mesh_component)
                {
                    mesh_component = { mesh };
                }
            );
        }

        created_node->SetLocalTransform(Game::Transform::FromMatrix(gltf_node.transform));
        for (const Renderer::GLTFNode& child : gltf_node.children)
        {
            CreateGameNodeFromGLTFNodeECS(world, *created_node, child, gltf_scene);
        }

        return *created_node;
    }
} // namespace

namespace Game::Utils
{
    void LoadGltfIntoGameScene(Renderer::VulkanEngine& engine, Node& node, std::filesystem::path file_path)
    {
        const std::optional<Renderer::GLTFScene> scene = Renderer::Utils::LoadGltfScene(engine, file_path);

        if (scene.has_value() == false)
        {
            return;
        }

        if (scene->root_node.has_value())
        {
            // the root is not a valid scene node, it simply contains the real nodes, we have to look at
            // what's inside.
            Node& scene_root = node.CreateChild<Node>(file_path.filename().c_str());
            for (const Renderer::GLTFNode& child : scene->root_node->children)
            {
                CreateGameNodeFromGLTFNode(scene_root, child, *scene);
            }
            return;
        }

        // if no hierarchy, simply create a flat list of children
        for (const Renderer::MeshHandle& mesh : scene->loaded_meshes)
        {
            node.CreateChild<MeshNode>(mesh->name, mesh);
        }
    }

    void LoadGltfIntoGameSceneECS(ECS::World& world, Node& node, std::filesystem::path file_path)
    {
        ECS::EngineData* engine_data = world.get_ref<ECS::EngineData>().get();
        if (engine_data == nullptr || engine_data->renderer == nullptr)
        {
            return;
        }

        const std::optional<Renderer::GLTFScene> scene =
            Renderer::Utils::LoadGltfScene(*engine_data->renderer, file_path);

        if (scene.has_value() == false)
        {
            return;
        }

        if (scene->root_node.has_value())
        {
            // the root is not a valid scene node, it simply contains the real nodes, we have to look at
            // what's inside.
            Node& scene_root = node.CreateChild<Node>(file_path.filename().c_str());
            for (const Renderer::GLTFNode& child : scene->root_node->children)
            {
                CreateGameNodeFromGLTFNodeECS(world, scene_root, child, *scene);
            }
            return;
        }

        // if no hierarchy, simply create a flat list of children
        for (const Renderer::MeshHandle& mesh : scene->loaded_meshes)
        {
            node.CreateChild<MeshNode>(mesh->name, mesh);
        }
    }

} // namespace Game::Utils