#include "mesh-renderer.hpp"
#include "../asset-loader.hpp"
#include <iostream>

namespace our
{
    // Receives the mesh & material from the AssetLoader by the names given in the json object
    void MeshRendererComponent::deserialize(const nlohmann::json &data)
    {
        if (!data.is_object())
            return;
        // Notice how we just get a string from the json file and pass it to the AssetLoader to get us the actual asset
        // TODO: (Req 8) Get the material and the mesh from the AssetLoader by their names
        // which are defined with the keys "mesh" and "material" in data.
        // Hint: To get a value of type T from a json object "data" where the key corresponding to the value is "key",
        // you can use write: data["key"].get<T>().
        // Look at "source/common/asset-loader.hpp" to know how to use the static class AssetLoader.
        // std::cout << "Deserializing MeshRendererComponent with data: " << data.dump(4) << std::endl;

        mesh = AssetLoader<Mesh>::get(data["mesh"].get<std::string>());
        material = AssetLoader<Material>::get(data["material"].get<std::string>());
        name = data["mesh"].get<std::string>();
        if (data.contains("dynamic"))
        {
            dynamic = data["dynamic"].get<bool>();
        }

        // Build triangle mesh from vertices/indices
        triangleMesh = new btTriangleMesh();
        for (size_t i = 0; i < mesh->cpuIndices.size(); i += 3)
        {
            auto &v0 = mesh->cpuVertices[mesh->cpuIndices[i]].position;
            auto &v1 = mesh->cpuVertices[mesh->cpuIndices[i + 1]].position;
            auto &v2 = mesh->cpuVertices[mesh->cpuIndices[i + 2]].position;

            triangleMesh->addTriangle(
                btVector3(v0.x, v0.y, v0.z),
                btVector3(v1.x, v1.y, v1.z),
                btVector3(v2.x, v2.y, v2.z));
        }
    }

}