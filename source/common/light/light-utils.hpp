#pragma once

#include "../shader/shader.hpp"
#include "../components/light.hpp"
#include <vector>
namespace our::light_utils
{
    /*
        take shader of lit material and set all lights parameters to shader
    */
    void setLightParameters(ShaderProgram *shader, std::vector<LightSource> lights)
    {
        shader->set("num_lights", (int)lights.size());

        // Set the light parameters in the shader
        for (int i = 0; i < lights.size(); i++)
        {
            LightComponent *light = lights[i].light;

            shader->set("lights[" + std::to_string(i) + "].position", lights[i].position);
            shader->set("lights[" + std::to_string(i) + "].direction", lights[i].direction);
            shader->set("lights[" + std::to_string(i) + "].type", (int)lights[i].type);
            shader->set("lights[" + std::to_string(i) + "].ambient", light->ambient);
            shader->set("lights[" + std::to_string(i) + "].diffuse", light->diffuse);
            shader->set("lights[" + std::to_string(i) + "].specular", light->specular);
            shader->set("lights[" + std::to_string(i) + "].outerCutOff", glm::cos(glm::radians(light->outerCutOff)));
            shader->set("lights[" + std::to_string(i) + "].cutOff", glm::cos(glm::radians(light->cutOff)));
            shader->set("lights[" + std::to_string(i) + "].constant", light->attenuationConstant);
            shader->set("lights[" + std::to_string(i) + "].linear", light->attenuationLinear);
            shader->set("lights[" + std::to_string(i) + "].quadratic", light->attenuationQuadratic);
        }
    }
}