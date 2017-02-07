#version 330 core
// Input GL_POINTS that represent sphere centers.
layout (points) in;
// Output 4 triangles that represent a square centered on the sphere.
layout (triangle_strip, max_vertices=4) out;

in VS_OUT {
    float radius;
    vec3 sphere_color;
} gs_in[];

out vec2 square_coordinates;
out vec3 sphere_coordinates;
out vec3 f_sphere_color;

uniform float radius_scale;
uniform mat4 projection;

void main() {    
    vec4 sphere_center = gl_in[0].gl_Position;
    sphere_coordinates = vec3(sphere_center);

    float r = radius_scale * gs_in[0].radius;

    // Generate a square centered on the sphere.
    gl_Position = projection * (sphere_center + vec4(-r, -r, 0.0, 0.0));
    square_coordinates = vec2(-1, -1);
    f_sphere_color = gs_in[0].sphere_color;
    EmitVertex();   

    gl_Position = projection * (sphere_center + vec4(r, -r, 0.0, 0.0));
    square_coordinates = vec2(1, -1);
    f_sphere_color = gs_in[0].sphere_color;
    EmitVertex();

    gl_Position = projection * (sphere_center + vec4(-r, r, 0.0, 0.0));
    square_coordinates = vec2(-1, 1);
    f_sphere_color = gs_in[0].sphere_color;
    EmitVertex();

    gl_Position = projection * (sphere_center + vec4(r, r, 0.0, 0.0));
    square_coordinates = vec2(1, 1);
    f_sphere_color = gs_in[0].sphere_color;
    EmitVertex();

    EndPrimitive();
}  
