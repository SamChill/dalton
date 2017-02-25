#version 330 core

in GS_OUT {
    float radius;
    vec3 billboard_coordinates;
    vec3 sphere_center;
    vec3 sphere_color;
    flat int sphere_number;
} fs_in;

out vec4 color;

uniform float ambient_occlusion;
uniform float box_size;
uniform float decay;
uniform float eta;
uniform float outline;
uniform float radius_scale;
uniform float saturation;
uniform int neighbor_count;
uniform mat4 view;
uniform sampler1D sphere_texture;
uniform isampler2D neighbor_texture;

void draw_imposter(vec3 local_coordinates) {
    // Compute the distance from the center of the bill board quad to the current point.
    float r = length(local_coordinates.xy);

    // Discard fragments that are outside the sphere and border.
    if (r > fs_in.radius + outline*outline) {
        discard;
    }

    // Set the color of the sphere according to the predefined sphere color.
    color.rgb = mix(fs_in.sphere_color, vec3(1.0), 1.0-saturation);
    // Draw a black outline.
    color.rgb *= step(0.0, fs_in.radius - r); 
    color.a = 1.0;
}

void determine_depth(float z) {
    gl_FragDepth = 1.0 - (z + (box_size+10)/2.0) / (box_size+10);
}

float sphere_z(vec2 xy) {
    float r = fs_in.radius;
    float z_sq = r*r - dot(xy, xy);
    if (z_sq > 0.0) {
        return sqrt(z_sq);
    }else{
        return -eta*(length(xy) - r);
    }
}

vec4 sphere(int i) {
    return view * texelFetch(sphere_texture, i, 0);
}

int neighbor(int i) {
    ivec2 coord = ivec2(i, fs_in.sphere_number);
    return texelFetch(neighbor_texture, coord, 0).r;
}

void lighting(vec3 p, vec3 normal) {
    for (int k=0; k<neighbor_count; k++) {
        int i = neighbor(k);
        vec4 s = sphere(i);
        vec3 q = s.rgb;
        float cos_alpha = dot(normalize(q-p), normal);
        float r = s.a;
        float d = distance(q, p);
        color.rgb *= 1-ambient_occlusion*max(cos_alpha,0)*pow(r/d, decay);
    }
}

void main()
{
    // The local coordinates are centered on the spheres (i.e. the sphere
    // center is the origin).
    vec3 local_coordinates = fs_in.billboard_coordinates - fs_in.sphere_center;
    local_coordinates.z = sphere_z(local_coordinates.xy);

    // The global coordinates are the point on the sphere in in world space.
    vec3 global_coordinates = fs_in.billboard_coordinates;
    global_coordinates.z += local_coordinates.z;

    // Determine the gl_FragDepth.
    determine_depth(global_coordinates.z);

    // Draw the imposters.
    draw_imposter(local_coordinates);

    // Lighting.
    if (ambient_occlusion > 0.01) {
        vec3 normal = normalize(local_coordinates);
        lighting(global_coordinates, normal);
    }
}  
