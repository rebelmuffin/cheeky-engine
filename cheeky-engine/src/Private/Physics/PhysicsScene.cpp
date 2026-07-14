#include "Physics/PhysicsScene.h"

namespace Physics
{
    PhysicsScene::PhysicsScene()
    {
        b3WorldDef definition = b3DefaultWorldDef();
        m_world = b3CreateWorld(&definition);
    }

    PhysicsScene::~PhysicsScene()
    {
        b3DestroyWorld(m_world);
        m_world = b3_nullWorldId;
    }

    BodyHandle PhysicsScene::CreateBody(const BodyCreateParams& params, const bool dynamic, const char* debug_name)
    {
        const glm::vec3& pos = params.position;
        const glm::quat& rot = params.rotation;

        b3BodyDef bodyDef = b3DefaultBodyDef();
        bodyDef.name = debug_name;
        bodyDef.position = { pos.x, pos.y, pos.z };
        bodyDef.rotation = { { rot.x, rot.y, rot.z }, rot.w };
        if (dynamic)
        {
            bodyDef.type = b3_dynamicBody;
        }

        return { b3CreateBody(m_world, &bodyDef) };
    }

    void PhysicsScene::DestroyBody(BodyHandle body)
    {
        if (b3Body_IsValid(body.body_id))
        {
            b3DestroyBody(body.body_id);
        }
    }

    ColliderHandle PhysicsScene::CreateSphereCollider(
        BodyHandle attached_body, const glm::vec3& position, float radius, const char* debug_name
    )
    {
        b3Sphere sphere{ { position.x, position.y, position.z }, radius };
        b3ShapeDef shape_def = b3DefaultShapeDef();
        shape_def.name = debug_name;
        shape_def.density = 1.0f;
        shape_def.enableContactEvents = true;
        shape_def.enableHitEvents = true;
        b3ShapeId shape_id = b3CreateSphereShape(attached_body.body_id, &shape_def, &sphere);
        return { shape_id };
    }

    ColliderHandle PhysicsScene::CreateBoxCollider(
        BodyHandle attached_body, const BodyCreateParams& params, const glm::vec3& half_extents, const char* debug_name
    )
    {
        const glm::vec3& pos = params.position;
        const glm::quat& rot = params.rotation;
        // TODO: implement scale

        b3Transform transform = b3Transform_identity;
        transform.p = { pos.x, pos.y, pos.z };
        transform.q = { { rot.x, rot.y, rot.z }, rot.w };
        b3BoxHull hull = b3MakeTransformedBoxHull(half_extents.x, half_extents.y, half_extents.z, transform);
        b3ShapeDef shape_def = b3DefaultShapeDef();
        shape_def.name = debug_name;
        shape_def.density = 1.0f;
        shape_def.enableContactEvents = true;
        shape_def.enableHitEvents = true;
        b3ShapeId shape_id = b3CreateHullShape(attached_body.body_id, &shape_def, &hull.base);
        return { shape_id };
    }

    void PhysicsScene::DestroyCollider(ColliderHandle collider)
    {
        constexpr bool update_mass = true;
        if (b3Shape_IsValid(collider.shape_id))
        {
            b3DestroyShape(collider.shape_id, update_mass);
        }
    }

    void PhysicsScene::GetBodyTransform(BodyHandle body, glm::vec3* position, glm::quat* rotation)
    {
        b3Transform xform = b3Body_GetTransform(body.body_id);
        if (position != nullptr)
        {
            position->x = xform.p.x;
            position->y = xform.p.y;
            position->z = xform.p.z;
        }

        if (rotation != nullptr)
        {
            rotation->x = xform.q.v.x;
            rotation->y = xform.q.v.y;
            rotation->z = xform.q.v.z;
            rotation->w = xform.q.s;
        }
    }

    void PhysicsScene::Step()
    {
        if (m_paused)
        {
            return;
        }

        constexpr int substep_count = 4;
        b3World_Step(m_world, 1.0f / m_frequency, substep_count);
    }
} // namespace Physics