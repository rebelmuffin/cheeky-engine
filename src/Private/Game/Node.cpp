#include "Game/Node.h"
#include "Game/GameLogging.h"
#include "Game/GameScene.h"

#include "ThirdParty/ImGUI.h"
#include <algorithm>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/matrix.hpp>
#include <memory>
#include <utility>

namespace Game
{
    Transform Transform::FromMatrix(glm::mat4 mat)
    {
        Transform xform{};

        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(mat, xform.scale, xform.rotation, xform.position, skew, perspective);

        return xform;
    }

    glm::mat4 Transform::ToMatrix() const
    {
        return glm::mat4(rotation) * glm::translate(position) * glm::scale(scale);
    }

    Transform Transform::Transformed(const Transform& other) const
    {
        glm::mat4 result = ToMatrix() * other.ToMatrix();
        return FromMatrix(result);
    }

    Transform Transform::InverseTransformed(const Transform& other) const
    {
        glm::mat4 result = other.ToMatrix() * ToMatrix();
        return FromMatrix(result);
    }

    RootNode& Node::SceneRoot() { return const_cast<RootNode&>(std::as_const(*this).SceneRoot()); }

    const RootNode& Node::SceneRoot() const
    {
        const Node* cur_node = this;
        while (cur_node->m_parent != nullptr)
        {
            cur_node = cur_node->m_parent;
        }

        return (const RootNode&)*cur_node;
    }

    /// Set whether this node should be updated every tick through OnTickUpdate.
    void Node::SetTickUpdate(bool tick_update_enabled)
    {
        Scene().SetNodeTickUpdate(*this, tick_update_enabled);
    }

    /// Destroy the given child node.
    void Node::DestroyChild(NodeId_t child_node_id)
    {
        std::erase_if(
            m_children,
            [child_node_id](const std::unique_ptr<Node>& node)
            {
                return node->Id() == child_node_id;
            }
        );
    }

    /// Move the given child from this node and attach them to another node instead.
    void Node::MoveChild(NodeId_t child_node_id, Node& new_parent)
    {
        auto child_it = std::find_if(
            m_children.begin(),
            m_children.end(),
            [child_node_id](const std::unique_ptr<Node>& node)
            {
                return node->Id() == child_node_id;
            }
        );

        if (child_it == m_children.end())
        {
            LogWarning(
                "Trying to move a child(Id %d) that does not belong to this node(%s).",
                Id(),
                DebugDisplayName().data()
            );
            return;
        }

        new_parent.AddChild(std::move(*child_it));
        m_children.erase(child_it);
    }

    /// Attach this node to given node, detaching from the current parent.
    void Node::AttachToParent(Node& new_parent)
    {
        if (IsRootNode())
        {
            LogError(
                "Trying to attach a root node to another node(%s). Root nodes cannot be moved.",
                new_parent.DebugDisplayName().data()
            );
            return;
        }

        m_parent->MoveChild(Id(), new_parent);
    }

    void Node::SetLocalTransform(const Transform& transform)
    {
        m_local_transform = transform;
        RefreshTransform();
    }

    void Node::OnImGui() { ImGui::Text("%s", DebugDisplayName().data()); }

    Node* Node::AddChild(std::unique_ptr<Node>&& node)
    {
        std::unique_ptr<Node>& new_child = m_children.emplace_back(std::move(node));
        return new_child.get();
    }

    void Node::RefreshTransform()
    {
        m_world_transform = m_local_transform;
        m_world_transform = m_local_transform.Transformed(m_parent->WorldTransform());

        for (std::unique_ptr<Node>& node : m_children)
        {
            node->RefreshTransform();
        }
    }
} // namespace Game