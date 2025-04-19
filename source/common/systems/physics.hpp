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
        btDiscreteDynamicsWorld *dynamicsWorld;

        std::unordered_map<unsigned int, btRigidBody *> rigidBodies;
        // the main player
        btCollisionObject *playerGhost;

    public:
        void initialize(World *world);
        void update(float deltaTime);
        void destroy();
        bool checkCollision(const glm::vec3 &box1_min, const glm::vec3 &box1_max,
                            const glm::vec3 &box2_min, const glm::vec3 &box2_max);
    };

}