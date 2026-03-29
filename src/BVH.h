#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "Ray.h"

struct AABB {
    glm::vec3 bmin{1e30f};
    glm::vec3 bmax{-1e30f};

    void grow(const glm::vec3& p);
    void grow(const AABB& b);
    float area() const;
    // Optionally returns which axis the ray enters on (0=x,1=y,2=z) and the facing direction (-1 or +1)
    bool intersect(const Ray& ray, float& tmin, float& tmax, int* hitAxis = nullptr, int* hitDir = nullptr) const;
};

struct BVHNode {
    AABB bounds;
    int left, right;
    int triStart, triCount;
    
    bool isLeaf() const { return triCount > 0; }
};

class BVH {
public:
    BVH(std::vector<Triangle>& triangles);
    void build();
    
    void traverse(const Ray& ray, int maxDepth, 
                  std::vector<int>& testedTris, 
                  int& boxTests, int& triTests);

    std::vector<BVHNode> nodes;
    int rootNodeIdx = 0;
    int maxTreeDepth = 0;

private:
    std::vector<Triangle>& tris;
    std::vector<int> triIndices; // indirection array for sorting
    std::vector<glm::vec3> centroids;

    int nodesUsed = 0;
    
    void updateNodeBounds(int nodeIdx);
    void subdivide(int nodeIdx, int depth);
    void computeMetrics(int nodeIdx, int currentDepth);
    void traverseInternal(int nodeIdx, const Ray& ray, int currentDepth, int maxDepth,
                          std::vector<int>& testedTris, int& boxTests, int& triTests);
    void markAllTested(int nodeIdx, std::vector<int>& testedTris, int& triTests);
    
    struct Bin { AABB bounds; int triCount = 0; };
};
