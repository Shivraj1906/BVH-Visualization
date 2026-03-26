#include "Renderer.h"
#include "Mesh.h"
#include "BVH.h"
#include "Ray.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    } catch(std::ifstream::failure& e) {
        std::cerr << "SHADER FILE READ ERROR: " << vertexPath << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    
    GLuint vertex, fragment;
    int success;
    char infoLog[512];
    
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "VERTEX SHADER COMPILATION FAILED: " << vertexPath << "\n" << infoLog << std::endl;
    }
    
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "FRAGMENT SHADER COMPILATION FAILED: " << fragmentPath << "\n" << infoLog << std::endl;
    }
    
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << "SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() const { glUseProgram(ID); }
void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}
void Shader::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
}
void Shader::setFloat(const std::string& name, float val) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), val);
}

Renderer::Renderer() {}
Renderer::~Renderer() {
    delete meshShader; delete bboxShader; delete rayShader;
}

void Renderer::init() {
    meshShader = new Shader("../src/shaders/mesh.vert", "../src/shaders/mesh.frag");
    bboxShader = new Shader("../src/shaders/bbox.vert", "../src/shaders/bbox.frag");
    rayShader  = new Shader("../src/shaders/ray.vert", "../src/shaders/ray.frag");

    glGenVertexArrays(1, &bvhVAO);
    glGenBuffers(1, &bvhVBO);

    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);

    buildSourceIcon();
}

void Renderer::setupMesh(const Mesh& mesh) {
    glGenVertexArrays(1, &meshVAO);
    glGenBuffers(1, &meshVBO_pos);
    glGenBuffers(1, &meshVBO_col);

    meshVertexCount = (int)mesh.vertices.size();

    glBindVertexArray(meshVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO_pos);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(glm::vec3), mesh.vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, meshVBO_col);
    glBufferData(GL_ARRAY_BUFFER, mesh.colors.size() * sizeof(glm::vec3), mesh.colors.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Renderer::updateMeshColors(const Mesh& mesh) {
    if(meshVBO_col == 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, meshVBO_col);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.colors.size() * sizeof(glm::vec3), mesh.colors.data());
}

void Renderer::buildBVHGeometryEx(const BVH& bvh, int nodeIdx, int currentDepth, int maxDepth, std::vector<glm::vec3>& lines, std::vector<BBoxDrawCommand>& cmds) {
    const BVHNode& node = bvh.nodes[nodeIdx];
    
    if (currentDepth <= maxDepth) {
        glm::vec3 p0 = node.bounds.bmin;
        glm::vec3 p1 = node.bounds.bmax;
        glm::vec3 corners[8] = {
            glm::vec3(p0.x, p0.y, p0.z), glm::vec3(p1.x, p0.y, p0.z),
            glm::vec3(p1.x, p1.y, p0.z), glm::vec3(p0.x, p1.y, p0.z),
            glm::vec3(p0.x, p0.y, p1.z), glm::vec3(p1.x, p0.y, p1.z),
            glm::vec3(p1.x, p1.y, p1.z), glm::vec3(p0.x, p1.y, p1.z)
        };
        int indices[24] = {
            0,1, 1,2, 2,3, 3,0,
            4,5, 5,6, 6,7, 7,4,
            0,4, 1,5, 2,6, 3,7 
        };

        cmds.push_back({ (int)lines.size(), 24, glm::vec3(0.0f) });
        
        for (int i = 0; i < 24; i++) {
            lines.push_back(corners[indices[i]]);
        }

        float t = (float)currentDepth / (float)std::max(1, maxDepth);
        // cyan -> yellow -> magenta approximation
        cmds.back().color = glm::vec3(1.0f - t, t, 1.0f); 

        if (!node.isLeaf()) {
            buildBVHGeometryEx(bvh, node.left, currentDepth + 1, maxDepth, lines, cmds);
            buildBVHGeometryEx(bvh, node.right, currentDepth + 1, maxDepth, lines, cmds);
        }
    }
}

