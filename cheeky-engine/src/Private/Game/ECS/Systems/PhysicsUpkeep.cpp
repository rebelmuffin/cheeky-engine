#include "Game/ECS/Systems/PhysicsUpkeep.h"

#include "Game/ECS/Components/ColliderComponent.h"
#include "Game/ECS/Components/EngineData.h"
#include "Game/ECS/Components/PhysicsBodyComponent.h"
#include "Game/ECS/Components/SceneData.h"
#include "Game/ECS/Components/TransformComponent.h"
#include "Physics/PhysicsScene.h"

namespace
{
    void AddColliderToBody(Game::ECS::Entity body_entity, Physics::ColliderHandle collider)
    {
        Game::ECS::ColliderSet& set = body_entity.ensure<Game::ECS::ColliderSet>();
        set.set.emplace(collider);
    }

    void CreateBody(
        Physics::PhysicsScene& physics,
        const Game::ECS::Entity entity,
        const Game::ECS::WorldTransform& transform,
        const Game::ECS::PhysicsBodyComponent& body
    )
    {
        const Physics::BodyCreateParams params{
            .position = transform.Transform().position,
            .rotation = transform.Transform().rotation,
            .scale = transform.Transform().scale,
        };

        const Physics::BodyHandle handle = physics.CreateBody(params, body.is_dynamic, entity.name());
        std::ignore = entity.set<Physics::BodyHandle>(handle);

        if (const Game::ECS::SphereCollider* sphere = entity.try_get<Game::ECS::SphereCollider>())
        {
            const Physics::ColliderHandle col_handle =
                physics.CreateSphereCollider(handle, {}, sphere->Radius(), entity.name());
            AddColliderToBody(entity, col_handle);
        }
        if (const Game::ECS::BoxCollider* box = entity.try_get<Game::ECS::BoxCollider>())
        {
            const Physics::BodyCreateParams col_params{};
            const Physics::ColliderHandle col_handle =
                physics.CreateBoxCollider(handle, col_params, box->half_extents(), entity.name());
            AddColliderToBody(entity, col_handle);
        }
    }

    void DestroyBody(Physics::PhysicsScene& physics, const Game::ECS::Entity entity)
    {
        if (const Physics::BodyHandle* body_handle = entity.try_get<Physics::BodyHandle>())
        {
            physics.DestroyBody(*body_handle);
            std::ignore = entity.remove<Physics::BodyHandle>();
        }

        if (const Game::ECS::ColliderSet* collider_set = entity.try_get<Game::ECS::ColliderSet>())
        {
            for (const Physics::ColliderHandle& col : collider_set->set)
            {
                physics.DestroyCollider(col);
            }
            std::ignore = entity.remove<Game::ECS::ColliderSet>();
        }
    }
} // namespace

namespace Game::ECS::Systems
{
    void PhysicsUpkeep::OnInitialise()
    {
        m_world->observer<PhysicsBodyComponent>()
            .event(flecs::OnSet)
            .event(flecs::OnRemove)
            .each(
                [](const flecs::iter& it, const size_t i, const PhysicsBodyComponent& body)
                {
                    const World world = it.world();
                    const Entity entity = it.entity(i);
                    Physics::PhysicsScene* physics = world.get<SceneData>().physics_scene;
                    if (physics == nullptr)
                    {
                        return;
                    }

                    const WorldTransform& transform = entity.ensure<WorldTransform>();

                    // regardless of why this is getting updated, recreate it.
                    // might need to be more efficient with this later on
                    DestroyBody(*physics, entity);
                    if (it.event() == flecs::OnSet)
                    {
                        CreateBody(*physics, entity, transform, body);
                    }
                }
            );
    }
} // namespace Game::ECS::Systems
