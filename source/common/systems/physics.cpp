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

    class HitMarker
    {
    public:
        Entity *entity;
        float timer = 0.0f;
    };

    void PhysicsSystem::initialize(World *world, glm::ivec2 windowSize)
    {
        // Initialize Bullet Physics components
        ourWorld = world;
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

        playerGhost->setCollisionFlags(
            btCollisionObject::CF_CHARACTER_OBJECT |
            btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        // 4. Add to the world
        dynamicsWorld->addCollisionObject(playerGhost,
                                          btBroadphaseProxy::CharacterFilter,
                                          btBroadphaseProxy::StaticFilter | // Mask
                                              btBroadphaseProxy::CharacterFilter);
        // but now this does not collide with the dynamic objects
        // it only collide with static and kinematic objects
        Entity *player = world->getEntity("player");
        playerGhost->setUserPointer(player);
        dynamicsWorld->addAction(characterController); // Important!
        dynamicsWorld->updateSingleAabb(playerGhost);  // Update AABB for the ghost object

        // Initialize audio system
        if (!audioSystem.initialize())
        {
            printf("[ERROR] Failed to initialize audio system!\n");
        }
        else
        {
            printf("[INFO] Audio system initialized successfully.\n");

            // Load audio files
            // Note: You need to add appropriate WAV and MP3 files to these directories
            audioSystem.loadSound("bullet_shoot", "assets/audio/sfx/bullet_shoot.mp3", AudioType::SOUND_EFFECT);
            audioSystem.loadSound("bullet_impact", "assets/audio/sfx/bullet_impact.mp3", AudioType::SOUND_EFFECT);
            audioSystem.loadSound("demon_growl", "assets/audio/voice/demon_growl.mp3", AudioType::VOICE);
            audioSystem.loadSound("demon_death", "assets/audio/voice/demon_death.mp3", AudioType::VOICE);
            audioSystem.loadSound("narration_intro", "assets/audio/voice/narration_intro.mp3", AudioType::VOICE);

            audioSystem.loadSound("health_damage", "assets/audio/sfx/health_damage.mp3", AudioType::SOUND_EFFECT);
            audioSystem.loadSound("player_death", "assets/audio/sfx/player_death.mp3", AudioType::SOUND_EFFECT);
            audioSystem.loadSound("background", "assets/audio/music/background.mp3", AudioType::MUSIC);

            // Play introduction narration
            audioSystem.playVoice("narration_intro");

            // Start background music
            audioSystem.playMusic("background", -1); // -1 means loop indefinitely
        }

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
            else // this object will move (dynamic)
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
            if (!(obj->getCollisionFlags() & btCollisionObject::CF_STATIC_OBJECT))
                dynamicsWorld->debugDrawObject(obj->getWorldTransform(), obj->getCollisionShape(), color);
        }
        debugDrawer->flushLines(world);
    }

    void PhysicsSystem::updateSpawning(World *world, float deltaTime)
    {
        if (spawnInterval <= 0)
            return; // Spawning disabled

        spawnTimer += deltaTime;
        if (spawnTimer >= spawnInterval)
        {
            spawnTimer = 0.0f;

            // Count active demons
            int activeCount = 0;
            for (auto entity : world->getEntities())
            {
                if (entity->getComponent<DemonComponent>() && entity->enabled)
                {
                    activeCount++;
                }
            }

            // Spawn if under limit
            glm::vec3 spawnPos;
            if (activeCount == 0)
            {
                spawnPos = {0.0f, 10.0f, 10.0f}; // Spawn at the origin
            }
            else
            {
                spawnPos = {0.0f, -10.0f, -10.0f}; // Random spawn position
            }

            if (activeCount <= maxActiveDemons && demonPool.size())
            {
                // glm::vec3 spawnPos = {
                //     rand() % 20 - 10.0f, // -10 to 10
                //     0.0f,
                //     rand() % 20 - 10.0f};

                // glm::vec3 spawnPos = {
                //     0.0f, // -10 to 10
                //     0.0f,
                //     0.0f};
                printf("Spawning demon at: %f, %f, %f\n", spawnPos.x, spawnPos.y, spawnPos.z);
                spawnDemon(spawnPos, defaultTarget, world);
            }
        }
    }

    void PhysicsSystem::update(World *world, float deltaTime, Application *app)
    {
        // Step the physics simulation
        if (dynamicsWorld)
        {
            dynamicsWorld->stepSimulation(deltaTime);

            dynamicsWorld->updateAabbs();             // Updates all bounding boxes
            dynamicsWorld->computeOverlappingPairs(); // Rebuilds collision pairs

            processEntities(world);
            dynamicsWorld->updateSingleAabb(playerGhost); // Update AABB for the ghost object
            // moveCharacter(world, deltaTime);
            fireBullet(world, app, deltaTime);

            // Update autoo demons movement
            int i = 0;
            for (auto entity : world->getEntities())
            {
                if (auto demon = entity->getComponent<DemonComponent>())
                {
                    if (entity->enabled)
                    {
                        if (auto mesh = entity->getComponent<MeshRendererComponent>())
                        {
                            if (mesh->ghostObject)
                            {
                                dynamicsWorld->updateSingleAabb(mesh->ghostObject);
                            }
                        }
                        updateDemonMovement(entity, deltaTime);
                        demon->update(deltaTime);
                    }
                }
            }

            checkPlayerDemonCollisions(world);
            Entity *player = world->getEntity("player");
            auto health = player->getComponent<HealthComponent>();

            health->update(deltaTime);
            if (health->currentHealth <= 0)
            {
                // Play player death sound
                audioSystem.playSound("player_death");
                app->changeState("menu");
            }
            checkDemonProximity(world, deltaTime);
        }
        else
        {
            printf("[ERROR] CollisionSystem::stepSimulation: physicsWorld is null!\n");
        }
    }

    bool PhysicsSystem::raycast(const glm::vec3 &start, const glm::vec3 &end,
                                Entity *&hitEntity, glm::vec3 &hitPoint, glm::vec3 &hitNormal)
    {
        if (!dynamicsWorld)
            return false;

        btVector3 btStart(start.x, start.y, start.z);
        btVector3 btEnd(end.x, end.y, end.z);

        // Configure raycast query
        btCollisionWorld::ClosestRayResultCallback rayCallback(btStart, btEnd);
        dynamicsWorld->rayTest(btStart, btEnd, rayCallback);

        if (rayCallback.hasHit())
        {
            hitPoint = glm::vec3(
                rayCallback.m_hitPointWorld.x(),
                rayCallback.m_hitPointWorld.y(),
                rayCallback.m_hitPointWorld.z());
            hitNormal = glm::vec3(
                rayCallback.m_hitNormalWorld.x(),
                rayCallback.m_hitNormalWorld.y(),
                rayCallback.m_hitNormalWorld.z());

            // Get the entity associated with the hit object
            // auto *hitBody = btRigidBody::upcast(rayCallback.m_collisionObject);
            // if (hitBody && hitBody->getUserPointer())
            // {
            //     Entity *hitEntity = static_cast<Entity *>(hitBody->getUserPointer());
            //     return true;
            // }

            if (rayCallback.m_collisionObject->getUserPointer())
            {
                hitEntity = static_cast<Entity *>(rayCallback.m_collisionObject->getUserPointer());
                return true;
            }
        }
        return false;
    }

    void PhysicsSystem::fireBullet(World *world, Application *app, float deltaTime)
    {
        // Apply cooldown
        fireCooldown -= deltaTime;
        if (fireCooldown > 0)
            return;

        // Check for 'B' key press
        Entity *cursor = world->getEntity("hit_cursor");
        if (!cursor)
            return;

        if (app->getKeyboard().isPressed(GLFW_KEY_B))
        {
            if (fireCooldown <= 0)
            {
                // Play bullet shooting sound
                audioSystem.playSound("bullet_shoot");

                Entity *player = world->getEntity("player");
                glm::vec3 start = player->localTransform.position;
                glm::vec3 forward = player->localTransform.getFront();
                glm::vec3 end = start + forward * 100.0f;

                Entity *hitEntity = nullptr;
                glm::vec3 hitPoint, hitNormal;

                if (raycast(start, end, hitEntity, hitPoint, hitNormal))
                {
                    printf("entity name: %s\n", hitEntity->name.c_str());

                    // Play bullet impact sound
                    audioSystem.playSound("bullet_impact");

                    printf("hit point: %f %f %f\n", hitPoint.x, hitPoint.y, hitPoint.z);
                    if (hitEntity->name == "demon")
                    {
                        // Play demon death sound when monkey (demon) is hit
                        audioSystem.playVoice("demon_death");

                        // Update cursor position and make it face the camera (billboard effect)
                        auto cursorTransform = cursor->localTransform;
                        cursorTransform.position = hitPoint + hitNormal * 0.01f;

                        // Simple billboard effect (always face camera)
                        glm::vec3 toCamera = glm::normalize(
                            player->localTransform.position - cursorTransform.position);
                        cursorTransform.rotation = glm::eulerAngles(
                            glm::quatLookAt(toCamera, glm::vec3(0, 1, 0)));

                        cursor->localTransform = cursorTransform;
                        if (auto mesh = hitEntity->getComponent<MeshRendererComponent>())
                        {
                            if (mesh->characterController)
                            {
                                // Character controller
                                dynamicsWorld->removeAction(mesh->characterController);
                                dynamicsWorld->removeCollisionObject(mesh->ghostObject);

                                delete mesh->characterController;
                                delete mesh->ghostObject->getCollisionShape();
                                delete mesh->ghostObject;

                                mesh->characterController = nullptr;
                                mesh->ghostObject = nullptr;
                            }
                        }
                        printf("Hit entity: sucecss\n");
                        world->markForRemoval(hitEntity);
                        printf("Hit entity: %s\n", hitEntity->name.c_str());
                        world->deleteMarkedEntities();
                    }
                }
                else
                {
                    // Hide cursor when not hitting anything
                    cursor->localTransform.position = glm::vec3(0, 0, -100);
                    printf("did not Hit entity: sucecss\n");
                }
                fireCooldown = FIRE_RATE;
            }
        }
    }

    void PhysicsSystem::checkDemonProximity(World *world, float deltaTime)
    {
        // Get player entity
        Entity *player = world->getEntity("player");
        if (!player)
            return;

        glm::vec3 playerPos = player->localTransform.position;

        // Check for demons (monkeys) nearby
        static std::unordered_map<unsigned int, float> growlCooldowns;
        const float GROWL_COOLDOWN = 5.0f;  // Don't growl again for 5 seconds
        const float GROWL_DISTANCE = 15.0f; // Distance at which demons growl

        for (auto entity : world->getEntities())
        {
            // Skip if this isn't a demon (monkey)
            if (entity->name != "monkey")
                continue;

            // Calculate distance to player
            float distance = glm::length(entity->localTransform.position - playerPos);

            // Check if this demon is close enough and not on cooldown
            auto it = growlCooldowns.find(entity->id);
            bool onCooldown = (it != growlCooldowns.end() && it->second > 0);

            if (distance <= GROWL_DISTANCE && !onCooldown)
            {
                // Play growl sound
                audioSystem.playVoice("demon_growl");

                // Set cooldown
                growlCooldowns[entity->id] = GROWL_COOLDOWN;

                // Debug info
                printf("Demon at distance %.2f growled at player\n", distance);
            }
        }

        // Update cooldowns
        for (auto &[id, cooldown] : growlCooldowns)
        {
            cooldown -= deltaTime;
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
        audioSystem.destroy();
    }

    // Initialize demon system
    void PhysicsSystem::initializeDemons(World *world, int poolSize)
    {
        Entity *demonTemplate = world->getEntity("demon_template"); // Load from JSON
        // Pre-create demons with physics bodies
        for (int i = 0; i < poolSize; i++)
        {
            Entity *demon = cloneDemon(demonTemplate, world);
            printf("intializing Demon %d: %s\n", i, demon->name.c_str());
            demon->enabled = false;
            initializeDemonPhysics(demon);
            demonPool.push_back(demon);
        }
    }

    // Spawn a demon at position
    Entity *PhysicsSystem::spawnDemon(glm::vec3 position, glm::vec3 target, World *world)
    {
        printf("spawning demon at %f %f %f\n", position.x, position.y, position.z);
        // if (demonPool.empty())
        // {
        //     // Create more if pool is empty
        //     Entity *demon = cloneDemon(demonTemplate, world);
        //     initializeDemonPhysics(demon);
        //     demonPool.push_back(demon);
        // }
        if (demonPool.empty())
            return nullptr;

        Entity *demon = demonPool.back();
        demonPool.pop_back();

        // Setup demon
        demon->localTransform.position = position;
        if (auto dc = demon->getComponent<DemonComponent>())
        {
            dc->targetPosition = target;
            dc->health = dc->maxHealth;
        }
        demon->enabled = true;

        // Reactivate physics
        if (auto mesh = demon->getComponent<MeshRendererComponent>())
        {
            if (mesh->bulletBody)
            {
                mesh->bulletBody->activate();
                mesh->bulletBody->setWorldTransform(
                    btTransform(btQuaternion::getIdentity(),
                                btVector3(position.x, position.y, position.z)));
            }
        }

        return demon;
    }

    // Return demon to pool
    void PhysicsSystem::returnDemon(Entity *demon)
    {
        demon->enabled = false;

        if (auto mesh = demon->getComponent<MeshRendererComponent>())
        {
            if (mesh->characterController)
            {
                // Remove from world
                dynamicsWorld->removeAction(mesh->characterController);
                dynamicsWorld->removeCollisionObject(mesh->ghostObject);

                // Clean up memory
                delete mesh->characterController;
                delete mesh->ghostObject->getCollisionShape();
                delete mesh->ghostObject;

                mesh->characterController = nullptr;
                mesh->ghostObject = nullptr;
            }
        }
    }

    // Helpers to clone a demon
    Entity *PhysicsSystem::cloneDemon(Entity *original, World *world)
    {
        if (!original)
        {
            printf("trying to clone a null demon\n");
            return nullptr;
        }

        Entity *clone = world->add();
        clone->localTransform = original->localTransform;

        clone->name = "demon";

        // Clone components
        if (auto originalComp = original->getComponent<DemonComponent>())
        {
            printf("cloning demon component\n");
            auto *newComp = clone->addComponent<DemonComponent>();
            *newComp = *originalComp;
        }

        if (auto originalMesh = original->getComponent<MeshRendererComponent>())
        {
            printf("cloning demon mesh\n");
            auto *newMesh = clone->addComponent<MeshRendererComponent>();
            newMesh->mesh = originalMesh->mesh;
            newMesh->material = originalMesh->material;
            newMesh->dynamic = originalMesh->dynamic;
        }

        return clone;
    }

    // Helper to initialize physics for a demon
    void PhysicsSystem::initializeDemonPhysics(Entity *demon)
    {
        if (auto mesh = demon->getComponent<MeshRendererComponent>())
        {
            // Create capsule shape for character controller
            btConvexShape *capsule = new btCapsuleShapeZ(
                0.5f,
                2.0f);

            // Create ghost object
            mesh->ghostObject = new btPairCachingGhostObject();
            mesh->ghostObject->setCollisionShape(capsule);
            mesh->ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);

            // Set initial transform
            btTransform startTransform;
            startTransform.setIdentity();
            startTransform.setOrigin(btVector3(
                demon->localTransform.position.x,
                demon->localTransform.position.y,
                demon->localTransform.position.z));
            mesh->ghostObject->setWorldTransform(startTransform);

            // Create character controller
            mesh->characterController = new btKinematicCharacterController(
                mesh->ghostObject,
                capsule,
                0.35f);

            // Configure controller
            if (auto demonComp = demon->getComponent<DemonComponent>())
            {
                mesh->characterController->setGravity(btVector3(0, -9.81f, 0));
                mesh->characterController->setFallSpeed(55.0f);
                mesh->characterController->setJumpSpeed(demonComp->jumpSpeed);
                mesh->characterController->setMaxJumpHeight(15.0f);
            }

            // MUST SET THESE for the callllll back
            mesh->ghostObject->setCollisionFlags(
                btCollisionObject::CF_CHARACTER_OBJECT |
                btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK |
                btCollisionObject::CF_NO_CONTACT_RESPONSE); // Prevents physics push);

            // Add to world
            dynamicsWorld->addCollisionObject(mesh->ghostObject,
                                              btBroadphaseProxy::CharacterFilter,
                                              btBroadphaseProxy::AllFilter);

            mesh->ghostObject->setCollisionFlags(
                btCollisionObject::CF_CHARACTER_OBJECT |
                btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK // Enables raycasts
            );
            dynamicsWorld->addAction(mesh->characterController);

            // Store reference to entity
            mesh->ghostObject->setUserPointer(demon);
        }
    }

    void PhysicsSystem::updateDemonMovement(Entity *demon, float deltaTime)
    {
        if (auto demonComp = demon->getComponent<DemonComponent>())
        {
            if (auto mesh = demon->getComponent<MeshRendererComponent>())
            {
                if (mesh->characterController)
                {
                    // 1. Handle Jumping
                    if (demonComp->canJump() && mesh->characterController->canJump())
                    {
                        mesh->characterController->jump(btVector3(0, demonComp->jumpPower, 0));
                        demonComp->resetJumpTimer();
                    }

                    // 2. Calculate movement direction
                    glm::vec3 targetDir = glm::normalize(
                        demonComp->targetPosition - demon->localTransform.position);

                    // 3. Apply movement
                    btVector3 walkDirection(
                        targetDir.x * demonComp->moveSpeed,
                        0, // Zero Y movement - let jump handle vertical
                        targetDir.z * demonComp->moveSpeed);
                    mesh->characterController->setWalkDirection(walkDirection);

                    // 4. Update transform
                    btTransform transform;
                    if (mesh->ghostObject)
                    {
                        transform = mesh->ghostObject->getWorldTransform();
                        demon->localTransform.position = glm::vec3(
                            transform.getOrigin().x(),
                            transform.getOrigin().y(),
                            transform.getOrigin().z());

                        // Face movement direction
                        if (walkDirection.length2() > 0.1f)
                        {
                            float yaw = atan2(walkDirection.x(), walkDirection.z());
                            demon->localTransform.rotation.y = yaw;
                        }
                    }
                }
            }
        }
    }

    // handling collisions in the system
    void PhysicsSystem::checkPlayerDemonCollisions(World *world)
    {
        Entity *player = world->getEntity("player");
        if (!player)
            return;

        // Only check if cooldown expired
        float currentTime = glfwGetTime();
        if (currentTime - lastHitTime < HIT_COOLDOWN)
            return;

        DemonContactCallback callback(player, this, world);

        // Check against all active demons
        for (auto entity : world->getEntities())
        {
            if (entity->name == "demon" && entity->enabled)
            {
                // printf("entity name demon f3lan\n");
                if (auto mesh = entity->getComponent<MeshRendererComponent>())
                {
                    if (mesh->ghostObject)
                    {
                        // printf("found ghost object\n");
                        dynamicsWorld->contactPairTest(
                            playerGhost,
                            mesh->ghostObject,
                            callback);
                    }
                    else
                    {
                        printf("Demon %s has no ghost object!\n", entity->name.c_str());
                    }
                }
            }
        }
    }

}