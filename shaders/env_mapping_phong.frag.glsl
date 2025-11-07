#version 450
#include "bindless.glsl"

layout (location = 0) in vec3 inUvw;

layout(push_constant) uniform Constants {
    mat4 projViewModel;
    mat3 model;
    uint environmentHandle;
} pushConstants;

layout (location = 0) out vec4 outFragColor;

void main() {
    outFragColor = texture(uGlobalTexturesCube[nonuniformEXT(pushConstants.environmentHandle)], normalize(inUvw));
}
