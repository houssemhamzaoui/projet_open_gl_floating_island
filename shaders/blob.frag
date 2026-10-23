#version 330 core

in vec2 vLocal;
out vec4 FragColor;

uniform float uOpacity;
uniform vec3  uColor;

void main()
{
    float distance = length(vLocal);
    float alpha = uOpacity * (1.0 - smoothstep(0.35, 1.0, distance));
    FragColor = vec4(uColor, alpha);
}
