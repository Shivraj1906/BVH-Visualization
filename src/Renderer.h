#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

class Mesh;
class BVH;
struct Ray;

class Shader {
public:
    GLuint ID;
    Shader(const char* vertexPath, const char* fragmentPath);
    void use() const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& vec) const;
    void setFloat(const std::string& name, float val) const;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void init();
    void setupMesh(const Mesh& mesh);
    void updateMeshColors(const Mesh& mesh);
    void updateBVHWireframe(const BVH& bvh, int maxDepth, const Ray* ray, bool collectFaces);
    void updateRay(const Ray& ray);
    void setGeometryHitPoints(const std::vector<glm::vec3>& points);

    void draw(const Mesh& mesh, const glm::mat4& view, const glm::mat4& proj,
              bool showBVH, bool showRay, float meshOpacity, bool showWireframes,
              float originRadius = 0.05f, float lineWidth = 1.0f, float dotRadius = 0.03f);

private:
    Shader* meshShader = nullptr;
    Shader* bboxShader = nullptr;
    Shader* rayShader = nullptr;

    GLuint meshVAO = 0, meshVBO_pos = 0, meshVBO_col = 0;
    int meshVertexCount = 0;

    GLuint bvhVAO = 0, bvhVBO = 0;
    struct BBoxDrawCommand {
        int offset;
        int count;
        glm::vec3 color;
    };
    std::vector<BBoxDrawCommand> bvhDrawCommands;

    GLuint hitVAO = 0, hitVBO = 0;
    std::vector<BBoxDrawCommand> hitDrawCommands;

    GLuint rayVAO = 0, rayVBO = 0;
    GLuint sphereVAO = 0, sphereVBO = 0;
    int sphereVertexCount = 0;

    std::vector<glm::vec3> bvhHitPoints;
    std::vector<glm::vec3> geomHitPoints;

    void buildSourceIcon();
    void buildBVHGeometryEx(const BVH& bvh, int nodeIdx, int currentDepth, int maxDepth,
                            const Ray* ray, bool collectFaces,
                            std::vector<glm::vec3>& lines, std::vector<BBoxDrawCommand>& cmds,
                            std::vector<glm::vec3>& hitFaces, std::vector<BBoxDrawCommand>& hitCmds);
};
