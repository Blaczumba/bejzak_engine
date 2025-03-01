#version 450

layout(binding = 2) uniform ObjectUniform {
    mat4 model;

} object;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;

void main() {
    gl_Position = object.model * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
    mat3 normalMatrix = transpose(inverse(mat3(object.model)));
    outNormal = normalize(normalMatrix * inNormal);
    outTangent = normalize(normalMatrix * inTangent);
}