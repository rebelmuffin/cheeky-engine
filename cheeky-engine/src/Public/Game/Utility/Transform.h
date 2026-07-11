#pragma once

#include <glm/gtc/quaternion.hpp>

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

        static bool Equals(const Transform& lhs, const Transform& rhs, float tolerance = glm::epsilon<float>());
    };
}