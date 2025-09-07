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

    Node::Node(std::string_view name, bool tick_update) : m_name(name), m_tick_updating(tick_update) {}

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

    void Node::Destroy()
    {
        if (m_parent == nullptr)
        {
            LogError(
                "Trying to destroy a node with no parent. This node is either the root node or "
                "uninitialised. Node: %s(Id %u)",
                DebugDisplayName().data(),
                Id()
            );
            return;
        }

        m_parent->DestroyChild(m_id);
    }

    /// Destroy the given child node. All destruction has to go through this for proper release of resources.
    void Node::DestroyChild(NodeId_t child_node_id)
    {
        auto found = std::find_if(
            m_children.begin(),
            m_children.end(),
            [child_node_id](const std::unique_ptr<Node>& node)
            {
                return node->Id() == child_node_id;
            }
        );

        if (found == m_children.end())
        {
            LogError(
                "Trying to destroy a child(Id %u) that does not belong to this node(%s)",
                child_node_id,
                DebugDisplayName().data()
            );
            return;
        }

        // destroy the children of the child recursively
        std::vector<NodeId_t> children_nodes{};
        children_nodes.reserve(found->get()->m_children.size());
        for (const std::unique_ptr<Node>& child : found->get()->m_children)
        {
            children_nodes.emplace_back(child->m_id);
        }

        for (const NodeId_t child_id : children_nodes)
        {
            found->get()->DestroyChild(child_id);
        }

        m_owning_scene->ReleaseNode(*found->get());
        m_children.erase(found);
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
                "Trying to move a child(Id %u) that does not belong to this node(%s).",
                child_node_id,
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
    void Node::SetLocalPosition(const glm::vec3& position)
    {
        m_local_transform.position = position;
        RefreshTransform();
    }
    void Node::SetLocalRotation(const glm::quat& rotation)
    {
        m_local_transform.rotation = rotation;
        RefreshTransform();
    }
    void Node::SetLocalScale(const glm::vec3& scale)
    {
        m_local_transform.scale = scale;
        RefreshTransform();
    }

    void Node::OnImGui()
    {
        if (ImGui::DragFloat3("Position", &m_local_transform.position.x))
        {
            RefreshTransform();
        }
        if (ImGui::DragFloat3("Scale", &m_local_transform.scale.x, 0.5f, 0.01f))
        {
            RefreshTransform();
        }
    }

    void Node::PostCreateChild(Node& node) { m_owning_scene->RegisterNode(node); }

    Node* Node::AddChild(std::unique_ptr<Node>&& node)
    {
        std::unique_ptr<Node>& new_child = m_children.emplace_back(std::move(node));
        new_child->m_parent = this;
        new_child->RefreshTransform();
        return new_child.get();
    }

    void Node::RefreshTransform()
    {
        if (m_parent != nullptr)
        {
            m_world_transform = m_local_transform.Transformed(m_parent->WorldTransform());
        }
        else
        {
            m_world_transform = m_local_transform;
        }

        for (std::unique_ptr<Node>& node : m_children)
        {
            node->RefreshTransform();
        }
    }

    RootNode::RootNode() : Node("root node", false) {}

    CameraNode::CameraNode(std::string_view name) : Node(name, false) {}
} // namespace Game