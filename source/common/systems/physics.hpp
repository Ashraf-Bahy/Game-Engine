#pragma once

#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <unordered_map>

namespace our
{

    class PhysicsSystem
    {
    private:
        btBroadphaseInterface *broadphase;
        btDefaultCollisionConfiguration *collisionConfiguration;
        btCollisionDispatcher *dispatcher;
        btSequentialImpulseConstraintSolver *solver;
        btDiscreteDynamicsWorld *dynamicsWorld = nullptr;

        std::unordered_map<unsigned int, btRigidBody *> rigidBodies;
        // the main player
        btCollisionObject *playerGhost;

    public:
        void initialize(World *world);
        void update(World *world, float deltaTime);
        void destroy();
        bool checkCollision(const glm::vec3 &box1_min, const glm::vec3 &box1_max,
                            const glm::vec3 &box2_min, const glm::vec3 &box2_max);

        // Sync the transforms of the entity and its collision component
        void syncTransforms(Entity *entity, Transform *transform);

        // Process all entities in the world and update their transforms and physics bodies
        void processEntities(World *world);
    };

}