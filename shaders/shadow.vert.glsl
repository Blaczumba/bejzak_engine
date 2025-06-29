#version 450

layout(binding = 0) uniform Light {
    mat4 projView;

    vec3 pos;

} light;

layout( push_constant ) uniform Constants {
    mat4 model;
} pushConstants;

layout (location = 0) in vec3 inPosition;

void main() {
    gl_Position = light.projView * pushConstants.model * vec4(inPosition, 1.0);
} 
