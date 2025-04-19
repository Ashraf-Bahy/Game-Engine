#pragma once

#include <glm/glm.hpp>
#include "../ecs/component.hpp"

namespace our
{
    // This enum defines the type of the light source
    // Directional lights are infinitely far away and have a direction
    // Point lights are at a specific position in the scene and have a color
    // Spot lights are at a specific position in the scene and have a color and a direction
    enum class Type
    {
        Directional,
        Point,
        Spot
    };

    // This component represents a light source in the scene.
    class LightComponent : public Component
    {
    private:
        Type type;
        glm::vec3 ambient, specular, diffuse; // The color of the light
        glm::vec3 direction;                  // The direction of the light (for directional and spot lights)
        float cutOff;                         // spot cutoff angle in degrees
        float outerCutOff;                    // spot outer cutoff angle in degrees

        float attenuationConstant;  // The constant of the attenuation
        float attenuationLinear;    // The linear of the attenuation
        float attenuationQuadratic; // The quadratic of the attenuation

    public:
        // The ID of this component type is "Light Component"
        static std::string getID() { return "Light Component"; }

        // Receives the mesh & material from the AssetLoader by the names given in the json object
        void deserialize(const nlohmann::json &data) override;
    };

}