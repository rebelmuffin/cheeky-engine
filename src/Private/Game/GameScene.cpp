#include "Game/GameScene.h"
#include "Game/Node.h"

#include <algorithm>
#include <iterator>
#include <memory>

namespace
{
    void UpdateUpkeepList(std::vector<Game::Node*>& vec, Game::Node& target_node, bool enable)
    {
        if (enable == false)
        {
            const auto r_it = std::find_if(
                vec.rbegin(),
                vec.rend(),
                [&target_node](const Game::Node* update_node)
                {
                    return target_node.Id() == update_node->Id();
                }
            );

            if (r_it == vec.rend())
            {
                return;
            }

            // reverse iterator is offset by one turns out. Learned the hard way. Thanks C++
            auto it = --r_it.base();

            // we don't do an erase, we simply swap the element we're removing with the last element
            // effectively "shrinking" the list without moving everything.
            // If the update order needs to be predictible, this won't do but let's see how it goes.
            *it = std::move(vec.back());
            vec.pop_back();
        }
        else
        {
            vec.emplace_back(&target_node);
        }
    }
} // namespace

namespace Game
{
    GameScene::GameScene()
    {
        m_root = std::make_unique<RootNode>();
        RegisterNode(*m_root.get());
    }

    void GameScene::RegisterNode(Node& node)
    {
        node.m_id = ++m_next_node_id;
        node.m_owning_scene = this;
        m_active_nodes[node.m_id] = &node;
        node.OnAdded();
        node.RefreshTransform();
        if (node.m_tick_updating)
        {
            SetNodeTickUpdate(node, true);
        }

        if (node.m_is_renderable)
        {
            SetNodeRenderable(node, true);
        }
    }

    void GameScene::ReleaseNode(Node& node)
    {
        m_active_nodes.erase(node.m_id);
        if (node.m_tick_updating)
        {
            SetNodeTickUpdate(node, false);
        }
        if (node.m_is_renderable)
        {
            SetNodeRenderable(node, false);
        }

        if (&node == m_active_camera)
        {
            m_active_camera = nullptr;
        }
        node.OnRemoved();
    }

    void GameScene::SetActiveCamera(CameraNode* camera) { m_active_camera = camera; }

    void GameScene::SetNodeTickUpdate(Node& node, bool update)
    {
        UpdateUpkeepList(m_updating_nodes, node, update);
    }

    void GameScene::SetNodeRenderable(Node& node, bool is_renderable)
    {
        UpdateUpkeepList(m_renderable_nodes, node, is_renderable);
    }

    void GameScene::SetPaused(bool paused) { m_paused = paused; }

    Node* GameScene::NodeFromId(NodeId_t node_id)
    {
        if (m_active_nodes.contains(node_id))
        {
            return m_active_nodes[node_id];
        }

        return nullptr;
    }

    void GameScene::Draw(Renderer::DrawContext& ctx, const CameraNode* camera_node)
    {
        const CameraNode* used_camera = camera_node;
        if (used_camera == nullptr)
        {
            used_camera = m_active_camera;
        }
        if (used_camera != nullptr)
        {
            ctx.camera_position = m_active_camera->m_world_transform.position;
            ctx.camera_rotation = glm::mat4(m_active_camera->m_world_transform.rotation);
            ctx.camera_vertical_fov = m_active_camera->vertical_fov;
        }

        for (Node* renderable : m_renderable_nodes)
        {
            renderable->Draw(ctx);
        }
    }

    void GameScene::TickUpdate(const GameTime& time) { UpdateAllNodes(time); }

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