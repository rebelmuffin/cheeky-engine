#include "Game/ECS/ECSNode.h"

#include "Game/ECS/Components/SceneNodeComponent.h"
#include "Game/ECS/Components/TransformComponent.h"

namespace Game
{
    ECSNode::ECSNode(ECS::World& ecs_world, std::string_view name) :
        Node(name, false, false),
        m_entity_id(ecs_world.entity())
    {
    }

    void ECSNode::OnAdded()
    {
        m_entity_id.insert(
            [node_id = Id()](ECS::SceneNodeComponent& node)
            {
                node = { node_id };
            }
        );
    }

    void ECSNode::OnRemoved() { std::ignore = m_entity_id.remove<ECS::SceneNodeComponent>(); }

    void ECSNode::OnTransformUpdated()
    {
        m_entity_id.insert(
            [&](ECS::WorldTransform& world, ECS::LocalTransform& local)
            {
                world = { WorldTransform() };
                local = { LocalTransform() };
            }
        );
    }
} // namespace Game