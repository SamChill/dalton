#version 330 core
in vec2 uv;
uniform sampler2D accumulator_texture;
uniform sampler2D statistics_texture;
uniform sampler2D random_texture;
uniform vec2 rand_offset;

float rand() {
    return texture(random_texture, uv + rand_offset).x;
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

    //if (rand() < 0.5) {
    //    discard;
    //}

    //if (error < tol && N > 10) {
    //    discard;
    //}

    //if (length(sem) < 0.1) discard;

    //if (N > 20 && length(sem) < tol) {
    //    discard;
    //}

    //float r = rand() + 1;

    //if (N < 10) {
    //    return;
    //}

    //if (N > 50 && length(sem) < 0.01) {
    //    discard;
    //}

    //if (r*exp(-0.1*N) > length(sem)) {
    //    discard;
    //}
}
