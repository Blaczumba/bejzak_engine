#version 450

#include "bindless.glsl"
#include "32bit_push_constants.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

#define colorMapHandle pushConstants.handles[0]

void main() {
    outColor = texture(uGlobalTextures2D[nonuniformEXT(colorMapHandle)], uv);
}