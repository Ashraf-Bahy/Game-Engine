#include "physics.hpp"
#include "../physics/physics-utils.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include "../ecs/transform.hpp"

namespace our
{

    void PhysicsSystem::initialize(World *world, glm::ivec2 windowSize)
    {
        // Initialize Bullet Physics components
        broadphase = new btDbvtBroadphase();
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);
        solver = new btSequentialImpulseConstraintSolver();
        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

        // ghost body (act as the main player)
        // playerGhost = new btGhostObject();
        // playerGhost->setCollisionShape(new btSphereShape(1.0f));
        // playerGhost->setUserPointer((void *)0);
        // map_ids[0] = "GHOST";

        // dynamicsWorld->addCollisionObject(playerGhost);

        // Create rigid bodies for entities with a MeshRendererComponent
        for (auto &entity : world->getEntities())
        {
            MeshRendererComponent *meshComponent = entity->getComponent<MeshRendererComponent>();
            if (!meshComponent)
                continue;
            // create the shape
            // NOTE: we must track this pointer and delete it when all btCollisionObjects that use it are done with it!

            // for the rigiid bodies of the system that will not move
            if (!meshComponent->moving)
            {
                const bool USE_QUANTIZED_AABB_COMPRESSION = true;
                meshComponent->shape = new btBvhTriangleMeshShape(meshComponent->triangleMesh, USE_QUANTIZED_AABB_COMPRESSION);

                // meshComponent->shape->setLocalScaling(btVector3(
                //     entity->localTransform.scale.x,
                //     entity->localTransform.scale.y,
                //     entity->localTransform.scale.z));

                btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(
                    0, // Mass (0 = static object)
                    physics_utils::prepareMotionStateEntity(entity),
                    meshComponent->shape);

                btRigidBody *rigidBody = new btRigidBody(rigidBodyCI);
                rigidBody->setUserPointer(entity);
                dynamicsWorld->addRigidBody(rigidBody);
                rigidBodies[entity->id] = rigidBody;
                meshComponent->bulletBody = rigidBody; // Store the rigid body in the component for later use
            }
            else // this object will move
            {
                // Get the mesh data from the component
                btTriangleIndexVertexArray *meshData = meshComponent->mesh->data;
                if (!meshData || meshData->getNumSubParts() == 0)
                {
                    // Handle error: no valid mesh data
                    continue;
                }

                // Access the first indexed mesh (assuming single sub-part)
                btIndexedMesh &indexedMesh = meshData->getIndexedMeshArray()[0];

                // Extract vertex data
                const unsigned char *vertexBase = indexedMesh.m_vertexBase;
                int numVertices = indexedMesh.m_numVertices;
                int vertexStride = indexedMesh.m_vertexStride; // Should be 3*sizeof(btScalar)

                // Create convex hull shape and add all vertices
                btConvexHullShape *convexShape = new btConvexHullShape();
                // Get entity scale
                glm::vec3 scale = entity->localTransform.scale;

                // Add vertices WITH SCALING APPLIED
                for (int i = 0; i < numVertices; ++i)
                {
                    const btScalar *vertex = reinterpret_cast<const btScalar *>(vertexBase + i * vertexStride);
                    convexShape->addPoint(btVector3(
                        vertex[0] * scale.x, // Apply X scale
                        vertex[1] * scale.y, // Apply Y scale
                        vertex[2] * scale.z  // Apply Z scale
                        ));
                }

                // Optional: Simplify convex hull for better performance
                btShapeHull hullSimplifier(convexShape);
                if (hullSimplifier.buildHull(convexShape->getMargin()))
                {
                    // Create simplified shape if hull is built successfully
                    btConvexHullShape *simplifiedShape = new btConvexHullShape();
                    for (int i = 0; i < hullSimplifier.numVertices(); ++i)
                    {
                        simplifiedShape->addPoint(hullSimplifier.getVertexPointer()[i]);
                    }
                    // Swap to use simplified shape
                    delete convexShape;
                    convexShape = simplifiedShape;
                }

                // Set up motion state and transform
                Transform *transform = &entity->localTransform;
                btTransform btTrans;
                btTrans.setIdentity();
                btTrans.setOrigin(btVector3(
                    transform->position.x,
                    transform->position.y,
                    transform->position.z));

                btMotionState *motionState = new btDefaultMotionState(btTrans);

                // Configure mass and inertia
                btScalar mass = 1.0f;
                btVector3 inertia(0, 0, 0);
                convexShape->calculateLocalInertia(mass, inertia);

                // Create rigid body
                btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(
                    mass,
                    motionState,
                    convexShape,
                    inertia);
                btRigidBody *body = new btRigidBody(rigidBodyCI);
                body->setUserPointer(entity);

                // Add to world and store references
                dynamicsWorld->addRigidBody(body);
                rigidBodies[entity->id] = body;
                meshComponent->bulletBody = body;
            }
        }

        debugDrawer = new GLDebugDrawer(windowSize);
        dynamicsWorld->setDebugDrawer(debugDrawer);
        if (dynamicsWorld->getDebugDrawer())
            dynamicsWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
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

    void PhysicsSystem::debugDrawWorld(World *world)
    {

        // Manually iterate through all collision objects
        auto &collisionObjects = dynamicsWorld->getCollisionObjectArray();
        for (int i = 0; i < collisionObjects.size(); ++i)
        {
            btCollisionObject *obj = collisionObjects[i];

            // Set color based on collision flags
            btVector3 color(1, 1, 1);
            if (obj->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT)
            {
                color = btVector3(0, 1, 0);
            }
            else if (obj->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT)
            {
                color = btVector3(1, 1, 0);
            }
            else if (obj->getCollisionFlags() & btCollisionObject::CF_DYNAMIC_OBJECT)
            {
                color = btVector3(0, 0, 1);
            }
            else
            {
                color = btVector3(1, 0, 0);
            }
            dynamicsWorld->debugDrawObject(obj->getWorldTransform(), obj->getCollisionShape(), color);
        }
        debugDrawer->flushLines(world);
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
        delete debugDrawer;
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