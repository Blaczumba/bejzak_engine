#version 450

#include "bindless.glsl"
#include "32bit_push_constants.glsl"

// Push constants definitions:
#define modelMat pushConstants.model
#define diffuseMapHandle pushConstants.handles[1]
#define normalMapHandle pushConstants.handles[2]
#define metallicRoughnessMapHandle pushConstants.handles[3]
#define shadowMapHandle pushConstants.handles[4]

layout(location = 0) in vec3 TBNfragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 lightFragPosition;

layout(location = 3) in vec3 TBNLightPos;
layout(location = 4) in vec3 TBNViewPos;

layout(location = 0) out vec4 outColor;

const int KELNER_SIZE = 9;
const ivec2 offsets[] = ivec2[](
    ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1),
    ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
    ivec2(-1, -1), ivec2(0, -1), ivec2(1, -1)
);

const float PI = 3.14159265359;
const vec3 lightColor = 40.0 * vec3(1.0, 0.95, 0.88);
const float ao = 1.0;

float calculateShadowPcf() {
    vec3 lightFrag = lightFragPosition.xyz / lightFragPosition.w;
    if (lightFrag.z >= 1.0)
        return 1.0;

    float sum = textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[0])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[1])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[2])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[3])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[4])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[5])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[6])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[7])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(shadowMapHandle)], lightFrag.xyz, offsets[8]);

    return sum / float(KELNER_SIZE);
}

float DistributionGGX(float NdotH, float roughness) {
    float a  = roughness * roughness;   // perceptual -> alpha
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotX, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;            // k for direct (analytic) lights
    return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float roughness) {
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // --- Material inputs -------------------------------------------------------------
    vec3 albedo = texture(uGlobalTextures2D[nonuniformEXT(diffuseMapHandle)], fragTexCoord).rgb;

    vec2 mr = texture(uGlobalTextures2D[nonuniformEXT(metallicRoughnessMapHandle)], fragTexCoord).bg;
    float metallic  = clamp(mr.x, 0.0, 1.0);
    float roughness = clamp(mr.y, 0.045, 1.0);
    vec3 N = normalize(texture(uGlobalTextures2D[nonuniformEXT(normalMapHandle)], fragTexCoord).rgb * 2.0 - 1.0);

    // --- Vectors (tangent space) -----------------------------------------------------
    vec3 toLight = TBNLightPos - TBNfragPosition;
    float distance = length(toLight);
    vec3 L = toLight / distance;
    vec3 V = normalize(TBNViewPos - TBNfragPosition);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);  // avoid 0 -> division blow-up at grazing angles
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float attenuation = 1.0 / (0.5 * distance * distance);
    vec3 radiance = lightColor * attenuation;

    // --- Cook-Torrance BRDF ----------------------------------------------------------
    float NDF = DistributionGGX(NdotH, roughness);
    float G   = GeometrySmith(NdotV, NdotL, roughness);
    vec3  F   = FresnelSchlick(HdotV, F0);

    vec3 specular = (NDF * G * F) / (4.0 * NdotV * NdotL + 1e-4);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.1) * albedo * ao;

    vec3 color = ambient + calculateShadowPcf() * Lo;
    outColor = vec4(color, 1.0);
}
