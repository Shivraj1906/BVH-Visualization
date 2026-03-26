#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Scene::Scene() {}

Scene::~Scene() {
    delete bvh;
    delete currentRay;
}

void Scene::init() {
    renderer.init();
    
    if (mesh.init("../assets/bunny_lowpoly.obj")) {
        bvh = new BVH(mesh.triangles);
        bvh->build();
        statTotalTris = (int)mesh.triangles.size();
        maxTreeDepth = bvh->maxTreeDepth;
        renderer.setupMesh(mesh);

        glm::vec3 sum(0.0f);
        for (const auto& v : mesh.vertices) sum += v;
        if (!mesh.vertices.empty()) meshCentroid = sum / (float)mesh.vertices.size();
        
        // Initial ray setup will happen cleanly in first update()
    } else {
        std::cerr << "Failed to load mesh!" << std::endl;
    }

    lastConfig = config;
    needsUpdate = true;
}

void Scene::update() {
    // Check config changes
    if (config.enableBVH != lastConfig.enableBVH ||
        config.bvhMaxDepth != lastConfig.bvhMaxDepth ||
        config.rayOrigin[0] != lastConfig.rayOrigin[0] ||
        config.rayOrigin[1] != lastConfig.rayOrigin[1] ||
        config.rayOrigin[2] != lastConfig.rayOrigin[2] ||
        config.rayDirEuler[0] != lastConfig.rayDirEuler[0] ||
        config.rayDirEuler[1] != lastConfig.rayDirEuler[1] ||
        config.rayDirEuler[2] != lastConfig.rayDirEuler[2] ||
        needsUpdate) {
        
        needsUpdate = false;
        lastConfig = config;

        glm::vec3 origin(config.rayOrigin[0], config.rayOrigin[1], config.rayOrigin[2]);
        glm::vec3 dir = config.rayDir;
        
        delete currentRay;
        currentRay = new Ray(origin, dir);

        if (bvh) {
            std::vector<int> testedTris;
            int depthLimit = config.enableBVH ? config.bvhMaxDepth : 0;
            
            bvh->traverse(*currentRay, depthLimit, testedTris, statBoxTests, statTriTests);
            
            mesh.resetColors();
            glm::vec3 redColor(1.0f, 0.2f, 0.35f); // Coral pink/red aesthetic
            
            for (int triIdx : testedTris) {
                mesh.colors[triIdx * 3 + 0] = redColor;
                mesh.colors[triIdx * 3 + 1] = redColor;
                mesh.colors[triIdx * 3 + 2] = redColor;
            }
            
            renderer.updateMeshColors(mesh);
            renderer.updateBVHWireframe(*bvh, depthLimit);
        }

        renderer.updateRay(*currentRay);
    }
}

void Scene::draw(const glm::mat4& view, const glm::mat4& proj) {
    if (bvh) {
        renderer.draw(mesh, view, proj, config.showBVHBoxes, config.showRay, config.meshOpacity, config.showWireframes, config.originRadius);
    }
}
