#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, location = 0) in vec3 aPos;
layout(set = 0, location = 0) out vec3 localPos;

layout(push_constant) uniform PushConstantObject
{
	mat4 projection;
    mat4 view;
};

void main()
{
    localPos = aPos;  
    gl_Position =  projection * view * vec4(localPos, 1.0);
}