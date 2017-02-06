#version 330 core
// Input GL_POINTS that represent sphere centers.
layout (points) in;
// Output 4 triangles that represent a square centered on the sphere.
layout (triangle_strip, max_vertices=4) out;

out vec2 square_coordinates;
out vec3 sphere_coordinates;

uniform mat4 projection;
uniform float radius;

void main() {    
    vec4 sphere_center = gl_in[0].gl_Position;
    sphere_coordinates = vec3(sphere_center);

    // Generate a square centered on the sphere.
    gl_Position = projection * (sphere_center + vec4(-radius, -radius, 0.0, 0.0));
    square_coordinates = vec2(-1, -1);
    EmitVertex();   

    gl_Position = projection * (sphere_center + vec4(radius, -radius, 0.0, 0.0));
    square_coordinates = vec2(1, -1);
    EmitVertex();

    gl_Position = projection * (sphere_center + vec4(-radius, radius, 0.0, 0.0));
    square_coordinates = vec2(-1, 1);
    EmitVertex();

    gl_Position = projection * (sphere_center + vec4(radius, radius, 0.0, 0.0));
    square_coordinates = vec2(1, 1);
    EmitVertex();

    EndPrimitive();
}  
