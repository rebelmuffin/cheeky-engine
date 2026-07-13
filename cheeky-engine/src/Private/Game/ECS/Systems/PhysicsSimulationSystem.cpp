#include "Game/ECS/Systems/PhysicsSimulationSystem.h"

#include "Game/ECS/Components/ColliderComponent.h"
#include "Game/ECS/Components/PhysicsBodyComponent.h"
#include "Game/ECS/Components/SceneData.h"
#include "Game/ECS/Components/TransformComponent.h"
#include "Game/GameTime.h"
#include "Physics/PhysicsScene.h"

namespace Game::ECS::Systems
{
    void PhysicsSimulationSystem::OnInitialise()
    {
        m_world
            ->system("Physics Simulation")
            .kind(flecs::PreUpdate)
            .run(
                [](flecs::iter& it)
                {
                    const World& world = it.world();
                    // const GameTime& time = world.get<GameTime>();

                    Physics::PhysicsScene* physics = world.get<SceneData>().physics_scene;
                    if (physics == nullptr)
                    {
                        return;
                    }

                    physics->Step(1 / 560.0f);
                }
            );

        m_world->system("Physics Transform Update")
            .each(
                [](Entity e, const WorldTransform& transform, const PhysicsBodyComponent& body, const Physics::BodyHandle& body_handle)
                {
                    const World& world = e.world();

                    Physics::PhysicsScene* physics = world.get<SceneData>().physics_scene;
                    if (physics == nullptr)
                    {
                        return;
                    }

                    if (body.is_dynamic == false)
                    {
                        return;
                    }

                    Transform world_transform = transform.Transform();
                    physics->GetBodyTransform(body_handle, &world_transform.position, &world_transform.rotation);
                    e.set<WorldTransform>({ world_transform });
                }
            );
    }
} // namespace Game::ECS::Systems
