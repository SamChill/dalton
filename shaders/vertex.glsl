#version 330 core
in vec3 sphere_center;

uniform mat4 view;

void main()
{
    gl_Position = view * vec4(sphere_center, 1.0); 
}
