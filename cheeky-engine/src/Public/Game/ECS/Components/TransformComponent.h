#pragma once

#include "Game/Utility/Transform.h"

namespace Game::ECS
{
    struct WorldTransform
    {
        WorldTransform() = default;
        WorldTransform(const Game::Transform& transform) : m_transform(transform) {}

        const Game::Transform& Transform() const { return m_transform; }

      private:
        Game::Transform m_transform{};
    };

    struct LocalTransform
    {
        LocalTransform() = default;
        LocalTransform(const Game::Transform& transform) : m_transform(transform) {}

        const Game::Transform& Transform() const { return m_transform; }

      private:
        Game::Transform m_transform{};
    };
} // namespace Game::ECS