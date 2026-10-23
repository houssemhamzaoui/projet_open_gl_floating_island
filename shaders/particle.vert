#version 330 core

layout(location = 0) in vec2 aCorner;
layout(location = 1) in vec4 aPositionSize;
layout(location = 2) in vec4 aParams;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;

out vec2  vCorner;
out float vAlpha;
out float vShape;
out vec3  vWorldPos;

void main()
{
    float size = aPositionSize.w;
    float shape = aParams.w;
    vec2 c = aCorner * vec2(size, size * aParams.y);
    float r = aParams.x;
    c = vec2(c.x * cos(r) - c.y * sin(r), c.x * sin(r) + c.y * cos(r));

    vec3 up = shape < 0.5 ? vec3(0.0, 1.0, 0.0) : uCameraUp;
    vec3 worldPos = aPositionSize.xyz + uCameraRight * c.x + up * c.y;

    vCorner = aCorner;
    vAlpha = aParams.z;
    vShape = shape;
    vWorldPos = worldPos;
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
