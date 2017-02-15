#version 330 core
// Input GL_POINTS that represent sphere centers.
layout (points) in;
// Output 4 triangles that represent a square centered on the sphere.
layout (triangle_strip, max_vertices=4) out;

in VS_OUT {
    float radius;
    vec3 sphere_color;
    int sphere_number;
} gs_in[];

out GS_OUT {
    float radius;
    vec3 billboard_coordinates;
    vec3 sphere_center;
    vec3 sphere_color;
    flat int sphere_number;
} gs_out;

uniform float radius_scale;
uniform mat4 projection;
uniform float outline;

void main() {    

    // Calculate the radius.
    float r = radius_scale * (gs_in[0].radius + outline);

    vec2 corners[4] = vec2[4](
        vec2(-r, -r), vec2(r, -r), vec2(-r, r), vec2(r, r)
    );


    // Get the center of the sphere. This has already been rotated.
    vec4 sphere_center = gl_in[0].gl_Position;

    // Generate a square centered on the sphere.
    for (int i=0; i<4; i++) {
        vec2 corner = corners[i];

        // Calculate the world coordinates of the billboard.
        gs_out.billboard_coordinates = sphere_center.xyz;
        gs_out.billboard_coordinates.xy += corner;

        // Set some data to forward to the fragment shader.
        gs_out.radius = radius_scale * gs_in[0].radius;
        gs_out.sphere_color  = gs_in[0].sphere_color;
        gs_out.sphere_number = gs_in[0].sphere_number;
        gs_out.sphere_center = vec3(sphere_center);

        // Set the position of the vertex and apply the projection to screen space.
        gl_Position = projection * (sphere_center + vec4(corner, 0.0, 0.0));
        EmitVertex();   
    }

    EndPrimitive();
}  
