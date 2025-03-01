#version 450

layout(binding = 3) uniform sampler2D texSampler;
layout(binding = 4) uniform sampler2DShadow shadowMap;
layout(binding = 5) uniform sampler2D normalMap;
layout(binding = 6) uniform sampler2D metallicRoughnessMap;

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

float calculateShadow() {
    vec3 lightFrag = lightFragPosition.xyz / lightFragPosition.w;
    if(lightFrag.z >= 1.0)
        return 1.0;

    float sum = textureOffset(shadowMap, lightFrag.xyz, offsets[0])
        + textureOffset(shadowMap, lightFrag.xyz, offsets[1])
        + textureOffset(shadowMap, lightFrag.xyz, offsets[2])
        + textureOffset(shadowMap, lightFrag.xyz, offsets[3])
        + textureOffset(shadowMap, lightFrag.xyz, offsets[4])
        + textureOffset(shadowMap, lightFrag.xyz, offsets[5])
        + textureOffset(shadowMap, lightFrag.xyz, offsets[6])
        + textureOffset(shadowMap, lightFrag.xyz, offsets[7])
        + textureOffset(shadowMap, lightFrag.xyz, offsets[8]);

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
    vec3 albedo = texture(texSampler, fragTexCoord, 0).rgb;
    vec2 metallicRoughness = texture(metallicRoughnessMap, fragTexCoord).bg;
    vec3 normal = normalize(2.0 * texture(normalMap, fragTexCoord).rgb - 1.0);

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
    outColor = vec4(ambient + calculateShadow() * Lo, 1.0);
}