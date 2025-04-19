#pragma once

#include "../ecs/entity.hpp"
#include "../components/mesh-renderer.hpp"
#include <btBulletDynamicsCommon.h>

namespace our::physics_utils
{

    // Prepare a motion state for an entity
    inline btDefaultMotionState *prepareMotionStateEntity(Entity *entity)
    {
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(
            entity->localTransform.position.x,
            entity->localTransform.position.y,
            entity->localTransform.position.z));
        return new btDefaultMotionState(transform);
    }

}