#pragma once
#include "../ecs/component.hpp"
#include <glm/glm.hpp>

namespace our
{
    class HealthComponent : public Component
    {
    public:
        int maxHealth = 100;
        int currentHealth = 100;
        float invulnerabilityTime = 0.5f; // Seconds after being hit

        bool takeDamage(int amount)
        {
            if (invulnerabilityTimer <= 0)
            {
                currentHealth -= amount;
                if (currentHealth < 0)
                    currentHealth = 0; // Clamp health to zero
                invulnerabilityTimer = invulnerabilityTime;
                return true; // Damage was applied
            }
            return false; // Damage was ignored due to invulnerability
        }

        // Update the invulnerability timer
        void update(float deltaTime)
        {
            if (invulnerabilityTimer > 0)
            {
                invulnerabilityTimer -= deltaTime;
                if (invulnerabilityTimer < 0)
                    invulnerabilityTimer = 0; // Ensure it doesn't go negative
            }
        }

        void deserialize(const nlohmann::json &data) override
        {
            maxHealth = data.value("maxHealth", maxHealth);
            currentHealth = data.value("currentHealth", maxHealth); // Default to max
            invulnerabilityTime = data.value("invulnerabilityTime", invulnerabilityTime);
        }

        static std::string getID() { return "HealthComponent"; }

    private:
        float invulnerabilityTimer = 0.0f;
    };

}