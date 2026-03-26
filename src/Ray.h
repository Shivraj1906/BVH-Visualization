#pragma once
#include <glm/glm.hpp>

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
    glm::vec3 invDir;
    int sign[3];

    Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), dir(glm::normalize(d)) {
        // Safe inverse direction to avoid division by zero issues in slab methods
        invDir = glm::vec3(
            dir.x == 0.0f ? 1e-8f : 1.0f / dir.x,
            dir.y == 0.0f ? 1e-8f : 1.0f / dir.y,
            dir.z == 0.0f ? 1e-8f : 1.0f / dir.z
        );
        sign[0] = (invDir.x < 0);
        sign[1] = (invDir.y < 0);
        sign[2] = (invDir.z < 0);
    }
};
