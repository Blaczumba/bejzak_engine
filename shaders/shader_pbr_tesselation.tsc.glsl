#version 450

// Constants for tessellation levels
const float tessLevelOuter = 1.0;  // Outer tessellation level
const float tessLevelInner = 1.0;  // Inner tessellation level

layout(vertices = 1) out;

layout(location = 0) in vec2 inTexCoord[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec3 inTangent[];

struct OutputPatch
{
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

// Output to tessellation evaluation shader
layout(location = 0) out patch OutputPatch outPatch;

vec3 ProjectToPlane(vec3 Point, vec3 PlanePoint, vec3 PlaneNormal) {
    vec3 v = Point - PlanePoint;
    float Len = dot(v, PlaneNormal);
    vec3 d = Len * PlaneNormal;
    return (Point - d);
}

void CalcPositions() {
    // The original vertices stay the same
    outPatch.WorldPos_B030 = gl_in[0].gl_Position.xyz;
    outPatch.WorldPos_B003 = gl_in[1].gl_Position.xyz;
    outPatch.WorldPos_B300 = gl_in[2].gl_Position.xyz;

    // Edges are names according to the opposing vertex
    vec3 EdgeB300 = outPatch.WorldPos_B003 - outPatch.WorldPos_B030;
    vec3 EdgeB030 = outPatch.WorldPos_B300 - outPatch.WorldPos_B003;
    vec3 EdgeB003 = outPatch.WorldPos_B030 - outPatch.WorldPos_B300;

    // Generate two midpoints on each edge
    outPatch.WorldPos_B021 = outPatch.WorldPos_B030 + EdgeB300 / 3.0;
    outPatch.WorldPos_B012 = outPatch.WorldPos_B030 + EdgeB300 * 2.0 / 3.0;
    outPatch.WorldPos_B102 = outPatch.WorldPos_B003 + EdgeB030 / 3.0;
    outPatch.WorldPos_B201 = outPatch.WorldPos_B003 + EdgeB030 * 2.0 / 3.0;
    outPatch.WorldPos_B210 = outPatch.WorldPos_B300 + EdgeB003 / 3.0;
    outPatch.WorldPos_B120 = outPatch.WorldPos_B300 + EdgeB003 * 2.0 / 3.0;

    // Project each midpoint on the plane defined by the nearest vertex and its normal
    outPatch.WorldPos_B021 = ProjectToPlane(outPatch.WorldPos_B021, outPatch.WorldPos_B030,
                                          outPatch.Normal[0]);
    outPatch.WorldPos_B012 = ProjectToPlane(outPatch.WorldPos_B012, outPatch.WorldPos_B003,
                                         outPatch.Normal[1]);
    outPatch.WorldPos_B102 = ProjectToPlane(outPatch.WorldPos_B102, outPatch.WorldPos_B003,
                                         outPatch.Normal[1]);
    outPatch.WorldPos_B201 = ProjectToPlane(outPatch.WorldPos_B201, outPatch.WorldPos_B300,
                                         outPatch.Normal[2]);
    outPatch.WorldPos_B210 = ProjectToPlane(outPatch.WorldPos_B210, outPatch.WorldPos_B300,
                                         outPatch.Normal[2]);
    outPatch.WorldPos_B120 = ProjectToPlane(outPatch.WorldPos_B120, outPatch.WorldPos_B030,
                                         outPatch.Normal[0]);

    // Handle the center
    vec3 Center = (outPatch.WorldPos_B003 + outPatch.WorldPos_B030 + outPatch.WorldPos_B300) / 3.0;
    outPatch.WorldPos_B111 = (outPatch.WorldPos_B021 + outPatch.WorldPos_B012 + outPatch.WorldPos_B102 +
                          outPatch.WorldPos_B201 + outPatch.WorldPos_B210 + outPatch.WorldPos_B120) / 6.0;
    outPatch.WorldPos_B111 += (outPatch.WorldPos_B111 - Center) / 2.0;
}

void main() {
    // Pass through the per-vertex data to the tessellation evaluation shader
    // gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;
    // outTexCoord[gl_InvocationID] = inTexCoord[gl_InvocationID];
    // outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    for (int i = 0; i < 3; i++) {
       outPatch.Normal[i] = inNormal[i];
       outPatch.Tangent[i] = inTangent[i];
       outPatch.TexCoord[i] = inTexCoord[i];
    }

    CalcPositions();

    gl_TessLevelOuter[0] = tessLevelOuter;
    gl_TessLevelOuter[1] = tessLevelOuter;
    gl_TessLevelOuter[2] = tessLevelOuter;
    gl_TessLevelInner[0] = tessLevelInner;
}