#version 330 core

#include "lighting.glsl"

in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform float uTime;
uniform bool  uRainbow;
uniform float uAlpha;
uniform vec4  uTint;

vec3 hueToRgb(float hue)
{
    vec3 k = abs(fract(hue + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    return clamp(k - 1.0, 0.0, 1.0);
}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uSun.direction);

    vec3 color = vColor * (0.8 + 0.4 * max(dot(N, L), 0.0));

    if (uRainbow) {
        float hue = fract(dot(N, vec3(0.3, 0.5, 0.7)) + dot(N, V) * 0.5 + uTime * 0.2);
        color = mix(color, hueToRgb(hue) * 0.6 + 0.5, 0.55);
    }

    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
    color += vec3(1.0) * rim * 0.8;

    float glint = pow(max(dot(reflect(-L, N), V), 0.0), 40.0);
    color += vec3(1.0) * glint;

    color = mix(color, uTint.rgb, uTint.a);
    FragColor = vec4(applyFog(color, vWorldPos), uAlpha);
}
