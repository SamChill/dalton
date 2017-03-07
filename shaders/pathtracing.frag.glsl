#version 330 core
in vec3 camera_origin;
in vec3 camera_direction;
in vec2 uv;
layout (location = 0) out vec4 color;
layout (location = 1) out vec4 statistics;

uniform float shininess;
uniform int max_bounces;
uniform float radius_scale;
uniform float saturation;
uniform sampler1D sphere_texture;
uniform sampler1D sphere_color_texture;
uniform sampler1D material_texture;
uniform sampler2D accumulator_texture;
uniform sampler2D statistics_texture;
uniform sampler2D random_texture;
uniform ivec2 screen_size;
uniform mat4 view;
uniform mat4 projection;
uniform int sphere_count;
uniform vec2 rand_offset;
uniform vec4 background_color;
uniform float sampling_weight;
uniform float focal_distance;
uniform float focal_strength;
uniform float ambient_light;
uniform float direct_light;

const vec4 BLACK = vec4(0.0, 0.0, 0.0, 1.0);
const vec4 WHITE = vec4(1.0, 1.0, 1.0, 1.0);
vec2 rand_state = rand_offset;

vec3 rand3() {
    vec3 r = texture(random_texture, uv + rand_state).xyz;
    rand_state = r.xy;
    return r;
}

vec3 randomInHemisphere(vec3 normal) {
    vec3 d = rand3();
    return sign(dot(normal, d)) * d;
}

struct Sphere {
    vec3 position;
    float radius;
    vec4 color;
    float shininess;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

Sphere sphereLookup(int i) {
    vec4 data = texelFetch(sphere_texture, i, 0);
    vec3 position = data.rgb;
    float radius = radius_scale * data.a;
    vec4 color = texelFetch(sphere_color_texture, i, 0);
    color.rgb = mix(color.rgb, vec3(1.0), 1.0-saturation);
    color.a = 1.0;
    vec4 material = texelFetch(material_texture, i, 0);
    float shininess = material.r;
    Sphere sphere = Sphere(position, radius, color, shininess);
    return sphere;
}

float raySphereIntersect(Ray ray, Sphere sphere) {
    float a = dot(ray.direction, ray.direction);
    vec3 s0_r0 = ray.origin - sphere.position;
    float b = 2.0 * dot(ray.direction, s0_r0);
    float c = dot(s0_r0, s0_r0) - (sphere.radius * sphere.radius);
    float disc = b*b - 4.0*a*c;
    if (disc < 0.0) {
        return -1.0;
    }
    return (-b - sqrt(disc))/(2.0*a);
}

Sphere hitSphere(Ray ray, out bool hit, out float time, out int idx) {
    time = 1e20;
    idx = -1;
    Sphere hit_sphere;

    Sphere sphere;
    for (int i=0; i<sphere_count; i++) {
        sphere = sphereLookup(i);
        float t0 = raySphereIntersect(ray, sphere);
        if (t0 > 0 && t0 < time) {
            time = t0;
            idx = i;
            hit_sphere = sphere;
        }
    }
    if (idx == -1) {
        hit = false;
        return sphere;
    }else{
        hit = true;
        return hit_sphere;
    }
}

void main() {
    vec3 jitter = vec3(rand3().xy + vec2(0.5), 0.0);
    jitter.x /= 2*float(screen_size.x);
    jitter.y /= 2*float(screen_size.y);
    jitter = (inverse(view)*inverse(projection)*vec4(jitter, 0.0)).xyz;

    Ray ray = Ray(
        camera_origin + jitter, 
        camera_direction
    );

    float focal_distance = length(ray.origin) + focal_distance;
    vec3 focal_point = ray.origin + focal_distance * ray.direction + jitter;
    vec3 dof_jitter = focal_strength*vec3(rand3().xy, 0.0);
    dof_jitter = (inverse(view)*inverse(projection)*vec4(dof_jitter, 0.0)).xyz;

    ray = Ray(
        ray.origin+dof_jitter, 
        normalize(focal_point - (ray.origin+ dof_jitter))
    );

    color = vec4(0.0);
    vec4 mask = vec4(1.0);
    for (int bounces=0; bounces<max_bounces+1; bounces++) {
        bool hit;
        float time;
        int idx;
        Sphere hitsphere = hitSphere(ray, hit, time, idx);
        if (!hit) {
            if (bounces == 0) {
                color = background_color;
            }else{
                color += ambient_light * mask * WHITE;
                vec3 direct_light_direction = -camera_direction;
                
                float dlight = clamp(
                    dot(
                        ray.direction, 
                        direct_light_direction
                    ), 0.0,1.0);
                color += mask * direct_light * dlight;
            }
            break;
        }
        vec3 P = ray.origin + ray.direction * time;
        vec3 normal = normalize(P - hitsphere.position);
        vec3 outgoing_dir = hitsphere.shininess*shininess * reflect(ray.direction, normal) + 
                (1.0 - hitsphere.shininess*shininess) * randomInHemisphere(normal);
        ray = Ray(
            P  + 0.0001 * outgoing_dir, 
            outgoing_dir
        );

        mask *= hitsphere.color;
    }
    statistics.rgb = color.rgb * color.rgb + texture(statistics_texture, uv).rgb;
    statistics.a = 1 + texture(statistics_texture, uv).a;
    color += texture(accumulator_texture, uv);
    color.a = 1.0;
}
