cmake_minimum_required(VERSION 3.20)
project(bvh_visualizer)
set(CMAKE_CXX_STANDARD 17)

# System-installed packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

include(FetchContent)

# 1. ImGui via FetchContent
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.8
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# ImGuizmo via FetchContent
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
)
FetchContent_GetProperties(imguizmo)
if(NOT imguizmo_POPULATED)
    FetchContent_Populate(imguizmo)
endif()

add_library(imgui_app STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)
target_include_directories(imgui_app PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imguizmo_SOURCE_DIR}
)
target_link_libraries(imgui_app PUBLIC glfw OpenGL::GL)

# 2. tinyobjloader via FetchContent
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        966edceaf8cdca7996c4e9a1c5ced2938de63366
)
FetchContent_MakeAvailable(tinyobjloader)

add_library(tinyobj INTERFACE)
target_include_directories(tinyobj INTERFACE ${tinyobjloader_SOURCE_DIR})

# 3. glad (generated locally into vendor/glad)
add_library(glad STATIC vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/include)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

# Project Executable
# main application sources
add_executable(bvh_visualizer
    src/main.cpp
    src/Scene.cpp
    src/Mesh.cpp
    src/BVH.cpp
    src/Renderer.cpp
)

target_include_directories(bvh_visualizer PRIVATE src)
# The executable links against everything
target_link_libraries(bvh_visualizer PRIVATE
    OpenGL::GL
    glfw
    glm::glm
    imgui_app
    tinyobj
    glad
)
