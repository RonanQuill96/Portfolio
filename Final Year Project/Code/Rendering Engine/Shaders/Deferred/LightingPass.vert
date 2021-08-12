#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, location = 0) in vec3 inPosition;
layout(set = 0, location = 1) in vec2 inTexCoord;

layout(set = 0, location = 1) out vec2 fragTexCoord;
layout(set = 0, location = 0) out vec3 fragPosition;

void main() {    
	fragTexCoord = inTexCoord;
	gl_Position = vec4(fragTexCoord * 2.0f - 1.0f, 0.0f, 1.0f);
    fragPosition = inPosition;
}