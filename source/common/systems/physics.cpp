#include "physics.hpp"
#include "../physics/physics-utils.hpp"

namespace our
{

    void PhysicsSystem::initialize(World *world)
    {
        // Initialize Bullet Physics components
        broadphase = new btDbvtBroadphase();
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);
        solver = new btSequentialImpulseConstraintSolver();
        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

        // ghost body (act as the main player)
        playerGhost = new btGhostObject();
        playerGhost->setCollisionShape(new btSphereShape(1.0f));
        playerGhost->setUserPointer((void *)0);
        // map_ids[0] = "GHOST";

        dynamicsWorld->addCollisionObject(playerGhost);

        // Create rigid bodies for entities with a MeshRendererComponent
        for (auto &entity : world->getEntities())
        {
            MeshRendererComponent *meshComponent = entity->getComponent<MeshRendererComponent>();
            if (!meshComponent)
                continue;

            btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(
                0, // Mass (0 = static object)
                physics_utils::prepareMotionStateEntity(entity),
                meshComponent->mesh->shape);
            btRigidBody *rigidBody = new btRigidBody(rigidBodyCI);
            rigidBody->setUserPointer(entity);
            dynamicsWorld->addRigidBody(rigidBody);
            rigidBodies[entity->id] = rigidBody;
        }
    }

    void PhysicsSystem::update(float deltaTime)
    {
        // Step the physics simulation
        dynamicsWorld->stepSimulation(deltaTime, 10);
    }

    void PhysicsSystem::destroy()
    {
        // Clean up Bullet Physics resources
        for (auto &[id, body] : rigidBodies)
        {
            dynamicsWorld->removeRigidBody(body);
            delete body->getCollisionShape();
            delete body;
        }
        delete dynamicsWorld;
        delete solver;
        delete dispatcher;
        delete collisionConfiguration;
        delete broadphase;
    }

    bool PhysicsSystem::checkCollision(const glm::vec3 &box1_min, const glm::vec3 &box1_max,
                                       const glm::vec3 &box2_min, const glm::vec3 &box2_max)
    {
        // Check along x-axis
        if (box1_max.x < box2_min.x || box2_max.x < box1_min.x)
        {
            return false;
        }

        // Check along y-axis
        if (box1_max.y < box2_min.y || box2_max.y < box1_min.y)
        {
            return false;
        }

        // Check along z-axis
        if (box1_max.z < box2_min.z || box2_max.z < box1_min.z)
        {
            return false;
        }

        // The two boxes are colliding
        return true;
    }

}