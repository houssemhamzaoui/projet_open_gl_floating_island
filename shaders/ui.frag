#version 330 core

in vec2 vUV;
in vec4 vColor;
in vec4 vParams;
out vec4 FragColor;

uniform sampler2D uAtlas;

float dot2(vec2 v) { return dot(v, v); }

float heartDistance(vec2 p)
{
    p.x = abs(p.x);
    if (p.y + p.x > 1.0)
        return sqrt(dot2(p - vec2(0.25, 0.75))) - sqrt(2.0) / 4.0;
    return sqrt(min(dot2(p - vec2(0.0, 1.0)), dot2(p - 0.5 * max(p.x + p.y, 0.0)))) * sign(p.x - p.y);
}

void main()
{
    vec3 color = vColor.rgb;
    float coverage = 1.0;

    if (vParams.w < -0.5) {
        float size = 2.0 * vParams.x;
        vec2 p = vec2(vUV.x, -vUV.y) / size * 1.15 + vec2(0.0, 0.5);
        float d = heartDistance(p) * size / 1.15;
        coverage = clamp(0.5 - d, 0.0, 1.0);
        color = mix(color, vec3(1.0), 0.6 * smoothstep(0.13, 0.05, length(p - vec2(-0.22, 0.72))));
    } else if (vParams.w < 0.5 || vParams.w > 2.5) {
        vec2 q = abs(vUV) - vParams.xy + vParams.z;
        float d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - vParams.z;
        coverage = clamp(0.5 - d, 0.0, 1.0);
        if (vParams.w > 2.5)
            coverage *= clamp(0.5 + d + (vParams.w - 3.0), 0.0, 1.0);
    } else if (vParams.w < 1.5) {
        coverage = texture(uAtlas, vUV).r;
    } else {
        vec2 p = vUV / vParams.xy;
        float d = (abs(p.x) + abs(p.y) - 1.0) * min(vParams.x, vParams.y) * 0.75;
        coverage = clamp(0.5 - d, 0.0, 1.0);
        color *= (p.x < 0.0 ? 1.15 : 0.80) + (p.y < -0.25 ? 0.15 : 0.0);
        color = mix(color, vec3(1.0), smoothstep(0.22, 0.12, length(p - vec2(-0.25, -0.35))));
    }
    FragColor = vec4(color, vColor.a * coverage);
}
