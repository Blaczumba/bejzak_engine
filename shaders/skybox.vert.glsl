#version 450

layout (location = 0) in vec3 inPos;

layout(push_constant) uniform Constants {
    mat4 proj;
    mat3 view;
	uint skyboxHandle;

} pushConstants;

layout (location = 0) out vec3 outUVW;

void main() {
	outUVW = inPos;
	vec4 outPos = pushConstants.proj * mat4(pushConstants.view) * vec4(inPos.xyz, 1.0);
	gl_Position = outPos.xyww;
}
