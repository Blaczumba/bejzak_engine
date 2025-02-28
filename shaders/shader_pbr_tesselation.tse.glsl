#version 450

// Define the type of tessellation - triangles and equal spacing
layout(triangles, equal_spacing, cw) in;

// Output to fragment shader
layout(location = 0) out vec3 teTbnfragPosition;
layout(location = 1) out vec2 teFragTexCoord;
layout(location = 2) out vec4 teLightFragPosition;
layout(location = 3) out vec3 TbnLightPos;
layout(location = 4) out vec3 TbnViewPos;

layout(binding=0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
    vec3 viewPos;

} camera;

layout(binding = 1) uniform Light {
    mat4 projView;

    vec3 pos;

} light;

const mat4 BiasMat = mat4(
	0.5, 0, 0, 0,
	0, 0.5, 0, 0,
	0, 0, 1.0, 0,
	0.5, 0.5, 0.0, 1.0
);

struct OutputPatch {
    vec3 WorldPos_B030;
    vec3 WorldPos_B021;
    vec3 WorldPos_B012;
    vec3 WorldPos_B003;
    vec3 WorldPos_B102;
    vec3 WorldPos_B201;
    vec3 WorldPos_B300;
    vec3 WorldPos_B210;
    vec3 WorldPos_B120;
    vec3 WorldPos_B111;
    vec3 Normal[3];
    vec3 Tangent[3];
    vec2 TexCoord[3];
};

layout(location = 0) in patch OutputPatch outPatch;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {
    return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {
    return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float w = gl_TessCoord.z;

    float uPow3 = pow(u, 3);
    float vPow3 = pow(v, 3);
    float wPow3 = pow(w, 3);
    float uPow2 = pow(u, 2);
    float vPow2 = pow(v, 2);
    float wPow2 = pow(w, 2);


    teTbnfragPosition = outPatch.WorldPos_B300 * wPow3 +
                    outPatch.WorldPos_B030 * uPow3 +
                    outPatch.WorldPos_B003 * vPow3 +
                    outPatch.WorldPos_B210 * 3.0 * wPow2 * u +
                    outPatch.WorldPos_B120 * 3.0 * w * uPow2 +
                    outPatch.WorldPos_B201 * 3.0 * wPow2 * v +
                    outPatch.WorldPos_B021 * 3.0 * uPow2 * v +
                    outPatch.WorldPos_B102 * 3.0 * w * vPow2 +
                    outPatch.WorldPos_B012 * 3.0 * u * vPow2 +
                    outPatch.WorldPos_B111 * 6.0 * w * u * v;
    gl_Position = camera.proj * camera.view * vec4(teTbnfragPosition, 1.0);
    teLightFragPosition = BiasMat * light.projView * vec4(teTbnfragPosition, 1.0);

    vec3 teNormal = interpolate3D(outPatch.Normal[0], outPatch.Normal[1], outPatch.Normal[2]);
    vec3 teTangent = interpolate3D(outPatch.Tangent[0], outPatch.Tangent[1], outPatch.Tangent[2]);
    vec3 teBitangent = cross(teNormal, teTangent);
    mat3 TbnMat = transpose(mat3(teTangent, teBitangent, teNormal));

    teTbnfragPosition = TbnMat * teTbnfragPosition;
    teFragTexCoord = interpolate2D(outPatch.TexCoord[0], outPatch.TexCoord[1], outPatch.TexCoord[2]);

    TbnLightPos = TbnMat * light.pos;
    TbnViewPos = TbnMat * camera.viewPos;
}