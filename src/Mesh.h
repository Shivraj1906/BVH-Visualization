#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct Triangle {
    glm::vec3 v0, v1, v2;
    int index;
};

class MeshLoader {
public:
    static bool loadOBJ(const std::string& filepath, 
                        std::vector<glm::vec3>& out_vertices,
                        std::vector<glm::vec3>& out_normals,
                        std::vector<Triangle>& out_triangles);
};

class Mesh {
public:
    Mesh() = default;
    bool init(const std::string& filepath);

    std::vector<glm::vec3> vertices; 
    std::vector<glm::vec3> normals;  
    std::vector<glm::vec3> colors;   
    std::vector<glm::vec3> baseColors; 

    std::vector<Triangle> triangles;
    glm::vec3 defaultColor = glm::vec3(0.90f, 0.90f, 0.88f); // soft off-white for light theme

    void resetColors();
};
