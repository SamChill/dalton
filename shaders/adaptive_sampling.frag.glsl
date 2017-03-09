#version 330 core
in vec2 uv;
uniform sampler2D accumulator_texture;
uniform sampler2D statistics_texture;
uniform sampler2D random_texture;
uniform vec2 rand_offset;
uniform float weight;

float rand() {
    return 0.5*texture(random_texture, uv + rand_offset).x + 0.5;
}

const float tol = 0.1;

void main() {
    vec3 sum = texture(accumulator_texture, uv).rgb;
    vec3 sum_sq = texture(statistics_texture, uv).rgb;
    float N = texture(statistics_texture, uv).a;
    vec3 mean = sum / N;
    vec3 var = sum_sq / N - mean*mean;
    vec3 stddev = sqrt(var);
    vec3 sem = stddev / sqrt(N);
    float error = length(sem);

    if (weight*rand() > error && N > 10) {
        discard;
    }
}
