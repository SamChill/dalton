#version 330 core
in vec2 quad;
out vec2 uv;

void main() {
    gl_Position = vec4(quad, 0.0, 1.0);
    uv = 0.5 * quad + 0.5;
}
