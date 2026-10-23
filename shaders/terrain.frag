#version 330 core

#include "lighting.glsl"

in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform vec3  uHole;
uniform float uHoleMinY;

void main()
{
    if (uHole.z > 0.0 && length(vWorldPos.xz - uHole.xy) < uHole.z && vWorldPos.y > uHoleMinY)
        discard;
    vec3 color = computeLighting(vColor, vNormal, vWorldPos);
    FragColor = vec4(applyFog(color, vWorldPos), 1.0);
}
