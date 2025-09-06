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
        Node(name, true),
        m_mesh_asset(mesh),
        m_scene_item(nullptr)
    {
    }

    void MeshNode::OnAdded()
    {
        // init adds the scene item.
        std::unique_ptr<Renderer::MeshSceneItem> scene_item = std::make_unique<Renderer::MeshSceneItem>();
        scene_item->name = Name();
        scene_item->asset = m_mesh_asset;
        std::unique_ptr<Renderer::SceneItem>& item =
            Scene().RenderScene()->scene_items.emplace_back(std::move(scene_item));
        m_scene_item = static_cast<Renderer::MeshSceneItem*>(item.get());
    }

    void MeshNode::OnRemoved()
    {
        // deinit removes the scene item
        bool found = false;
        std::vector<std::unique_ptr<Renderer::SceneItem>>& scene_items = Scene().RenderScene()->scene_items;
        for (auto it = scene_items.begin(); it != scene_items.end(); ++it)
        {
            if (it->get() == m_scene_item)
            {
                found = true;
                scene_items.erase(it);
                break;
            }
        }

        if (found == false)
        {
            LogError(
                "Could not find the scene item to destroy within render scene. Did something else delete it?"
            );
        }
    }

    void MeshNode::OnTickUpdate(const GameTime&)
    {
        if (m_scene_item == nullptr)
        {
            return;
        }

        m_scene_item->transform = WorldTransform().ToMatrix();
    }
} // namespace Game