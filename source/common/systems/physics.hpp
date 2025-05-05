#pragma once

#include "../ecs/world.hpp"
#include "../components/mesh-renderer.hpp"
#include <systems/free-camera-controller.hpp>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <unordered_map>
#include <LinearMath/btIDebugDraw.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

namespace our
{
    class GLDebugDrawer : public btIDebugDraw
    {
        glm::ivec2 windowSize;
        int m_debugMode;
        struct Line
        {
            glm::vec3 start, end;
            glm::vec3 color;
        };
        std::vector<Line> lines;
        GLuint VAO, VBO;
        ShaderProgram *lineShader = nullptr;

    public:
        GLDebugDrawer(glm::ivec2 windowSize) : m_debugMode(DBG_DrawWireframe | DBG_DrawAabb)
        {
            this->windowSize = windowSize;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            lineShader = new ShaderProgram();
            lineShader->attach("assets/shaders/debug_line.vert", GL_VERTEX_SHADER);
            lineShader->attach("assets/shaders/debug_line.frag", GL_FRAGMENT_SHADER);
            lineShader->link();
        }

        ~GLDebugDrawer()
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            delete lineShader;
        }

        void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override
        {
            lines.push_back({{from.x(), from.y(), from.z()},
                             {to.x(), to.y(), to.z()},
                             {color.x(), color.y(), color.z()}});
        }

        void flushLines(World *world)
        {
            CameraComponent *camera = nullptr;
            for (auto entity : world->getEntities())
            {
                if (!camera)
                    camera = entity->getComponent<CameraComponent>();
            }

            if (lines.empty() || camera == nullptr)
                return;
            glm::mat4 view = camera->getViewMatrix();
            glm::mat4 projection = camera->getProjectionMatrix(windowSize);
            glm::mat4 VP = projection * view;

            // Prepare data
            std::vector<float> vertexData;
            for (const auto &line : lines)
            {
                vertexData.insert(vertexData.end(), {line.start.x, line.start.y, line.start.z});
                vertexData.insert(vertexData.end(), {line.color.r, line.color.g, line.color.b});

                vertexData.insert(vertexData.end(), {line.end.x, line.end.y, line.end.z});
                vertexData.insert(vertexData.end(), {line.color.r, line.color.g, line.color.b});
            }

            // Upload to GPU
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);

            // Set vertex attributes
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0); // Position
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float))); // Color

            // Draw
            lineShader->use();
            lineShader->set("viewProjMatrix", VP);
            glDrawArrays(GL_LINES, 0, lines.size() * 2);

            // Cleanup
            glBindVertexArray(0);
            lines.clear();
        }

        void setDebugMode(int debugMode) override { m_debugMode = debugMode; }
        int getDebugMode() const override { return m_debugMode; }

        // Stubs for unused methods
        virtual void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) override {}
        void reportErrorWarning(const char *warningString) override {}
        void draw3dText(const btVector3 &location, const char *textString) override {}
    };

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
        btPairCachingGhostObject *playerGhost;
        btKinematicCharacterController *characterController;
        GLDebugDrawer *debugDrawer = nullptr;
        // std::vector<HitMarker> activeHitMarkers;
        float fireCooldown = 0.0f;
        const float FIRE_RATE = 0.5f; // Seconds between shots

        std::vector<Entity *> demonPool;
        Entity *demonTemplate = nullptr;

    public:
        void initialize(World *world, glm::ivec2 windowSize);
        void update(World *world, float deltaTime, Application *app);
        void destroy();

        // Sync the transforms of the entity and its collision component
        void syncTransforms(Entity *entity, Transform *transform);

        // Process all entities in the world and update their transforms and physics bodies
        void processEntities(World *world);

        // Debug draw the world using the debug drawer
        void debugDrawWorld(World *world);

        unsigned int moveCharacter(World *world, float deltaTime); // return mesh id that person collided with

        void updateCharacterMovement(World *world, FreeCameraControllerSystem &controllerSystem, float deltaTime);

        bool raycast(const glm::vec3 &start, const glm::vec3 &end,
                     Entity *&hitEntity, glm::vec3 &hitPoint, glm::vec3 &hitNormal);

        void fireBullet(World *world, Application *app, float deltaTime);

        // Demon management functions
        void initializeDemons(World *world, Entity *templateDemon, int poolSize = 20);
        Entity *spawnDemon(glm::vec3 position, glm::vec3 target, World *world);
        void returnDemon(Entity *demon);
        Entity *cloneDemon(Entity *original, World *world);
        void initializeDemonPhysics(Entity *demon);
    };

}