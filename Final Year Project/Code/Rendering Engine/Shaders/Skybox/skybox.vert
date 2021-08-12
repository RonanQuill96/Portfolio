#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferFrame
{
	mat4 viewProjection;
} ubf;

layout(set = 0, location = 0) in vec3 inPosition;

layout(set = 0, location = 0) out vec3 outPosition;

void main()
{
	//Set Pos to xyww instead of xyzw, so that z will always be 1 (furthest from camera)
    vec4 position = ubf.viewProjection * vec4(inPosition, 1.0);
    gl_Position = position.xyww;

    outPosition = inPosition;
}
