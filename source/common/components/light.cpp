#include "light.hpp"
#include <glm/glm.hpp>
#include "../deserialize-utils.hpp"

namespace our
{
    void LightComponent::deserialize(const nlohmann::json &data)
    {
        if (!data.is_object())
            return;

        // Deserialize the type of the light
        std::string typeStr = data.value("lightType", "Directional");
        if (typeStr == "Directional")
        {
            type = Type::Directional;
        }
        else if (typeStr == "Point")
        {
            type = Type::Point;
        }
        else if (typeStr == "Spot")
        {
            type = Type::Spot;
        }

        // Deserialize the light colors
        ambient = data.value("ambient", glm::vec3(0.0f, 0.0f, 0.0f));
        diffuse = data.value("diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
        specular = data.value("specular", glm::vec3(1.0f, 1.0f, 1.0f));
        direction = data.value("direction", glm::vec3(0.0f, 0.0f, -1.0f));

        // Deserialize the spot cutoff angles
        cutOff = data.value("cutOff", 12.5f);
        outerCutOff = data.value("outerCutOff", 15.0f);

        // Deserialize the attenuation factors
        attenuationConstant = data.value("attenuationConstant", 1.0f);
        attenuationLinear = data.value("attenuationLinear", 0.09f);
        attenuationQuadratic = data.value("attenuationQuadratic", 0.032f);
    }
}