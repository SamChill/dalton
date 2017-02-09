#version 330 core

in GS_OUT {
    float radius;
    vec3 billboard_coordinates;
    vec3 sphere_center;
    vec3 sphere_color;
    float sphere_number;
} fs_in;

out vec4 color;

uniform float box_size;
uniform float ambient_occlusion;
uniform float radius_scale;
uniform float saturation;
uniform int num_atoms;
uniform mat4 view;
uniform sampler1D sphere_texture;
uniform sampler1D radius_texture;

void draw_imposter(vec3 local_coordinates) {
    float r = length(local_coordinates.xy);
    if (r > fs_in.radius) {
        discard;
    }

    color.a = 1.0;
    color.rgb = mix(fs_in.sphere_color, vec3(1,1,1), 1-saturation);
}

void determine_depth(float z) {
    gl_FragDepth = 1.0 - (z + box_size/2.0) / box_size;
}

float sphere_z(float x, float y, float r) {
    return sqrt(r*r - x*x - y*y);
}

vec3 sphere_lookup(int i) {
    float h = 0.5/float(num_atoms);
    vec4 c = texture(sphere_texture, (float(i)+h)/(float(num_atoms)));
    return (view*c).rgb;
}

float radius_lookup(int i) {
    float h = 0.5/float(num_atoms);
    return radius_scale * texture(radius_texture, (float(i)+h)/(float(num_atoms))).r;
}

void lighting(vec3 p, vec3 normal) {
    float bl = 0.0;
    for (int i=0; i<num_atoms; i++) {
        if (i == int(fs_in.sphere_number)) {
            continue;
        }
        vec3 q = sphere_lookup(i);
        float cos_alpha = dot(normalize(q-p), normal);
        if (cos_alpha < 0.0) {
            continue;
        }
        float r = radius_lookup(i);
        float d = distance(q, p);
        color.rgb *= 1.0 - ambient_occlusion*cos_alpha*(r/d)*(r/d);
    }
}

void main()
{
    // The local coordinates are centered on the spheres (i.e. the sphere
    // center is the origin).
    vec3 local_coordinates = fs_in.billboard_coordinates - fs_in.sphere_center;
    local_coordinates.z = sphere_z(local_coordinates.x, local_coordinates.y, fs_in.radius);

    // The global coordinates are the point on the sphere in in world space.
    vec3 global_coordinates = fs_in.billboard_coordinates;
    global_coordinates.z += local_coordinates.z;

    // Draw the imposters.
    draw_imposter(local_coordinates);

    // Determine the gl_FragDepth.
    determine_depth(global_coordinates.z);

    // Lighting.
    vec3 normal = normalize(local_coordinates);
    lighting(global_coordinates, normal);
}  
