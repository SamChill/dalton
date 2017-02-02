#version 330 core
in vec2 square_coordinates;
in vec3 sphere_coordinates;
out vec4 color;

uniform float radius, gradient;

void main()
{
    float r = length(square_coordinates);
    
    if (r < 1) {
        // Local sphere coordinates. 
        // lx and ly in [-1, 1] 
        // lz in [0,1]
        vec3 local;
        local.x = square_coordinates.x;
        local.y = square_coordinates.y;
        local.z = sqrt(1 - local.x*local.x - local.y*local.y);

        // Color the sphere according to its local height.
        color.rgb = vec3(pow(local.z, gradient));
        color.a = 1.0;

        // Determine the global z position.
        float global_z = sphere_coordinates.z + radius*local.z;

        // Rescale z to be from 0 to 1.
        float depth = (global_z + 1.0) / 2.0;

        // Darken according to global depth.
        color.rgb *= vec3(depth) + 0.1;

        gl_FragDepth = 1-depth;
    }else{
        discard;
    }
}  
