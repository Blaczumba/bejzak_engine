#version 450

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, rg8) writeonly uniform image2DArray shadingMap;

layout(push_constant) uniform Constants {
    uvec2 mousePos; // Pixel coords.
} pcs;

float getRate(float dist) {
    return 1.0;
    if (dist < 5.0) return 1.0;
    if (dist < 10.0) return 0.75;
    if (dist < 15.0) return 0.5;
    if (dist < 24.0) return 0.25;
    return 0.1;
}

void main() {
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID);
    uint layer = texelCoord.z;

    ivec2 size = imageSize(shadingMap).xy;

    if (texelCoord.x >= size.x || texelCoord.y >= size.y) return;

    // Convert mouse pixel coordinates to tile coordinates (16x16 tiles)
    vec2 mouseTilePos = vec2(pcs.mousePos) / 16.0;
    float d = distance(vec2(texelCoord), mouseTilePos);
    float rate = getRate(d);

    // Store the rate in the red channel
    imageStore(shadingMap, texelCoord, vec4(rate, rate, 0, 0));
}