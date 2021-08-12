#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, location = 0) in vec3 inPosition;

layout(set = 0, location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform samplerCube cubeMapTexture;

void main() 
{
    outColor = texture(cubeMapTexture, inPosition);
}