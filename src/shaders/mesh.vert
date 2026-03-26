#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

flat out vec3 vColor;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0);
    vColor = aColor; // flat output directly handles flat shading via provoking vertex duplicate method!
}
