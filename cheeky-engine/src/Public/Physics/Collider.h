#pragma once

#include "EngineUtils.h"
#include "Physics/PhysicsDefinitions.h"

namespace Physics
{
    struct ColliderHandle
    {
        ShapeId shape_id;
    };

    struct ColliderLess
    {
        bool operator()(const ColliderHandle& lhs, const ColliderHandle& rhs) const
        {
            // is this a bad idea? maybe
            uint64_t lhs_value = lhs.shape_id.index1;
            lhs_value = (lhs_value << 16) + lhs.shape_id.generation;
            lhs_value = (lhs_value << 16) + lhs.shape_id.world0;

            uint64_t rhs_value = rhs.shape_id.index1;
            rhs_value = (rhs_value << 16) + rhs.shape_id.generation;
            rhs_value = (rhs_value << 16) + rhs.shape_id.world0;
            return rhs_value < lhs_value;
        }
    };
} // namespace Physics
