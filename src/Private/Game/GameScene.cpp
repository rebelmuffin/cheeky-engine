#include "Game/GameScene.h"
#include <algorithm>
#include <iterator>

namespace Game
{
    GameScene::GameScene(Renderer::VulkanEngine& renderer, Renderer::Scene* scene) :
        m_render_engine(&renderer),
        m_render_scene(scene)
    {
    }

    void GameScene::RegisterNode(Node& node)
    {
        node.m_id = ++m_next_node_id;
        node.m_owning_scene = this;
        node.OnAdded();
        node.RefreshTransform();
    }

    void GameScene::ReleaseNode(Node& node)
    {
        SetNodeTickUpdate(node, false);
        node.OnRemoved();
    }

    void GameScene::SetActiveCamera(CameraNode* camera) { m_active_camera = camera; }

    void GameScene::SetNodeTickUpdate(Node& node, bool update)
    {
        if (update == false)
        {
            const auto it = std::find_if(
                m_updating_nodes.rbegin(),
                m_updating_nodes.rend(),
                [&node](const Node* update_node)
                {
                    return node.Id() == update_node->Id();
                }
            );

            if (it == m_updating_nodes.rend())
            {
                return;
            }

            m_updating_nodes.erase(it.base());
        }
        else
        {
            m_updating_nodes.emplace_back(&node);
        }
    }

    void GameScene::SetPaused(bool paused) { m_paused = paused; }

    void GameScene::Draw(const GameTime& time)
    {
        if (m_active_camera != nullptr && m_render_scene != nullptr)
        {
            m_render_scene->camera_position = m_active_camera->m_world_transform.position;
            m_render_scene->camera_rotation = glm::mat4(m_active_camera->m_world_transform.rotation);
            m_render_scene->camera_vertical_fov = m_active_camera->vertical_fov;
        }

        UpdateAllNodes(time);
    }

    void GameScene::UpdateAllNodes(const GameTime& time)
    {
        if (m_paused)
        {
            return;
        }

        for (Node* node : m_updating_nodes)
        {
            node->OnTickUpdate(time);
        }
    }
} // namespace Game