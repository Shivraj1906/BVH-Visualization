#pragma once
#include "Mesh.h"
#include "BVH.h"
#include "Ray.h"
#include "Renderer.h"

struct SceneConfig {
    bool enableBVH = true;
    int bvhMaxDepth = 4;
    
    float rayOrigin[3] = {0.0f, 0.5f, 2.0f};
    float rayDirEuler[3] = {0.0f, -180.0f, 0.0f};
    glm::vec3 rayDir = glm::vec3(0.0f, 0.0f, -1.0f);
    float originRadius = 0.05f;
    float lineWidth = 2.5f;

    bool showBVHBoxes = true;
    bool showRay = true;
    bool showWireframes = true;
    bool highlightHitFaces = true;
    float meshOpacity = 1.0f;
};

class Scene {
public:
    Scene();
    ~Scene();

    void init();
    void update();
    void draw(const glm::mat4& view, const glm::mat4& proj);

    SceneConfig config;

    int statBoxTests = 0;
    int statTriTests = 0;
    int statTotalTris = 0;
    int maxTreeDepth = 0;

    glm::vec3 meshCentroid = glm::vec3(0.0f);

private:
    Mesh mesh;
    BVH* bvh = nullptr;
    Renderer renderer;
    Ray* currentRay = nullptr;

    bool needsUpdate = true;
    SceneConfig lastConfig;
};
