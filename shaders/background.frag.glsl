#version 330 core
in vec2 uv;
out vec4 color;
uniform vec4 background_color;

void main() {
    color = background_color;
}
