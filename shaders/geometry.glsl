#version 330 core
// Input GL_POINTS that represent sphere sphere_centers. 
layout (points) in;
// Output 4 triangles that represent a square sphere_centered on the sphere.
layout (triangle_strip, max_vertices=4) out;

out vec2 square_coordinates;
out vec3 sphere_coordinates;

uniform mat4 perspective;
uniform float radius;
uniform mat4 zoom;

void main() {    
    vec4 sphere_center = gl_in[0].gl_Position;
    sphere_coordinates = vec3(sphere_center);

    // Generate a square sphere_centered on the sphere sphere_center.
    gl_Position = sphere_center + zoom*perspective*vec4(-radius, -radius, 0.0, 0.0);
    square_coordinates = vec2(-radius, -radius);
    EmitVertex();   

    gl_Position = sphere_center + zoom*perspective*vec4(radius, -radius, 0.0, 0.0);
    square_coordinates = vec2(radius, -radius);
    EmitVertex();

    gl_Position = sphere_center + zoom*perspective*vec4(-radius, radius, 0.0, 0.0);
    square_coordinates = vec2(-radius, radius);
    EmitVertex();

    gl_Position = sphere_center + zoom*perspective*vec4(radius, radius, 0.0, 0.0);
    square_coordinates = vec2(radius, radius);
    EmitVertex();

    EndPrimitive();
}  
