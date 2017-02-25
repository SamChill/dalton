#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D accumulator_texture;
uniform int samples;
uniform float sampling_weight;

void main() {
    float w = 1 + (samples-1)*sampling_weight;
    color = texture(accumulator_texture, uv) / w;
    color.a = 1.0;
}
