#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices=4) out;

out vec3 oPos;
uniform mat4 pmat;
uniform float radius;

void main() {    
    // The input vertices are points that describe the center of a sphere.
    vec4 center = gl_in[0].gl_Position;
    float z = center.z;

    // Generate a square centered on the sphere center.
    gl_Position = center + pmat*vec4(-radius, -radius, 0.0, 0.0);
    oPos = vec3(-radius, -radius, z);
    EmitVertex();   

    gl_Position = center + pmat*vec4(radius, -radius, 0.0, 0.0);
    oPos = vec3(radius, -radius, z);
    EmitVertex();

    gl_Position = center + pmat*vec4(-radius, radius, 0.0, 0.0);
    oPos = vec3(-radius, radius, z);
    EmitVertex();

    gl_Position = center + pmat*vec4(radius, radius, 0.0, 0.0);
    oPos = vec3(radius, radius, z);
    EmitVertex();

    EndPrimitive();
}  
