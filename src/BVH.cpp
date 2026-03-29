#include "BVH.h"
#include <algorithm>
#include <iostream>

void AABB::grow(const glm::vec3& p) {
    bmin = glm::min(bmin, p);
    bmax = glm::max(bmax, p);
}

void AABB::grow(const AABB& b) {
    if (b.bmin.x != 1e30f) {
        bmin = glm::min(bmin, b.bmin);
        bmax = glm::max(bmax, b.bmax);
    }
}

float AABB::area() const {
    glm::vec3 e = bmax - bmin;
    return e.x * e.y + e.y * e.z + e.z * e.x;
}

bool AABB::intersect(const Ray& ray, float& tmin, float& tmax, int* hitAxis, int* hitDir) const {
    float tx1 = (bmin.x - ray.origin.x) * ray.invDir.x;
    float tx2 = (bmax.x - ray.origin.x) * ray.invDir.x;
    float tmin_x = std::min(tx1, tx2);
    float tmax_x = std::max(tx1, tx2);

    float ty1 = (bmin.y - ray.origin.y) * ray.invDir.y;
    float ty2 = (bmax.y - ray.origin.y) * ray.invDir.y;
    float tmin_y = std::min(ty1, ty2);
    float tmax_y = std::max(ty1, ty2);

    float tz1 = (bmin.z - ray.origin.z) * ray.invDir.z;
    float tz2 = (bmax.z - ray.origin.z) * ray.invDir.z;
    float tmin_z = std::min(tz1, tz2);
    float tmax_z = std::max(tz1, tz2);

    tmin = std::max(std::max(tmin_x, tmin_y), tmin_z);
    tmax = std::min(std::min(tmax_x, tmax_y), tmax_z);

    if (tmax < tmin || tmax <= 0.0f) return false;

    if (hitAxis || hitDir) {
        int axis = 0;
        float bestT = tmin_x;
        if (tmin_y > bestT) { axis = 1; bestT = tmin_y; }
        if (tmin_z > bestT) { axis = 2; bestT = tmin_z; }
        if (hitAxis) *hitAxis = axis;
        if (hitDir) {
            // Entering face normal points outward; sign is based on ray direction
            float dirComp = axis == 0 ? ray.dir.x : (axis == 1 ? ray.dir.y : ray.dir.z);
            *hitDir = dirComp >= 0.0f ? -1 : 1;
        }
    }

    return true;
}

BVH::BVH(std::vector<Triangle>& triangles) : tris(triangles) {
    triIndices.resize(tris.size());
    centroids.resize(tris.size());
    for(size_t i = 0; i < tris.size(); ++i) {
        triIndices[i] = i;
        centroids[i] = (tris[i].v0 + tris[i].v1 + tris[i].v2) * 0.3333333f;
    }
}

void BVH::updateNodeBounds(int nodeIdx) {
    BVHNode& node = nodes[nodeIdx];
    node.bounds.bmin = glm::vec3(1e30f);
    node.bounds.bmax = glm::vec3(-1e30f);
    for (int i = 0; i < node.triCount; ++i) {
        int triIdx = triIndices[node.triStart + i];
        node.bounds.grow(tris[triIdx].v0);
        node.bounds.grow(tris[triIdx].v1);
        node.bounds.grow(tris[triIdx].v2);
    }
}

void BVH::build() {
    nodes.resize(tris.size() * 2);
    rootNodeIdx = 0;
    nodesUsed = 1;

    BVHNode& root = nodes[rootNodeIdx];
    root.left = root.right = -1;
    root.triStart = 0;
    root.triCount = (int)tris.size();
    
    updateNodeBounds(rootNodeIdx);
    subdivide(rootNodeIdx, 1);

    computeMetrics(rootNodeIdx, 1);
}

