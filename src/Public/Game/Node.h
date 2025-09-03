#pragma once

#include "Game/GameTime.h"
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/quaternion.hpp>

#include <memory>
#include <vector>

namespace Game
{
    struct Transform
    {
        glm::vec3 position{};
        glm::vec3 scale{};
        glm::quat rotation{};

        static Transform FromMatrix(glm::mat4 mat);
        glm::mat4 ToMatrix() const;

        Transform Transformed(const Transform& other) const;
        Transform InverseTransformed(const Transform& other) const;
    };

    using NodeId_t = uint32_t;
    constexpr NodeId_t INVALID_NODE_ID = 0u;

    class GameScene;
    class RootNode;

    // Each node represents a position in the game scene and can have any number of children that will be
    // transformed along with their parent.
    class Node
    {
      public:
        virtual ~Node() = default;

        // getters
        NodeId_t Id() const { return m_id; }
        const std::string& Name() const { return m_name; }
        const std::vector<std::unique_ptr<Node>>& Children() const { return m_children; }
        const Transform& WorldTransform() const { return m_world_transform; }
        const Transform& LocalTransform() const { return m_local_transform; }
        bool IsRootNode() const { return m_parent == nullptr; }

        GameScene& Scene() { return *m_owning_scene; }
        const GameScene& Scene() const { return *m_owning_scene; }
        RootNode& SceneRoot();
        const RootNode& SceneRoot() const;

        // operations

        /// Set whether this node should be updated every tick through OnTickUpdate.
        void SetTickUpdate(bool tick_update_enabled);

        /// Destroy the given child node.
        void DestroyChild(NodeId_t child_node_id);

        /// Move the given child from this node and attach them to another node instead.
        void MoveChild(NodeId_t child_node_id, Node& new_parent);

        /// Attach this node to given node, detaching from the current parent.
        void AttachToParent(Node& new_parent);

        void SetLocalTransform(const Transform& transform);

      protected:
        Node(GameScene& scene, bool tick_update);

        // runtime functions

        /// Called when the node is added to the scene.
        virtual void OnAdded() {};

        /// Called when the node is removed from the scene before destruction.
        virtual void OnRemoved() {};

        /// Called every tick if enabled by default or through SetTickUpdate.
        virtual void OnTickUpdate(const GameTime&) {};

        virtual std::string DebugDisplayName() { return m_name; }
        virtual void OnImGui();

      private:
        Node* AddChild(std::unique_ptr<Node>&& node);
        void RefreshTransform();

        NodeId_t m_id{};
        std::string m_name = "node";
        GameScene* m_owning_scene = nullptr;
        std::vector<std::unique_ptr<Node>> m_children{};
        Node* m_parent = nullptr;

        Transform m_local_transform{};
        Transform m_world_transform{};

        friend class GameScene; // so the scene can access the runtime calls.
    };

    class RootNode : public Node
    {
    };

    class CameraNode : public Node
    {
      public:
        float vertical_fov = 70.0f;
    };
} // namespace Game