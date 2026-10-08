#version 450
#extension GL_EXT_multiview : enable

#include "bindless.glsl"
#include "32bit_push_constants.glsl"

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

#define colorMapHandle pushConstants.handles[0]

void main() {
    outColor = texture(uGlobalTextureArrays2D[nonuniformEXT(colorMapHandle)], vec3(uv, gl_ViewIndex));
}