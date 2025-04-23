#include "physics.hpp"
#include "../physics/physics-utils.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include "../ecs/transform.hpp"

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

            // for the rigiid bodies of the system that will not move
            if (meshComponent->moving == false)
            {
                // create the shape
                // NOTE: we must track this pointer and delete it when all btCollisionObjects that use it are done with it!
                const bool USE_QUANTIZED_AABB_COMPRESSION = true;
                meshComponent->mesh->shape = new btBvhTriangleMeshShape(meshComponent->mesh->data, USE_QUANTIZED_AABB_COMPRESSION);
                btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(
                    0, // Mass (0 = static object)
                    physics_utils::prepareMotionStateEntity(entity),
                    meshComponent->mesh->shape);
                btRigidBody *rigidBody = new btRigidBody(rigidBodyCI);
                rigidBody->setUserPointer(entity);
                dynamicsWorld->addRigidBody(rigidBody);
                rigidBodies[entity->id] = rigidBody;
                meshComponent->bulletBody = rigidBody; // Store the rigid body in the component for later use
            }
            // else if (meshComponent->name == "demon")
            // {
            //     // Demon is a ghost object (no physical collision, just detection)
            //     btGhostObject *ghost = new btGhostObject();
            //     ghost->setCollisionShape(new btCapsuleShape(0.5f, 1.0f)); // Adjust as needed
            //     ghost->setWorldTransform(physics_utils::getEntityWorldTransform(entity));
            //     ghost->setUserPointer(entity);

            //     // Set collision flags to make it a ghost
            //     ghost->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);

            //     dynamicsWorld->addCollisionObject(ghost);
            //     // ghostObjects[entity->id] = ghost; // Store if needed
            // }
            else // this object will move
            {
                Transform *transform = &entity->localTransform;
                btTransform btTrans;
                btTrans.setIdentity();
                btTrans.setOrigin(btVector3(
                    transform->position.x,
                    transform->position.y,
                    transform->position.z));

                btMotionState *motionState = new btDefaultMotionState(btTrans);
                // 🟣 Create a sphere shape for the monkey
                btScalar radius = 2.0f; // Adjust to match your model size
                btCollisionShape *shape = new btSphereShape(radius);
                btConvexHullShape *convexShape = new btConvexHullShape();
                // 🏋️ Mass and inertia for dynamics
                btScalar mass = 1.0f;
                btVector3 inertia(0, 0, 0);
                shape->calculateLocalInertia(mass, inertia);

                // ⚙️ Create the rigid body
                btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
                btRigidBody *body = new btRigidBody(rigidBodyCI);

                // 📌 Optional: set friction, damping, etc., if needed
                // body->setFriction(1.0f);
                // body->setDamping(0.1f, 0.1f);

                // 🧭 Useful for identifying the entity during collision callbacks
                body->setUserPointer(entity);

                // ➕ Add it to the world
                dynamicsWorld->addRigidBody(body);
                rigidBodies[entity->id] = body; // Store for updates
                meshComponent->bulletBody = body;
            }
        }
    }

    void PhysicsSystem::processEntities(World *world)
    {
        for (auto entity : world->getEntities())
        {

            Transform *transform = &entity->localTransform;
            if (transform)
                syncTransforms(entity, transform);
        }
    }

    void PhysicsSystem::syncTransforms(Entity *entity, Transform *transform)
    {

        MeshRendererComponent *meshComponent = entity->getComponent<MeshRendererComponent>();

        if (meshComponent && (meshComponent->name == "monkey" || meshComponent->name == "sphere"))
        {
            // Update ECS from physics
            btTransform trans;
            meshComponent->bulletBody->getMotionState()->getWorldTransform(trans);
            meshComponent->bulletBody->activate();
            transform->position = glm::vec3(
                trans.getOrigin().x(),
                trans.getOrigin().y(),
                trans.getOrigin().z());
            auto rotation = trans.getRotation();
            auto quat = glm::quat(
                rotation.w(),
                rotation.x(),
                rotation.y(),
                rotation.z());
            // Convert quaternion to Euler angles (pitch, yaw, roll)
            transform->rotation = glm::eulerAngles(quat);
        }
    }

    void PhysicsSystem::update(World *world, float deltaTime)
    {
        // Step the physics simulation

        if (dynamicsWorld)
        {
            dynamicsWorld->stepSimulation(deltaTime);
            processEntities(world);
        }
        else
        {
            printf("[ERROR] CollisionSystem::stepSimulation: physicsWorld is null!\n");
        }
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