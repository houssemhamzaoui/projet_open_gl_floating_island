#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
layout(location = 3) in mat4 aInstanceModel;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
out vec3 vNormal;
out vec3 vWorldPos;

void main()
{
    vec4 worldPos = aInstanceModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vColor    = aColor;
    vNormal   = transpose(inverse(mat3(aInstanceModel))) * aNormal;
    gl_Position = uProjection * uView * worldPos;
}
