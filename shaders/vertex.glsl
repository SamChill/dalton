#version 330 core
in vec3 position;

uniform mat4 view;

void main()
{
    gl_Position = view*vec4(position, 1.0); 
}
