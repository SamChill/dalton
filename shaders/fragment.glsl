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
uniform int outline;
uniform mat4 view;
uniform sampler1D sphere_texture;
uniform sampler1D radius_texture;
uniform sampler1D neighbor_texture;
uniform int neighbor_count;

void draw_imposter(vec3 local_coordinates) {
    float r = length(local_coordinates.xy);

    if (r > fs_in.radius) {
        discard;
    }

    if (outline == 1 && r > 0.97*fs_in.radius) {
        color = vec4(vec3(0.0), 1.0);
        return;
    }

    color.a = 1.0;
    color.rgb = mix(fs_in.sphere_color, vec3(1.0, 1.0, 1.0), 1.0-saturation);
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

int neighbor(int i) {
    float texel_count = float(neighbor_count * num_atoms);
    float texel_width = 1.0/texel_count;
    float shift = texel_width / 2.0;
    float coord = (fs_in.sphere_number * neighbor_count) + float(i) + shift;
    coord /= texel_count;
    return int(texture(neighbor_texture, coord).r);
}

void lighting(vec3 p, vec3 normal) {
    for (int k=0; k<neighbor_count; k++) {
        int i = neighbor(k);
        if (i == -1) continue;
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
