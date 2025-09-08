#pragma once

#include "Game/GameTime.h"
#include "Renderer/Renderable.h"
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/quaternion.hpp>

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace Game
{
    struct Transform
    {
        glm::vec3 position{};
        glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::quat rotation = glm::identity<glm::quat>();

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
        Node(std::string_view name, bool tick_update = false, bool renderable = false);
        virtual ~Node() = default;

        // getters
        NodeId_t Id() const { return m_id; }
        const std::string& Name() const { return m_name; }
        const std::vector<std::unique_ptr<Node>>& Children() const { return m_children; }
        const Transform& WorldTransform() const { return m_world_transform; }
        const Transform& LocalTransform() const { return m_local_transform; }
        bool IsRootNode() const { return m_parent == nullptr; }

        Node* Parent() { return m_parent; }
        const Node* Parent() const { return m_parent; }
        GameScene& Scene() { return *m_owning_scene; }
        const GameScene& Scene() const { return *m_owning_scene; }
        RootNode& SceneRoot();
        const RootNode& SceneRoot() const;

        // operations

        /// Create a child of this node. This is the intended way of creating any nodes within a game scene.
        template <typename T, typename... Args>
        T& CreateChild(Args... args);

        /// Set whether this node should be updated every tick through OnTickUpdate.
        void SetTickUpdate(bool tick_update_enabled);

        /// Destroy this node.
        void Destroy();

        /// Destroy the given child node. All destruction has to go through this for proper release of
        /// resources.
        void DestroyChild(NodeId_t child_node_id);

        /// Move the given child from this node and attach them to another node instead.
        void MoveChild(NodeId_t child_node_id, Node& new_parent);

        /// Attach this node to given node, detaching from the current parent.
        void AttachToParent(Node& new_parent);

        void SetLocalTransform(const Transform& transform);
        void SetLocalPosition(const glm::vec3& position);
        void SetLocalRotation(const glm::quat& rotation);
        void SetLocalScale(const glm::vec3& scale);

        virtual void OnImGui();

      protected:
        // runtime functions

        /// Called when the node is added to the scene.
        virtual void OnAdded() {};

        /// Called when the node is removed from the scene before destruction.
        virtual void OnRemoved() {};

        /// Called every time we need to draw the scene. Usually once a frame.
        virtual void Draw(Renderer::DrawContext&) {};

        /// Called every tick if enabled by default or through SetTickUpdate.
        virtual void OnTickUpdate(const GameTime&) {};

        virtual std::string DebugDisplayName() { return m_name; }

      private:
        void PostCreateChild(Node& node);
        Node* AddChild(std::unique_ptr<Node>&& node);
        void RefreshTransform();

        NodeId_t m_id{};
        std::string m_name = "node";
        GameScene* m_owning_scene = nullptr;
        std::vector<std::unique_ptr<Node>> m_children{};
        Node* m_parent = nullptr;
        bool m_tick_updating = false; // can be changed at runtime
        bool m_is_renderable = false; // cannot be changed at runtime

        Transform m_local_transform{};
        Transform m_world_transform{};

        friend class GameScene; // so the scene can access the runtime calls.
    };

    class RootNode : public Node
    {
      public:
        RootNode();
    };

    class CameraNode : public Node
    {
      public:
        CameraNode(std::string_view name);

        float vertical_fov = 70.0f;
    };

    template <typename T, typename... Args>
    T& Node::CreateChild(Args... args)
    {
        static_assert(
            std::is_base_of_v<Node, T>,
            "Trying to create a child node that does not inherit from Game::Node. This is unsupported."
        );

        std::unique_ptr<T> node_unique = std::make_unique<T>(std::forward<Args>(args)...);
        T& node_ref = *node_unique.get();
        AddChild(std::move(node_unique));
        PostCreateChild(node_ref);
        return node_ref;
    }
} // namespace Game