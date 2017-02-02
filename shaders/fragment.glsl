#version 330 core
in vec2 square_coordinates;
in vec3 sphere_coordinates;
out vec4 color;

uniform float radius;
uniform mat4 view;

void main()
{
    float r = length(square_coordinates);
    
    if (r < radius) {
        float d = sqrt(1 - r / radius);

        // Darker towards the back of the scene.
        float z = (sphere_coordinates.z + 1) / 2.0;
        d = z * d;

        color = vec4(d, d, d, 1.0);

        gl_FragDepth = 1-z;
    }else{
        discard;
    }
}  
