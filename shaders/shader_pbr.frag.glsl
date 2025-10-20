#version 450

#include "bindless.glsl"

layout(push_constant) uniform Constants {
    mat4 model;
    uint light;
    uint diffuse;
    uint normal;
    uint metallicRoughness;
    uint shadow;

} pushConstants;

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
const vec3 lightColor = vec3(10.0);
const float ao = 1.0;

// PCSS
const int BLOCKER_SEARCH_SAMPLES = 16;
const int PCF_SAMPLES = 25;
const float LIGHT_SIZE = 0.5; // tweak per light
const float MIN_FILTER_SIZE = 2.0 / 2048.0;

const vec2 poissonDisk[25] = vec2[](
    vec2(-0.942, -0.399), vec2(0.945, -0.768),
    vec2(-0.094, -0.929), vec2(0.345,  0.294),
    vec2(-0.915,  0.783), vec2(-0.878, -0.1),
    vec2(0.767,  0.857),  vec2(0.353, -0.874),
    vec2(0.558, -0.212),  vec2(-0.354, -0.116),
    vec2(-0.669, -0.69),  vec2(-0.775,  0.463),
    vec2(0.203, -0.604),  vec2(0.963,  0.527),
    vec2(-0.423,  0.915), vec2(-0.639, -0.229),
    vec2(-0.377, -0.857), vec2(0.591, -0.56),
    vec2(0.759,  0.169),  vec2(-0.015,  0.221),
    vec2(0.278,  0.748),  vec2(-0.83,   0.093),
    vec2(-0.486,  0.561), vec2(0.713,  0.285),
    vec2(-0.265, -0.364)
);

// float averageBlockerDepth(vec2 uv, float receiverDepth, float searchRadius)
// {
//     float blockerSum = 0.0;
//     int blockerCount = 0;
// 
//     for (int i = 0; i < BLOCKER_SEARCH_SAMPLES; ++i) {
//         float shadowDepth = texture(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], uv + poissonDisk[i] * searchRadius).r;
//         if (shadowDepth < receiverDepth) {
//             blockerSum += shadowDepth;
//             blockerCount++;
//         }
//     }
// 
//     if (blockerCount == 0)
//         return -1.0; // no blockers
// 
//     return blockerSum / float(blockerCount);
// }
// 
// float filterPcf(vec2 uv, float receiverDepth, float filterRadius)
// {
//     float sum = 0.0;
//     for (int i = 0; i < PCF_SAMPLES; ++i) {
//         float shadowDepth = texture(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], uv + poissonDisk[i] * filterRadius).r;
//         sum += float(receiverDepth <= shadowDepth);
//     }
//     return sum / float(PCF_SAMPLES);
// }
// 
// float calculateShadowPcss() {
//     vec3 lightFrag = lightFragPosition.xyz / lightFragPosition.w;
//     if (lightFrag.z >= 1.0)
//         return 1.0;
// 
//     float receiverDepth = lightFrag.z;
// 
//     vec2 uv = lightFrag.xy;
// 
//     float searchRadius = LIGHT_SIZE * (receiverDepth / lightFragPosition.w);
//     float avgBlockerDepth = averageBlockerDepth(uv, receiverDepth, searchRadius);
// 
//     if (avgBlockerDepth < 0.0)
//         return 1.0;
// 
//     float penumbraRatio = (receiverDepth - avgBlockerDepth) / avgBlockerDepth;
//     float filterRadius = clamp(penumbraRatio * LIGHT_SIZE, MIN_FILTER_SIZE, 0.02);
// 
//     float shadow = filterPcf(uv, receiverDepth, filterRadius);
// 
//     return shadow;
// }

// PCSS

float calculateShadowPcf() {
    vec3 lightFrag = lightFragPosition.xyz / lightFragPosition.w;
    if(lightFrag.z >= 1.0)
        return 1.0;

    float sum = textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[0])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[1])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[2])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[3])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[4])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[5])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[6])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[7])
        + textureOffset(uGlobalTexturesShadow[nonuniformEXT(pushConstants.shadow)], lightFrag.xyz, offsets[8]);

    return sum / KELNER_SIZE;
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

void main() {
    vec3 albedo = texture(uGlobalTextures2D[nonuniformEXT(pushConstants.diffuse)], fragTexCoord, 0).rgb;
    vec2 metallicRoughness = texture(uGlobalTextures2D[nonuniformEXT(pushConstants.metallicRoughness)], fragTexCoord).bg;
    vec3 normal = normalize(2.0 * texture(uGlobalTextures2D[nonuniformEXT(pushConstants.normal)], fragTexCoord).rgb - 1.0);

    vec3 lightDir = normalize(TBNLightPos - TBNfragPosition);
    vec3 viewDir = normalize(TBNViewPos - TBNfragPosition);
    vec3 halfwayDir = normalize(lightDir + viewDir); 

    vec3 F0 = mix(vec3(0.1), albedo, metallicRoughness.r);
    
    float distance = length(TBNLightPos - TBNfragPosition);
    float attenuation = 1.0 / (0.40 * distance);
    vec3 radiance = lightColor * attenuation;

    float NDF = DistributionGGX(normal, halfwayDir, metallicRoughness.g);   
    float G = GeometrySmith(normal, viewDir, lightDir, metallicRoughness.g); 
    vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - max(dot(halfwayDir, viewDir), 0.0), 0.0, 1.0), 5.0);

    float NdotV = max(dot(normal, viewDir), 0.0);
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallicRoughness.r);	  
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL; 

    vec3 ambient = vec3(0.1) * albedo * ao;
    outColor = vec4(ambient + calculateShadowPcf() * Lo, 1.0);
}