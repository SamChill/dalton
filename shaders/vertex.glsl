#version 330 core
in vec3 sphere_center;

uniform mat4 view;
uniform mat4 model;
uniform mat4 zoom;

void main()
{
    // Rotate the sphere centers by the view matrix.
    gl_Position = zoom*model*view*vec4(sphere_center, 1.0); 
}
