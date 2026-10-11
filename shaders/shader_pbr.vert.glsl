#version 450

#include "bindless.glsl"
#include "32bit_push_constants.glsl"

RegisterUniform(Light, { \
    mat4 projView; \
    vec3 pos; \
});

// Push constants definitions:
#define modelMat pushConstants.model
#define lightBufferHandle pushConstants.handles[0]

layout(set=1, binding=0) uniform CameraUniform { // Dynamic uniform buffer which depends on frame in flight
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    vec3 viewDir;
} camera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
// layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec3 TBNfragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 lightFragPosition;

layout(location = 3) out vec3 TBNLightPos;
layout(location = 4) out vec3 TBNViewPos;

const mat4 BiasMat = mat4(
	0.5, 0, 0, 0,
	0, 0.5, 0, 0,
	0, 0, 1.0, 0,
	0.5, 0.5, 0.0, 1.0
);

void main() {
    mat3 model3 = mat3(modelMat);
    mat3 normalMatrix = transpose(inverse(model3));

    vec3 normal = normalize(normalMatrix * inNormal);
    vec3 tangent = model3 * inTangent;
    tangent = normalize(tangent - dot(tangent, normal) * normal); // Gram-Schmidt in world space
    vec3 bitangent = cross(normal, tangent);
    // Orthonormal basis -> transpose == inverse, so this maps world space -> tangent space.
    mat3 TBNMat = transpose(mat3(tangent, bitangent, normal));

    vec4 worldPos = modelMat * vec4(inPosition, 1.0);

    TBNfragPosition = TBNMat * worldPos.xyz;
    TBNViewPos      = TBNMat * camera.viewPos;
    TBNLightPos     = TBNMat * GetResource(Light, lightBufferHandle).pos;
    lightFragPosition = BiasMat * GetResource(Light, lightBufferHandle).projView * worldPos;

    gl_Position = camera.proj * camera.view * worldPos;

    fragTexCoord = inTexCoord;
}
