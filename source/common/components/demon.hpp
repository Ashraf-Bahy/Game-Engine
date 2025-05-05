#pragma once
#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our
{

    class DemonComponent : public Component
    {
    public:
        // Target position the demon will move toward (e.g., player base)
        glm::vec3 targetPosition = {0.0f, 0.0f, 0.0f};

        // Movement properties
        float moveSpeed = 2.0f;      // How fast the demon moves
        float rotationSpeed = 90.0f; // Degrees per second

        // Combat properties
        float damage = 10.0f;     // Damage per second to player
        float attackRange = 1.5f; // Distance to start attacking
        float health = 100.0f;    // Current health
        float maxHealth = 100.0f; // Maximum health

        // The ID of the spawn point this demon came from
        int spawnPointId = -1;

        // Reads demon parameters from the given json object
        void deserialize(const nlohmann::json &data) override
        {
            if (!data.is_object())
                return;

            // Target position
            if (data.contains("targetPosition"))
            {
                targetPosition.x = data["targetPosition"][0];
                targetPosition.y = data["targetPosition"][1];
                targetPosition.z = data["targetPosition"][2];
            }

            // Movement
            moveSpeed = data.value("moveSpeed", moveSpeed);
            rotationSpeed = glm::radians(data.value("rotationSpeed", rotationSpeed));

            // Combat
            damage = data.value("damage", damage);
            attackRange = data.value("attackRange", attackRange);
            health = data.value("health", health);
            maxHealth = data.value("maxHealth", maxHealth);

            // Spawn info
            spawnPointId = data.value("spawnPointId", spawnPointId);
        }
    };
}