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

        BodyHandle CreateBody(const BodyCreateParams& params, bool dynamic = true, const char* debug_name = nullptr);
        void DestroyBody(BodyHandle body);

        ColliderHandle CreateSphereCollider(
            BodyHandle attached_body, const glm::vec3& position, float radius, const char* debug_name = nullptr
        );
        ColliderHandle CreateBoxCollider(
            BodyHandle attached_body,
            const BodyCreateParams& params,
            const glm::vec3& half_extents,
            const char* debug_name = nullptr
        );
        void DestroyCollider(ColliderHandle collider);

        void GetBodyTransform(BodyHandle body, glm::vec3* position, glm::quat* rotation);

        [[nodiscard]] bool GetPaused() const { return m_paused; }
        void SetPaused(const bool paused) { m_paused = paused; }

        [[nodiscard]] float GetFrequency() const { return m_frequency; }
        void SetFrequency(const float frequency) { m_frequency = frequency; }

        void Step();

      private:
        WorldId m_world{};
        bool m_paused = true;
        float m_frequency = 60.0f; // 60 times per sec

        friend class PhysicsDebugger;
        friend class PhysicsDrawer;
    };
} // namespace Physics