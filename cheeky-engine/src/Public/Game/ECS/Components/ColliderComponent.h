#pragma once

#include "Physics/Collider.h"

#include <glm/glm.hpp>

#include <set>

namespace Game::ECS
{
    struct ColliderSet
    {
        std::set<Physics::ColliderHandle, Physics::ColliderLess> set{};
    };

    class SphereCollider
    {
      public:
        SphereCollider() = default;
        SphereCollider(const float radius) : m_radius(radius) {}

        [[nodiscard]] float Radius() const { return m_radius; }

      private:
        float m_radius = 0.0f;
    };

    class BoxCollider
    {
      public:
        BoxCollider() = default;
        BoxCollider(glm::vec3 half_extents) : m_extents(half_extents) {}

        [[nodiscard]] const glm::vec3& half_extents() const { return m_extents; }

      private:
        glm::vec3 m_extents{};
    };
} // namespace Game::ECS