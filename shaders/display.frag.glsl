#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D accumulator_texture;
uniform sampler2D statistics_texture;

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

    color.rgb = mean.rgb;
    color.a = 1.0;
}
