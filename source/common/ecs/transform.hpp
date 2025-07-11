#pragma once

#include <glm/glm.hpp>
#include <json/json.hpp>
#include <glm/glm.hpp>              // Core GLM types and functions
#include <glm/gtc/quaternion.hpp>   // GLM quaternion operations
#include <glm/gtx/quaternion.hpp>   // Additional quaternion functions (for operator*)
#include <glm/gtx/euler_angles.hpp> // For Euler angle conversions (optional but recommended)

namespace our
{

    // A transform defines the translation, rotation & scale of an object relative to its parent
    struct Transform
    {
    public:
        glm::vec3 position = glm::vec3(0, 0, 0); // The position is defined as a vec3. (0,0,0) means no translation
        glm::vec3 rotation = glm::vec3(0, 0, 0); // The rotation is defined using euler angles (y: yaw, x: pitch, z: roll). (0,0,0) means no rotation
        glm::vec3 scale = glm::vec3(1, 1, 1);    // The scale is defined as a vec3. (1,1,1) means no scaling.

        glm::vec3 getFront() const
        {
            // Convert Euler angles (in radians) to a quaternion
            glm::quat rotationQuat = glm::quat(glm::vec3(rotation.x, rotation.y, rotation.z));

            // The default forward vector in local space is (0, 0, -1)
            return rotationQuat * glm::vec3(0, 0, -1);
        }

        // This function computes and returns a matrix that represents this transform
        glm::mat4 toMat4() const;
        // Deserializes the entity data and components from a json object
        void deserialize(const nlohmann::json &);
    };

}