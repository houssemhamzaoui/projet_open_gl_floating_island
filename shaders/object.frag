#version 330 core

#include "lighting.glsl"

in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform vec4 uFlash;

void main()
{
    vec3 color = computeLighting(vColor, vNormal, vWorldPos);
    color = mix(color, uFlash.rgb, uFlash.a);
    FragColor = vec4(applyFog(color, vWorldPos), 1.0);
}
