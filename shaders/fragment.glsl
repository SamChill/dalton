#version 330 core
in vec3 oPos;
out vec4 color;

uniform float radius;

void main()
{
    // oPos is the 2d position inside the square.
    // r is the distance from the center. The center is always (0, 0).
    float r = length(oPos.xy);
    
    if (r < radius) {
        float d = sqrt(1 - r / radius);

        // Darker towards the back of the scene.
        float z = (oPos.z + 1) / 2.0;
        d = z * d;

        color = vec4(d, d, d, 1.0);
    }else{
        discard;
    }
}  
