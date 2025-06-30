#version 450

layout( push_constant ) uniform Constants {
    mat4 model;
    mat4 lightProjView;

} pushConstants;

layout (location = 0) in vec3 inPosition;

void main() {
    gl_Position = pushConstants.lightProjView * pushConstants.model * vec4(inPosition, 1.0);
} 
