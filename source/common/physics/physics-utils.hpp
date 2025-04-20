#pragma once

#include "../ecs/entity.hpp"
#include "../components/mesh-renderer.hpp"
#include <btBulletDynamicsCommon.h>

namespace our::physics_utils
{

    // get world transform of entity to world space (rotation / position)
    btTransform getEntityWorldTransform(Entity *entity)
    {
        glm::mat3 transform = entity->getLocalToWorldMatrix();

        btVector3 btRotation = btVector3(entity->localTransform.rotation.x, entity->localTransform.rotation.y, entity->localTransform.rotation.z);
        btVector3 btPosition = btVector3(entity->localTransform.position.x, entity->localTransform.position.y, entity->localTransform.position.z);

        btQuaternion btRotationQuat;
        btRotationQuat.setEuler(btRotation.y(), btRotation.x(), btRotation.z());

        return btTransform(btRotationQuat, btPosition);
    }

    // set scaling of mesh collision component to same as entity transform scaling
    void setCollisionScalingByEntity(Entity *entity)
    {
        MeshRendererComponent *meshComponent = entity->getComponent<MeshRendererComponent>();
        btVector3 btScaling = btVector3(entity->localTransform.scale.x, entity->localTransform.scale.y, entity->localTransform.scale.z);
        meshComponent->mesh->shape->setLocalScaling(btScaling);
    }

    // prepare entity to enter to dynamic world
    btDefaultMotionState *prepareMotionStateEntity(Entity *entity)
    {
        btDefaultMotionState *motionstate = new btDefaultMotionState(getEntityWorldTransform(entity));
        setCollisionScalingByEntity(entity);

        return motionstate;
    }

}