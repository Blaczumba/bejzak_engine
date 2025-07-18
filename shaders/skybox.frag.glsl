#version 450

#include "bindless.glsl"

layout (location = 0) in vec3 inUVW;

layout(push_constant) uniform Constants {
    mat4 proj;
    mat3 view;
	uint skyboxHandle;

} pushConstants;

layout (location = 0) out vec4 outFragColor;

void main() {
	outFragColor = texture(uGlobalTexturesCube[nonuniformEXT(pushConstants.skyboxHandle)], inUVW);
}