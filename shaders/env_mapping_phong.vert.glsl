#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout(push_constant) uniform Constants {
    mat4 projViewModel;
    mat3 model;
    uint environmentHandle;
} pushConstants;

layout (location = 0) out vec3 outUvw;

void main() {
    outUvw = transpose(inverse(pushConstants.model)) * inNormal;

    gl_Position = pushConstants.projViewModel * vec4(inPos, 1.0);
}
