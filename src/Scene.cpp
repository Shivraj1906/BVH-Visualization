#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

static bool intersectRayTriangle(const Ray& ray, const Triangle& tri, float& tHit, glm::vec3& hitPos) {
    const float EPS = 1e-6f;
    glm::vec3 edge1 = tri.v1 - tri.v0;
    glm::vec3 edge2 = tri.v2 - tri.v0;
    glm::vec3 pvec = glm::cross(ray.dir, edge2);
    float det = glm::dot(edge1, pvec);
    if (std::fabs(det) < EPS) return false;
    float invDet = 1.0f / det;
    glm::vec3 tvec = ray.origin - tri.v0;
    float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f) return false;
    glm::vec3 qvec = glm::cross(tvec, edge1);
    float v = glm::dot(ray.dir, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;
    float t = glm::dot(edge2, qvec) * invDet;
    if (t <= 0.0f) return false;
    tHit = t;
    hitPos = ray.origin + ray.dir * tHit;
    return true;
}

Scene::Scene() {}

Scene::~Scene() {
    delete bvh;
    delete currentRay;
}

void Scene::init() {
    renderer.init();
    if (!loadModel("../assets/bunny_lowpoly.obj")) {
        std::cerr << "Failed to load default mesh!" << std::endl;
    }
}

bool Scene::loadModel(const std::string& filepath) {
    if (!mesh.init(filepath)) {
        std::cerr << "Failed to load mesh: " << filepath << std::endl;
        return false;
    }

    statTotalTris = (int)mesh.triangles.size();

    delete bvh;
    bvh = new BVH(mesh.triangles);
    bvh->build();
    maxTreeDepth = bvh->maxTreeDepth;
    renderer.setupMesh(mesh);

    glm::vec3 sum(0.0f);
    for (const auto& v : mesh.vertices) sum += v;
    meshCentroid = mesh.vertices.empty() ? glm::vec3(0.0f) : sum / (float)mesh.vertices.size();

    statBoxTests = 0;
    statTriTests = 0;
    needsUpdate = true;
    lastConfig = config;
    cameraResetRequested = true;

    return true;
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
        config.highlightHitFaces != lastConfig.highlightHitFaces ||
        config.lineWidth != lastConfig.lineWidth ||
        config.dotRadius != lastConfig.dotRadius ||
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
            glm::vec3 redColor(0.85f, 0.18f, 0.30f); // Deeper accent for light background
            
            for (int triIdx : testedTris) {
                mesh.colors[triIdx * 3 + 0] = redColor;
                mesh.colors[triIdx * 3 + 1] = redColor;
                mesh.colors[triIdx * 3 + 2] = redColor;
            }

            std::vector<glm::vec3> geomHits;
            geomHits.reserve(testedTris.size());
            for (int triIdx : testedTris) {
                const Triangle& tri = mesh.triangles[triIdx];
                float tHit = 0.0f;
                glm::vec3 hitPos;
                if (intersectRayTriangle(*currentRay, tri, tHit, hitPos)) {
                    geomHits.push_back(hitPos);
                }
            }
            
            renderer.updateMeshColors(mesh);
            renderer.updateBVHWireframe(*bvh, depthLimit, currentRay, config.highlightHitFaces);
            renderer.setGeometryHitPoints(geomHits);
        } else {
            renderer.setGeometryHitPoints({});
        }

        renderer.updateRay(*currentRay);
    }
}

void Scene::draw(const glm::mat4& view, const glm::mat4& proj) {
    if (bvh) {
        renderer.draw(mesh, view, proj, config.showBVHBoxes, config.showRay, config.meshOpacity, config.showWireframes, config.originRadius, config.lineWidth, config.dotRadius);
    }
}

bool Scene::shouldResetCamera() const {
    return cameraResetRequested;
}

void Scene::acknowledgeCameraReset() {
    cameraResetRequested = false;
}
