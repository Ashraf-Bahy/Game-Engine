#pragma once

#include "../ecs/entity.hpp"
#include "../components/mesh-renderer.hpp"
#include <btBulletDynamicsCommon.h>

#include <iostream>

namespace our::physics_utils
{

    // get world transform of entity to world space (rotation / position)
    btTransform getEntityWorldTransform(Entity *entity)
    {
        glm::mat4 worldMat = entity->getLocalToWorldMatrix();
        // 2. Extract position from matrix (works for ANY position)
        glm::vec3 worldPos = glm::vec3(worldMat[3]);

        btVector3 btRotation = btVector3(entity->localTransform.rotation.x, entity->localTransform.rotation.y, entity->localTransform.rotation.z);

        // i think we will need to use quetr directly
        btQuaternion btRotationQuat;
        btRotationQuat.setEuler(btRotation.y(), btRotation.x(), btRotation.z());

        return btTransform(btRotationQuat, btVector3(worldPos.x, worldPos.y, worldPos.z));
    }

    // set scaling of mesh collision component to same as entity transform scaling
    void setCollisionScalingByEntity(Entity *entity)
    {
        MeshRendererComponent *meshComponent = entity->getComponent<MeshRendererComponent>();
        btVector3 btScaling = btVector3(entity->localTransform.scale.x, entity->localTransform.scale.y, entity->localTransform.scale.z);
        std::cout << "Scaling values: x=" << btScaling.x()
                  << ", y=" << btScaling.y()
                  << ", z=" << btScaling.z() << std::endl;
        meshComponent->shape->setLocalScaling(btScaling);
    }

    // prepare entity to enter to dynamic world
    btDefaultMotionState *prepareMotionStateEntity(Entity *entity)
    {
        btDefaultMotionState *motionstate = new btDefaultMotionState(getEntityWorldTransform(entity));
        setCollisionScalingByEntity(entity);

        return motionstate;
    }

}