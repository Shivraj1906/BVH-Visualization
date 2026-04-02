#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>

bool MeshLoader::loadOBJ(const std::string& filepath, 
                         std::vector<glm::vec3>& out_vertices,
                         std::vector<glm::vec3>& out_normals,
                         std::vector<Triangle>& out_triangles) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filepath, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        return false;
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    int triIndex = 0;
    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            if (fv != 3) {
                index_offset += fv;
                continue;
            }

            Triangle tri;
            glm::vec3 faceNormal(0);

            for (size_t v = 0; v < 3; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                
                glm::vec3 pos(
                    attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                    attrib.vertices[3 * size_t(idx.vertex_index) + 2]
                );

                glm::vec3 norm(0);
                if (idx.normal_index >= 0) {
                    norm = glm::vec3(
                        attrib.normals[3 * size_t(idx.normal_index) + 0],
                        attrib.normals[3 * size_t(idx.normal_index) + 1],
                        attrib.normals[3 * size_t(idx.normal_index) + 2]
                    );
                }

                out_vertices.push_back(pos);
                out_normals.push_back(norm);
                
                if (v == 0) tri.v0 = pos;
                else if (v == 1) tri.v1 = pos;
                else if (v == 2) tri.v2 = pos;
            }

            if (glm::length(out_normals.back()) < 0.1f) {
                glm::vec3 e1 = tri.v1 - tri.v0;
                glm::vec3 e2 = tri.v2 - tri.v0;
                faceNormal = glm::normalize(glm::cross(e1, e2));
                for(int i = 0; i < 3; ++i) {
                    out_normals[out_normals.size() - 3 + i] = faceNormal;
                }
            }

            tri.index = triIndex++;
            out_triangles.push_back(tri);
            index_offset += fv;
        }
    }
    return true;
}

bool Mesh::init(const std::string& filepath) {
    vertices.clear();
    normals.clear();
    triangles.clear();
    baseColors.clear();
    colors.clear();

    if (!MeshLoader::loadOBJ(filepath, vertices, normals, triangles)) {
        return false;
    }
    
    baseColors.resize(vertices.size(), defaultColor);
    colors.resize(vertices.size(), defaultColor);
    
    // Bake simple lambertian shading
    for (size_t i = 0; i < normals.size(); ++i) {
        glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
        float ndotl = glm::max(0.4f, glm::dot(normals[i], lightDir)); // keep bright on white bg
        baseColors[i] = defaultColor * ndotl;
        colors[i] = baseColors[i];
    }
    return true;
}

void Mesh::resetColors() {
    colors = baseColors;
}