void Renderer::updateBVHWireframe(const BVH& bvh, int maxDepth) {
    std::vector<glm::vec3> lines;
    bvhDrawCommands.clear();

    if (maxDepth > 0 && !bvh.nodes.empty()) {
        buildBVHGeometryEx(bvh, bvh.rootNodeIdx, 1, maxDepth, lines, bvhDrawCommands);
    }

    if (!lines.empty()) {
        glBindVertexArray(bvhVAO);
        glBindBuffer(GL_ARRAY_BUFFER, bvhVBO);
        glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
}

void Renderer::updateRay(const Ray& ray) {
    std::vector<glm::vec3> rayLine = {
        ray.origin,
        ray.origin + ray.dir * 30.0f
    };
    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    glBufferData(GL_ARRAY_BUFFER, rayLine.size() * sizeof(glm::vec3), rayLine.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Renderer::buildSourceIcon() {
    std::vector<glm::vec3> verts;
    float radius = 1.0f; // Unit radius mapping
    int sectorCount = 36;
    int stackCount = 18;
    
    for(int i = 0; i <= stackCount; ++i) {
        float stackAngle = static_cast<float>(M_PI) / 2.0f - i * (static_cast<float>(M_PI) / stackCount);
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for(int j = 0; j <= sectorCount; ++j) {
            float sectorAngle = j * 2.0f * static_cast<float>(M_PI) / sectorCount;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            verts.push_back(glm::vec3(x, y, z));
        }
    }
    
    std::vector<glm::vec3> finalVerts;
    for(int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;
        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if(i != 0) {
                finalVerts.push_back(verts[k1]);
                finalVerts.push_back(verts[k2]);
                finalVerts.push_back(verts[k1 + 1]);
            }
            if(i != (stackCount - 1)) {
                finalVerts.push_back(verts[k1 + 1]);
                finalVerts.push_back(verts[k2]);
                finalVerts.push_back(verts[k2 + 1]);
            }
        }
    }

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, finalVerts.size() * sizeof(glm::vec3), finalVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    sphereVertexCount = (int)finalVerts.size();
}

void Renderer::draw(const Mesh& mesh, const glm::mat4& view, const glm::mat4& proj,
                    bool showBVH, bool showRay, float meshOpacity, bool showWireframes, float originRadius) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // 1. Mesh Pass
    if (meshOpacity > 0.01f) {
        if (meshOpacity < 1.0f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }

        meshShader->use();
        meshShader->setMat4("view", view);
        meshShader->setMat4("projection", proj);
        meshShader->setFloat("uOpacity", meshOpacity);

        glBindVertexArray(meshVAO);
        glDrawArrays(GL_TRIANGLES, 0, meshVertexCount);
        
        if (showWireframes) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-2.0f, -2.0f);
            
            bboxShader->use();
            bboxShader->setMat4("view", view);
            bboxShader->setMat4("projection", proj);
            bboxShader->setVec3("uColor", glm::vec3(0.0f, 0.0f, 0.0f)); // Black borders
            
            glDrawArrays(GL_TRIANGLES, 0, meshVertexCount);
            
            glDisable(GL_POLYGON_OFFSET_LINE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }

    // 2. BVH Wireframe Pass
    if (showBVH && !bvhDrawCommands.empty()) {
        glDepthMask(GL_FALSE);
        
        bboxShader->use();
        bboxShader->setMat4("view", view);
        bboxShader->setMat4("projection", proj);

        glBindVertexArray(bvhVAO);
        for (const auto& cmd : bvhDrawCommands) {
            bboxShader->setVec3("uColor", cmd.color);
            glDrawArrays(GL_LINES, cmd.offset, cmd.count);
        }
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
    }

    // 3. Ray Pass
    if (showRay) {
        rayShader->use();
        rayShader->setMat4("view", view);
        rayShader->setMat4("projection", proj);
        rayShader->setVec3("uColor", glm::vec3(1.0f, 1.0f, 0.0f));

        // RESET model matrix for the line since it has baked-in world coordinates
        rayShader->setMat4("model", glm::mat4(1.0f));

        glBindVertexArray(rayVAO);
        
        // Retrieve origin from VBO
        glm::vec3 lineData[2];
        glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * 2, lineData);
        glm::vec3 rayOrigin = lineData[0];

        glDrawArrays(GL_LINES, 0, 2);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), rayOrigin);
        model = glm::scale(model, glm::vec3(originRadius));
        rayShader->setMat4("model", model);
        
        glBindVertexArray(sphereVAO);
        glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
        glBindVertexArray(0);
    }
}
