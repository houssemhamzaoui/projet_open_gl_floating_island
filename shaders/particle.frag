#version 330 core

in vec2  vCorner;
in float vAlpha;
in float vShape;
in vec3  vWorldPos;
out vec4 FragColor;

uniform vec3  uLight;
uniform vec3  uCameraPos;
uniform vec3  uFogColor;
uniform float uFogDensity;

void main()
{
    float r = length(vCorner);
    vec3 color;
    float alpha;
    if (vShape < 0.5) {
        alpha = (1.0 - abs(vCorner.x)) * (1.0 - vCorner.y * vCorner.y) * 0.7;
        color = vec3(0.75, 0.82, 0.95);
    } else if (vShape < 1.5) {
        alpha = 1.0 - smoothstep(0.25, 1.0, r);
        color = vec3(1.0);
    } else if (vShape < 2.5) {
        vec2 p = vCorner * vec2(1.25, 1.0);
        float notch = smoothstep(0.18, 0.0, length(p - vec2(0.0, 1.0)));
        alpha = (1.0 - smoothstep(0.75, 0.95, length(p))) * (1.0 - notch);
        color = mix(vec3(1.0, 0.92, 0.95), vec3(1.0, 0.62, 0.76), smoothstep(0.0, 0.8, length(p)));
    } else {
        alpha = smoothstep(0.55, 0.75, r) * (1.0 - smoothstep(0.85, 1.0, r)) * 0.8;
        color = vec3(0.8, 0.88, 1.0);
    }
    alpha *= vAlpha;
    if (alpha < 0.01)
        discard;

    color *= uLight;
    float d = length(vWorldPos - uCameraPos);
    color = mix(color, uFogColor, 1.0 - exp(-pow(d * uFogDensity, 2.0)));
    FragColor = vec4(color, alpha);
}
