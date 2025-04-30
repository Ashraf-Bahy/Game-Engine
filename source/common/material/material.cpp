#include "material.hpp"

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"

namespace our
{

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const
    {
        // TODO: (Req 7) Write this function

        pipelineState.setup();
        shader->use();
    }

    // This function read the material data from a json object
    void Material::deserialize(const nlohmann::json &data)
    {
        if (!data.is_object())
            return;

        if (data.contains("pipelineState"))
        {
            pipelineState.deserialize(data["pipelineState"]);
        }
        shader = AssetLoader<ShaderProgram>::get(data["shader"].get<std::string>());
        transparent = data.value("transparent", false);
    }

    // This function should call the setup of its parent and
    // set the "tint" uniform to the value in the member variable tint
    void TintedMaterial::setup() const
    {
        // TODO: (Req 7) Write this function

        Material::setup();
        shader->set("tint", tint);
    }

    // This function read the material data from a json object
    void TintedMaterial::deserialize(const nlohmann::json &data)
    {
        Material::deserialize(data);
        if (!data.is_object())
            return;
        tint = data.value("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // This function should call the setup of its parent and
    // set the "alphaThreshold" uniform to the value in the member variable alphaThreshold
    // Then it should bind the texture and sampler to a texture unit and send the unit number to the uniform variable "tex"
    void TexturedMaterial::setup() const
    {
        // Call the setup of the parent (TintedMaterial)
        TintedMaterial::setup();

        // Set the "alphaThreshold" uniform
        shader->set("alphaThreshold", alphaThreshold);

        // Bind the texture and sampler to a texture unit
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->getOpenGLName());
        glBindSampler(0, sampler->getOpenGLName());

        // Send the texture unit number to the uniform variable "tex"
        shader->set("tex", 0);
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json &data)
    {
        TintedMaterial::deserialize(data);
        if (!data.is_object())
            return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
    }

    void LitMaterial::setup() const
    {
        Material::setup();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedo->getOpenGLName());
        glBindSampler(0, sampler->getOpenGLName());
        shader->set("material.albedo", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specular->getOpenGLName());
        glBindSampler(1, sampler->getOpenGLName());
        shader->set("material.specular", 1);

        shader->set("material.shininess", shininess);
    }

    void LitMaterial::deserialize(const nlohmann::json &data)
    {
        Material::deserialize(data);
        if (!data.is_object())
            return;
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));

        albedo = AssetLoader<Texture2D>::get(data.value("albedo", ""));
        specular = AssetLoader<Texture2D>::get(data.value("specular", ""));
        shininess = data.value("shininess", 32.0f);
    }

    void AdvancedLitMaterial::setup() const
    {
        Material::setup();
        shader->set("material.albedoMap", 0);
        shader->set("material.normalMap", 1);
        shader->set("material.roughnessMap", 2);
        shader->set("material.aoMap", 3);
        shader->set("material.metallicMap", 4);
        shader->set("material.emissiveMap", 5);
        shader->set("material.IOR", IOR);

        glActiveTexture(GL_TEXTURE0);
        albedoMap->bind();
        sampler->bind(0);

        if (normalMap)
        {
            shader->set("material.useNormalMap", true);
            glActiveTexture(GL_TEXTURE1);
            normalMap->bind();
            sampler->bind(1);
        }
        else
        {
            shader->set("material.useNormalMap", false);
        }

        if (roughnessMap)
        {
            shader->set("material.useRoughnessMap", true);
            glActiveTexture(GL_TEXTURE2);
            roughnessMap->bind();
            sampler->bind(2);
        }
        else
        {
            shader->set("material.useRoughnessMap", false);
        }

        if (aoMap)
        {
            shader->set("material.useAOMap", true);
            glActiveTexture(GL_TEXTURE3);
            aoMap->bind();
            sampler->bind(3);
        }
        else
        {
            shader->set("material.useAOMap", false);
        }

        if (metallicMap)
        {
            shader->set("material.useMetallicMap", true);
            glActiveTexture(GL_TEXTURE4);
            metallicMap->bind();
            sampler->bind(4);
        }
        else
        {
            shader->set("material.useMetallicMap", false);
        }

        if (emissiveMap)
        {
            shader->set("material.useEmissiveMap", true);
            glActiveTexture(GL_TEXTURE5);
            emissiveMap->bind();
            sampler->bind(5);
        }
        else
        {
            shader->set("material.useEmissiveMap", false);
        }
    }

    void AdvancedLitMaterial::deserialize(const nlohmann::json &data)
    {
        Material::deserialize(data);
        if (!data.is_object())
            return;
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));

        albedoMap = AssetLoader<Texture2D>::get(data.value("albedoMap", ""));
        normalMap = AssetLoader<Texture2D>::get(data.value("normalMap", ""));
        roughnessMap = AssetLoader<Texture2D>::get(data.value("roughnessMap", ""));
        aoMap = AssetLoader<Texture2D>::get(data.value("aoMap", ""));
        metallicMap = AssetLoader<Texture2D>::get(data.value("metallicMap", ""));
        emissiveMap = AssetLoader<Texture2D>::get(data.value("emissiveMap", ""));
        IOR = data.value("IOR", 0.04);
    }
}