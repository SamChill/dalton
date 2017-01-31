#version 330
in vec2 gl_FragCoord;
out vec4 color;
uniform vec3 iResolution;
uniform float intensity;
void main() {
    vec2 uv = gl_FragCoord.xy;
    float radius = length(uv);
    if (radius > 600.0)
        color = vec4(vec3(intensity), 1.0);
    else
        color = vec4(1.0, 0.0, 0.0, 1.0);
}