void BVH::subdivide(int nodeIdx, int depth) {
    BVHNode& node = nodes[nodeIdx];
    
    if (node.triCount <= 2) return;

    int bestAxis = -1;
    float bestPos = 0;
    float bestCost = 1e30f;
    const int BINS = 8;

    for (int axis = 0; axis < 3; ++axis) {
        float boundsMin = 1e30f, boundsMax = -1e30f;
        for (int i = 0; i < node.triCount; ++i) {
            float pos = centroids[triIndices[node.triStart + i]][axis];
            boundsMin = std::min(boundsMin, pos);
            boundsMax = std::max(boundsMax, pos);
        }
        if (boundsMin == boundsMax) continue;

        Bin bins[BINS];
        float scale = BINS / (boundsMax - boundsMin);
        for (int i = 0; i < node.triCount; ++i) {
            int triIdx = triIndices[node.triStart + i];
            int binIdx = std::min(BINS - 1, (int)((centroids[triIdx][axis] - boundsMin) * scale));
            bins[binIdx].triCount++;
            bins[binIdx].bounds.grow(tris[triIdx].v0);
            bins[binIdx].bounds.grow(tris[triIdx].v1);
            bins[binIdx].bounds.grow(tris[triIdx].v2);
        }

        float leftArea[BINS - 1], rightArea[BINS - 1];
        int leftCount[BINS - 1], rightCount[BINS - 1];
        
        AABB leftBox, rightBox;
        int leftSum = 0, rightSum = 0;
        
        for (int i = 0; i < BINS - 1; ++i) {
            leftSum += bins[i].triCount;
            leftCount[i] = leftSum;
            leftBox.grow(bins[i].bounds);
            leftArea[i] = leftBox.area();
            
            int rightIdx = BINS - 1 - i;
            rightSum += bins[rightIdx].triCount;
            rightCount[rightIdx - 1] = rightSum;
            rightBox.grow(bins[rightIdx].bounds);
            rightArea[rightIdx - 1] = rightBox.area();
        }

        float scale2 = (boundsMax - boundsMin) / BINS;
        for (int i = 0; i < BINS - 1; ++i) {
            float cost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestPos = boundsMin + scale2 * (i + 1);
            }
        }
    }

    float parentArea = node.bounds.area();
    float parentCost = node.triCount * parentArea;
    if (bestCost >= parentCost || bestAxis == -1) return;

    int i = node.triStart;
    int j = i + node.triCount - 1;
    while (i <= j) {
        if (centroids[triIndices[i]][bestAxis] < bestPos) {
            i++;
        } else {
            std::swap(triIndices[i], triIndices[j--]);
        }
    }

    int leftCount = i - node.triStart;
    if (leftCount == 0 || leftCount == node.triCount) return;

    int leftChildIdx = nodesUsed++;
    int rightChildIdx = nodesUsed++;

    nodes[leftChildIdx].triStart = node.triStart;
    nodes[leftChildIdx].triCount = leftCount;
    nodes[leftChildIdx].left = -1;
    nodes[leftChildIdx].right = -1;
    updateNodeBounds(leftChildIdx);

    nodes[rightChildIdx].triStart = i;
    nodes[rightChildIdx].triCount = node.triCount - leftCount;
    nodes[rightChildIdx].left = -1;
    nodes[rightChildIdx].right = -1;
    updateNodeBounds(rightChildIdx);

    node.triStart = 0;
    node.triCount = 0;
    node.left = leftChildIdx;
    node.right = rightChildIdx;

    subdivide(leftChildIdx, depth + 1);
    subdivide(rightChildIdx, depth + 1);
}

void BVH::computeMetrics(int nodeIdx, int currentDepth) {
    if (currentDepth > maxTreeDepth) maxTreeDepth = currentDepth;
    BVHNode& node = nodes[nodeIdx];
    if (!node.isLeaf()) {
        computeMetrics(node.left, currentDepth + 1);
        computeMetrics(node.right, currentDepth + 1);
    }
}

void BVH::markAllTested(int nodeIdx, std::vector<int>& testedTris, int& triTests) {
    BVHNode& node = nodes[nodeIdx];
    if (node.isLeaf()) {
        for (int i = 0; i < node.triCount; ++i) {
            testedTris.push_back(triIndices[node.triStart + i]);
            triTests++;
        }
    } else {
        markAllTested(node.left, testedTris, triTests);
        markAllTested(node.right, testedTris, triTests);
    }
}

void BVH::traverseInternal(int nodeIdx, const Ray& ray, int currentDepth, int maxDepth,
                           std::vector<int>& testedTris, int& boxTests, int& triTests) {
    boxTests++;
    BVHNode& node = nodes[nodeIdx];
    
    float tmin, tmax;
    if (!node.bounds.intersect(ray, tmin, tmax)) return;

    if (currentDepth == maxDepth || node.isLeaf()) {
        markAllTested(nodeIdx, testedTris, triTests);
        return;
    }

    traverseInternal(node.left, ray, currentDepth + 1, maxDepth, testedTris, boxTests, triTests);
    traverseInternal(node.right, ray, currentDepth + 1, maxDepth, testedTris, boxTests, triTests);
}

void BVH::traverse(const Ray& ray, int maxDepth, 
                   std::vector<int>& testedTris, 
                   int& boxTests, int& triTests) {
    testedTris.clear();
    boxTests = 0;
    triTests = 0;

    if (maxDepth < 0) return;

    if (maxDepth == 0) {
        for (size_t i = 0; i < tris.size(); ++i) {
            testedTris.push_back((int)i);
            triTests++;
        }
        return;
    }

    traverseInternal(rootNodeIdx, ray, 1, maxDepth, testedTris, boxTests, triTests);
}
