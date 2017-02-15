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
uniform int max_neighbor_count;
uniform mat4 view;
uniform sampler1D sphere_texture;
uniform sampler1D radius_texture;
uniform sampler1D neighbor_texture;

void draw_imposter(vec3 local_coordinates) {
    float r = length(local_coordinates.xy);

    if (r > fs_in.radius + outline*outline) {
        discard;
    }

    color.a = 1.0;
    color.rgb = mix(fs_in.sphere_color, vec3(1.0, 1.0, 1.0), 1.0-saturation);

    if (r > fs_in.radius) {
        color.rgb = vec3(0.0);
    }
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

vec3 sphere_lookup(int i) {
    vec4 c = texelFetch(sphere_texture, i, 0);
    return (view*c).rgb;
}

float radius_lookup(int i) {
    return radius_scale * texelFetch(radius_texture, i, 0).r;
}

int neighbor(int i) {
    return int(texelFetch(neighbor_texture, max_neighbor_count*fs_in.sphere_number + i, 0).r);
}

void lighting(vec3 p, vec3 normal) {
    for (int k=0; k<neighbor_count; k++) {
        int i = neighbor(k);
        vec3 q = sphere_lookup(i);
        float cos_alpha = dot(normalize(q-p), normal);
        if (cos_alpha < 0.0) {
            continue;
        }
        float r = radius_lookup(i);
        float d = distance(q, p);
        color.rgb *= 1-ambient_occlusion*cos_alpha*pow(r/d, decay);
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
