#pragma once

#include "Collider.h"
#include "Physics/PhysicsDefinitions.h"
#include "PhysicsBody.h"

#include <glm/gtc/quaternion.hpp>

namespace Physics
{
    struct BodyCreateParams
    {
        glm::vec3 position{};
        glm::quat rotation = glm::identity<glm::quat>();
        glm::vec3 scale = glm::vec3(1.0f);
    };

    class PhysicsScene
    {
      public:
        PhysicsScene();
        ~PhysicsScene();

        PhysicsScene(const PhysicsScene&) = delete;
        PhysicsScene(PhysicsScene&&) = delete;

        BodyHandle CreateBody(const BodyCreateParams& params, bool dynamic = true);
        void DestroyBody(BodyHandle body);

        ColliderHandle CreateSphereCollider(
            BodyHandle attached_body, const glm::vec3& position, float radius
        );
        ColliderHandle CreateBoxCollider(
            BodyHandle attached_body, const BodyCreateParams& params, const glm::vec3& half_extents
        );
        void DestroyCollider(ColliderHandle collider);

        void Step(float time_step);

      private:
        WorldId m_world;
    };
} // namespace Physics