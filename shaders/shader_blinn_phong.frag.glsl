#version 450

layout(binding=0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
    vec3 pos;
} camera;


layout(binding = 1) uniform Light {
    mat4 projView;

    vec3 pos;

} light;

layout(binding = 2) uniform ObjectUniform {
    mat4 model;

} object;

layout(binding = 3) uniform sampler2D texSampler;
layout(binding = 4) uniform sampler2DShadow shadowMap;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 lightFragPosition;
layout(location = 3) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

const int KELNER_SIZE = 9;  // size of offsets
const ivec2 offsets[] = ivec2[](
	ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1),
	ivec2(-1, 0), ivec2(0, 0), ivec2(1, 0),
	ivec2(-1, -1), ivec2(0, -1), ivec2(1, -1)
);

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

void main() {
    vec3 color = texture(texSampler, fragTexCoord).rgb;
    // vec3 color = 0.6 * vec3(1.0, 1.0, 1.0);
    const bool blinn = true;

    vec3 ambient = 0.05 * color;
    // diffuse
    vec3 lightDir = normalize(light.pos - fragPosition);
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 viewDir = normalize(camera.pos - fragPosition);
    float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }

    vec3 specular = vec3(0.3) * spec; // assuming bright white light color

    outColor = vec4(ambient + calculateShadow()*(diffuse + specular), 1.0);
}
