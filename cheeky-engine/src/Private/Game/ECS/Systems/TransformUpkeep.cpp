#include "Game/ECS/Systems/TransformUpkeep.h"

#include "Game/ECS/Components/SceneData.h"
#include "Game/ECS/Components/SceneNodeComponent.h"
#include "Game/GameScene.h"

namespace
{
    Game::Node* NodeFromEntity(Game::ECS::Entity e)
    {
        const Game::ECS::SceneNodeComponent* node_component = e.try_get<Game::ECS::SceneNodeComponent>();
        if (node_component == nullptr || node_component->node_id == Game::INVALID_NODE_ID)
        {
            return nullptr;
        }

        const Game::ECS::SceneData& scene_data = e.world().get<Game::ECS::SceneData>();
        if (scene_data.game_scene == nullptr)
        {
            return nullptr; // lol
        }

        return scene_data.game_scene->NodeFromId(node_component->node_id);
    }
} // namespace

namespace Game::ECS::Systems
{
    TransformUpkeep::TransformUpkeep(World& world) : GameSystem(world) {}

    void TransformUpkeep::OnInitialise()
    {
        // WARNING: This causes a circular call back to the observer.
        //          When a WorldTransform is updated, we call the node to set the transform
        //          which calls the WorldTransform updated, and we call update again due to observer
        //          only reason it's not an infinite loop is because we check if the transform is the same.
        //          Probably needs a better long-term solution than this hacky shit

        m_world->observer<WorldTransform>()
            .event(flecs::OnSet)
            .each(
                [](Entity e, const WorldTransform& t)
                {
                    Node* node = NodeFromEntity(e);
                    if (node != nullptr)
                    {
                        node->SetWorldTransform(t.Transform());
                    }
                }
            );

        m_world->observer<LocalTransform>()
            .event(flecs::OnSet)
            .each(
                [](Entity e, const LocalTransform& t)
                {
                    Node* node = NodeFromEntity(e);
                    if (node != nullptr)
                    {
                        node->SetLocalTransform(t.Transform());
                    }
                }
            );
    }
} // namespace Game::ECS::Systems