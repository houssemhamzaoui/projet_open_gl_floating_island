#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec4 aParams;

uniform mat4 uProjection;

out vec2 vUV;
out vec4 vColor;
out vec4 vParams;

void main()
{
    vUV = aUV;
    vColor = aColor;
    vParams = aParams;
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
}
