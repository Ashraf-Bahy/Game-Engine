#include "physics.hpp"
#include "../physics/physics-utils.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include "../ecs/transform.hpp"
#include <BulletDynamics/Character/btKinematicCharacterController.h>

#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

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

        // to make the character collide with the static objects
        dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(
            new btGhostPairCallback());

        // 1. Create a collision shape (e.g., capsule)
        btCollisionShape *capsule = new btCapsuleShapeZ(0.5f, 2.0f);

        // 2. Create a ghost object
        playerGhost = new btPairCachingGhostObject();
        playerGhost = playerGhost; // Store the player ghost object for later use
        playerGhost->setCollisionShape(capsule);
        playerGhost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);

        btTransform startTransform;
        startTransform.setIdentity();
        // startTransform.setOrigin(btVector3(0, 30, 0)); // Spawn at (x=0, y=10, z=0)

        playerGhost->setWorldTransform(startTransform); // Apply position

        // 3. Create the kinematic character controller
        characterController = new btKinematicCharacterController(
            playerGhost, (btConvexShape *)capsule, 0.35f /*step height*/, btVector3(0, 1, 0));
        characterController->setGravity(btVector3(0, -9.81f, 0)); // Set gravity for the character controller

        // 4. Add to the world
        dynamicsWorld->addCollisionObject(playerGhost,
                                          btBroadphaseProxy::CharacterFilter,
                                          btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);
        dynamicsWorld->addAction(characterController); // Important!
        dynamicsWorld->updateSingleAabb(playerGhost);  // Update AABB for the ghost object

        // Create rigid bodies for entities with a MeshRendererComponent
        for (auto &entity : world->getEntities())
        {
            MeshRendererComponent *meshComponent = entity->getComponent<MeshRendererComponent>();
            if (!meshComponent)
                continue;

            // for the rigiid bodies of the system that will not move
            if (!meshComponent->dynamic)
            {
                const bool USE_QUANTIZED_AABB_COMPRESSION = true;
                meshComponent->shape = new btBvhTriangleMeshShape(meshComponent->triangleMesh, USE_QUANTIZED_AABB_COMPRESSION);

                btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(
                    0, // Mass (0 = static object)
                    physics_utils::prepareMotionStateEntity(entity),
                    meshComponent->shape);

                btRigidBody *rigidBody = new btRigidBody(rigidBodyCI);
                rigidBody->setUserPointer(entity);
                rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);

                dynamicsWorld->addRigidBody(rigidBody, btBroadphaseProxy::StaticFilter, btBroadphaseProxy::AllFilter);
                rigidBodies[entity->id] = rigidBody;
                meshComponent->bulletBody = rigidBody; // Store the rigid body in the component for later use
            }
            else // this object will move (dynamic) like demons
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

    // transform the player to the new position (redundant function)
    // redundant as we replaced controlling the pyshics from the ECS with fr the character
    // fully controlling the ECS from the physics system (as it is more easy for handling collisions)
    unsigned int PhysicsSystem::moveCharacter(World *world, float deltaTime)
    {
        Entity *player = world->getEntity("player");

        // Sync rotation (yaw only) to Bullet
        float yaw = player->localTransform.rotation.y;
        btTransform ghostTransform = playerGhost->getWorldTransform();
        ghostTransform.setRotation(btQuaternion(btVector3(0, 1, 0), yaw)); // Y-axis rotation
        playerGhost->setWorldTransform(ghostTransform);

        // Step the simulation
        dynamicsWorld->stepSimulation(deltaTime, 7);

        // Sync ECS position from Bullet
        btTransform newTransform = playerGhost->getWorldTransform();
        btVector3 newPos = newTransform.getOrigin();
        player->localTransform.position = glm::vec3(newPos.x(), newPos.y(), newPos.z());

        return 0;
    }

    void PhysicsSystem::updateCharacterMovement(World *world, FreeCameraControllerSystem &controllerSystem, float deltaTime)
    {
        if (controllerSystem.hasMovementUpdate())
        {
            // Convert displacement to Bullet's coordinate system
            glm::vec3 displacement = controllerSystem.getPendingDisplacement();
            characterController->setWalkDirection(btVector3(
                displacement.x,
                displacement.y,
                displacement.z));
            controllerSystem.resetMovement();
        }

        // Step simulation
        dynamicsWorld->stepSimulation(deltaTime);

        // Update ECS position from Bullet
        btTransform transform;
        transform = characterController->getGhostObject()->getWorldTransform();
        btVector3 origin = transform.getOrigin();

        Entity *character = world->getEntity("player");
        character->localTransform.position = glm::vec3(
            origin.x(),
            origin.y(),
            origin.z());
    }

    void PhysicsSystem::syncTransforms(Entity *entity, Transform *transform)
    {

        MeshRendererComponent *meshComponent = entity->getComponent<MeshRendererComponent>();

        if (meshComponent && meshComponent->dynamic)
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
        else if (meshComponent && (meshComponent->name == "cube"))
        {
            // Update physics from ECS
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
            dynamicsWorld->updateSingleAabb(playerGhost); // Update AABB for the ghost object
            // moveCharacter(world, deltaTime);
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

}