#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, location = 0) in vec3 inColour;

layout(set = 0, location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(inColour, 1.0);
}