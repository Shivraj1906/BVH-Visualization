#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

#include "Scene.h"
#include <iostream>
#include <cmath>

glm::vec3 camPos(0.0f, 0.5f, 5.0f);
glm::vec3 camFront(0.0f, 0.0f, -1.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);

float camYaw = -90.0f;
float camPitch = 0.0f;
bool isRightDragging = false;
double lastMouseX, lastMouseY;
float camSpeed = 5.0f;
float mouseSensitivity = 0.1f;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            isRightDragging = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
            isRightDragging = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isRightDragging) {
        double dx = xpos - lastMouseX;
        double dy = lastMouseY - ypos;
        lastMouseX = xpos;
        lastMouseY = ypos;

        camYaw += (float)dx * mouseSensitivity;
        camPitch += (float)dy * mouseSensitivity;

        if (camPitch > 89.0f) camPitch = 89.0f;
        if (camPitch < -89.0f) camPitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(camYaw)) * cos(glm::radians(camPitch));
        front.y = sin(glm::radians(camPitch));
        front.z = sin(glm::radians(camYaw)) * cos(glm::radians(camPitch));
        camFront = glm::normalize(front);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    camPos += camFront * (float)yoffset * 0.5f;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "BVH Visualizer V2", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    Scene scene;
    scene.init();
    camPos = scene.meshCentroid + glm::vec3(0.0f, 0.0f, 3.0f);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    int gizmoMode = 0; // 0: Translate, 1: Rotate

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        if (!ImGui::GetIO().WantCaptureKeyboard) {
            float velocity = camSpeed * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camFront * velocity;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camFront * velocity;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= glm::normalize(glm::cross(camFront, camUp)) * velocity;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += glm::normalize(glm::cross(camFront, camUp)) * velocity;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camPos += camUp * velocity;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camPos -= camUp * velocity;
        }

        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        int dw, dh;
        glfwGetFramebufferSize(window, &dw, &dh);
        glViewport(0, 0, dw, dh);
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)dw / (float)dh, 0.1f, 100.0f);

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 300, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, io.DisplaySize.y), ImGuiCond_Always);
        ImGui::Begin("BVH Visualizer", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        
        if (ImGui::CollapsingHeader("BVH Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Enable BVH", &scene.config.enableBVH);
            int maxDepthSlider = scene.maxTreeDepth > 0 ? scene.maxTreeDepth : 20;
            ImGui::SliderInt("BVH Max Depth", &scene.config.bvhMaxDepth, 0, maxDepthSlider);
        }

        if (ImGui::CollapsingHeader("Camera Tools", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Move Speed", &camSpeed, 1.0f, 20.0f);
            ImGui::SliderFloat("Look Sensitivity", &mouseSensitivity, 0.01f, 0.5f);
            if (ImGui::Button("Reset Camera")) {
                camPos = scene.meshCentroid + glm::vec3(0.0f, 0.0f, 3.0f);
                camYaw = -90.0f;
                camPitch = 0.0f;
            }
        }

        if (ImGui::CollapsingHeader("Ray Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::RadioButton("Translate Handle", &gizmoMode, 0); ImGui::SameLine();
            ImGui::RadioButton("Rotate Handle", &gizmoMode, 1); ImGui::SameLine();
            ImGui::RadioButton("Universal", &gizmoMode, 2);
            ImGui::SliderFloat3("Origin X/Y/Z", scene.config.rayOrigin, -3.0f, 3.0f);
            ImGui::SliderFloat3("Rotation Euler", scene.config.rayDirEuler, -180.0f, 180.0f);
        }

        if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Box tests:      %d", scene.statBoxTests);
            ImGui::Text("Triangle tests: %d", scene.statTriTests);
            ImGui::Text("Total tris:     %d", scene.statTotalTris);
        }

        if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Show BVH boxes", &scene.config.showBVHBoxes);
            ImGui::Checkbox("Show ray", &scene.config.showRay);
            ImGui::Checkbox("Show primitive borders", &scene.config.showWireframes);
            ImGui::SliderFloat("Mesh opacity", &scene.config.meshOpacity, 0.1f, 1.0f);
        }
        ImGui::End();

        float matrix[16];
        float scale[3] = {1.0f, 1.0f, 1.0f};
        ImGuizmo::RecomposeMatrixFromComponents(scene.config.rayOrigin, scene.config.rayDirEuler, scale, matrix);

        ImGuizmo::OPERATION op;
        if (gizmoMode == 0) op = ImGuizmo::TRANSLATE;
        else if (gizmoMode == 1) op = ImGuizmo::ROTATE;
        else op = ImGuizmo::UNIVERSAL;
        
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), op, ImGuizmo::WORLD, matrix);

        if (ImGuizmo::IsUsing()) {
            ImGuizmo::DecomposeMatrixToComponents(matrix, scene.config.rayOrigin, scene.config.rayDirEuler, scale);
        }
        
        // Exact 1:1 synchronization between Vector Direction & Widget handle limits
        scene.config.rayDir = glm::normalize(glm::vec3(matrix[8], matrix[9], matrix[10]));

        scene.update();
        scene.draw(view, proj);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
