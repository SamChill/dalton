#version 330 core
in vec2 quad;
out vec3 camera_origin;
out vec3 camera_direction;
out vec2 uv;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = vec4(quad, 0.0, 1.0);
    camera_origin = (inverse(view) * inverse(projection)*vec4(quad.x, quad.y, -1, 0.0)).xyz;
    camera_direction = (inverse(view) * vec4(0.0, 0.0, -1.0, 0.0)).xyz;
    uv = 0.5 * quad + 0.5;
}
