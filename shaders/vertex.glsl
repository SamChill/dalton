#version 330 core

in float radius;
in float sphere_number;
in vec3 sphere_center;
in vec3 sphere_color;

out VS_OUT {
    float radius;
    vec3 sphere_color;
    float sphere_number;
} vs_out;

uniform mat4 view;

void main()
{
    gl_Position = view * vec4(sphere_center, 1.0); 
    vs_out.radius = radius;
    vs_out.sphere_color = sphere_color;
    vs_out.sphere_number = sphere_number;
}
