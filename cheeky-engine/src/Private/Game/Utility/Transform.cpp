#include "Game/Utility/Transform.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/matrix.hpp>
#include <glm/glm.hpp>

namespace Game
{
    Transform Transform::FromMatrix(glm::mat4 mat)
    {
        Transform xform{};

        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(mat, xform.scale, xform.rotation, xform.position, skew, perspective);

        return xform;
    }

    glm::mat4 Transform::ToMatrix() const
    {
        return glm::translate(position) * glm::mat4(rotation) * glm::scale(scale);
    }

    Transform Transform::Transformed(const Transform& other) const
    {
        // we only scale the position by the parent matrix, scale and rotation can be handled directly.
        // now I understand why Godot separates the origin and basis.
        glm::mat4 parent_rot_scale_mat = other.ToMatrix();
        Transform ret{};
        ret.position = glm::vec3(parent_rot_scale_mat * glm::vec4(position, 1.0f));
        ret.scale = scale * other.scale;
        ret.rotation = other.rotation * rotation;

        return ret;
    }

    Transform Transform::InverseTransformed(const Transform& other) const
    {
        Transform ret{};
        glm::mat4 parent_rot_scale_mat = other.ToMatrix();
        glm::mat4 inv_parent = glm::inverse(parent_rot_scale_mat);
        ret.position = glm::vec3(inv_parent * glm::vec4(position, 1.0f));
        ret.scale = scale / other.scale;
        ret.rotation = glm::inverse(other.rotation) * rotation;

        return ret;
    }

    bool Transform::Equals(const Transform& lhs, const Transform& rhs, float tolerance)
    {
        const glm::vec<3, bool>& pos_res = glm::epsilonEqual(lhs.position, rhs.position, tolerance);
        const bool pos_equals = pos_res.x && pos_res.y && pos_res.z;

        const glm::vec<3, bool>& scale_res = glm::epsilonEqual(lhs.scale, rhs.scale, tolerance);
        const bool scale_equals = scale_res.x && scale_res.y && scale_res.z;

        const glm::vec<4, bool>& rotation_res = glm::epsilonEqual(lhs.rotation, rhs.rotation, tolerance);
        const bool rotation_equals = rotation_res.x && rotation_res.y && rotation_res.z;

        return pos_equals && scale_equals && rotation_equals;
    }

} // namespace Game