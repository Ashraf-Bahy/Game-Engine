#pragma once
#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our
{
    class DemonComponent : public Component
    {
    public:
        glm::vec3 targetPosition = {0, 0, 0};
        float moveSpeed = 1.5f;
        float jumpSpeed = 0.0f; // Zero for no jumping
        float damage = 15.0f;
        float attackRange = 2.0f;
        float health = 100.0f;
        float maxHealth = 100.0f;
        float contactDamage = 10.0f;
        float damageCooldown = 1.0f;
        float currentCooldown = 0.0f;

        void update(float deltaTime)
        {
            // Update damage cooldown
            if (currentCooldown > 0)
                currentCooldown -= deltaTime;
        }

        bool canDealDamage() const { return currentCooldown <= 0; }
        void resetDamageCooldown() { currentCooldown = damageCooldown; }

        void deserialize(const nlohmann::json &data) override
        {
            // Target position
            if (data.contains("targetPosition"))
            {
                targetPosition.x = data["targetPosition"][0];
                targetPosition.y = data["targetPosition"][1];
                targetPosition.z = data["targetPosition"][2];
            }

            moveSpeed = data.value("moveSpeed", moveSpeed);
            jumpSpeed = data.value("jumpSpeed", jumpSpeed);
            damage = data.value("damage", damage);
            attackRange = data.value("attackRange", attackRange);
            health = data.value("health", health);
            maxHealth = data.value("maxHealth", maxHealth);
            contactDamage = data.value("contactDamage", contactDamage);
            damageCooldown = data.value("damageCooldown", damageCooldown);
        }

        // The ID of this component type is "Mesh Renderer"
        static std::string getID() { return "DemonComponent"; }
    };
}
