#pragma once

#include "Game/ECS.h"
#include "Game/Node.h"

namespace Game
{
    class ECSNode : public Node
    {
    public:
        ECSNode(ECS::World& ecs_world, std::string_view name);

        ECS::Entity Entity() const { return m_entity_id; }

    protected:
        void OnAdded() override;
        void OnRemoved() override;
        void OnTransformUpdated() override;

    private:
        ECS::Entity m_entity_id{};
    };
}