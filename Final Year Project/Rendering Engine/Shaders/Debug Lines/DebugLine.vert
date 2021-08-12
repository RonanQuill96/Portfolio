#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstantObject
{
    mat4 wvm;
} pushConstants;

layout(set = 0, location = 0) in vec3 inPosition;
layout(set = 0, location = 1) in vec3 inColour;

layout(set = 0, location = 0) out vec3 outColour;

void main() 
{
    gl_Position = pushConstants.wvm * vec4(inPosition, 1.0);
    outColour = inColour;
}