struct DirectionalLight {
    vec3  direction;
    vec3  color;
    float intensity;
};

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float radius;
};
const int MAX_POINT_LIGHTS = 16;
const int MAX_CAVE_PATH    = 64;

uniform DirectionalLight uSun;
uniform vec3  uCameraPos;
uniform vec3  uSkyAmbient;
uniform vec3  uGroundAmbient;
uniform float uAmbientStrength;
uniform vec3  uFogColor;
uniform float uFogDensity;

uniform int        uPointLightCount;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];

uniform int   uCavePathCount;
uniform vec3  uCavePath[MAX_CAVE_PATH];
uniform vec4  uCaveBounds;
uniform vec3  uCaveChamber;
uniform vec2  uCaveChamberSize;
uniform vec2  uCaveFade;
uniform vec3  uCaveAmbient;

uniform bool  uToon;
uniform int   uToonBands;
uniform float uShadowThreshold;
uniform float uHighlightThreshold;
uniform float uMidBandLevel;
uniform float uBandSoftness;
uniform vec3  uShadowTint;
uniform float uRimWidth;
uniform float uRimSoftness;

uniform float uSpecularStrength;
uniform float uShininess;
uniform float uRimStrength;

vec3 hemisphereAmbient(vec3 N)
{
    float up = N.y * 0.5 + 0.5;
    return mix(uGroundAmbient, uSkyAmbient, up);
}

float softStep(float edge, float value)
{
    return smoothstep(edge - uBandSoftness, edge + uBandSoftness, value);
}

float toonDiffuse(float NdotL)
{
    if (uToonBands >= 3)
        return uMidBandLevel         * softStep(uShadowThreshold, NdotL)
             + (1.0 - uMidBandLevel) * softStep(uHighlightThreshold, NdotL);
    return softStep(uShadowThreshold, NdotL);
}

float rimFactor(vec3 N, vec3 V)
{
    float edge = 1.0 - max(dot(N, V), 0.0);
    return smoothstep(1.0 - uRimWidth - uRimSoftness, 1.0 - uRimWidth + uRimSoftness, edge);
}

float caveDepth(vec3 p)
{
    if (uCavePathCount == 0)
        return 0.0;
    vec3 c = p - uCaveChamber;
    float hr = length(c.xz) / (uCaveChamberSize.x + 1.0);
    float vr = (c.y - uCaveChamberSize.y * 0.5) / (uCaveChamberSize.y * 0.5 + 1.0);
    if (hr * hr + vr * vr < 1.0)
        return 1.0;
    if (distance(p, uCaveBounds.xyz) > uCaveBounds.w)
        return 0.0;
    float best = 1e9;
    int nearest = 0;
    for (int i = 0; i < uCavePathCount; ++i) {
        float d = distance(p, uCavePath[i]);
        if (d < best) { best = d; nearest = i; }
    }
    if (best > 3.6)
        return 0.0;
    return float(nearest) / float(uCavePathCount - 1);
}

float attenuation(float distanceToLight, float radius)
{
    float x = clamp(1.0 - (distanceToLight * distanceToLight) / (radius * radius), 0.0, 1.0);
    return x * x;
}

vec3 computeLighting(vec3 albedo, vec3 normal, vec3 worldPos)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(uCameraPos - worldPos);

    float sunVisible = 1.0 - smoothstep(uCaveFade.x, uCaveFade.y, caveDepth(worldPos));
    vec3 L = normalize(-uSun.direction);
    vec3 H = normalize(L + V);

    float NdotL = dot(N, L);
    float NdotH = max(dot(N, H), 0.0);

    vec3 sunLight = uSun.color * uSun.intensity * sunVisible;
    vec3 ambient = mix(uCaveAmbient, hemisphereAmbient(N), sunVisible);

    float specular = pow(NdotH, uShininess);

    vec3 color;
    if (!uToon) {
        vec3 diffuse = sunLight * max(NdotL, 0.0);
        if (NdotL <= 0.0) specular = 0.0;
        color = albedo * (ambient * uAmbientStrength + diffuse)
              + sunLight * specular * uSpecularStrength;
    } else {
        float light = toonDiffuse(NdotL);

        vec3 litSide    = albedo * uSun.color * uSun.intensity;
        vec3 shadowSide = albedo * ambient * uShadowTint;
        color = mix(shadowSide, litSide, light * sunVisible);

        color += sunLight * softStep(0.5, specular) * uSpecularStrength * light;

        color += sunLight * rimFactor(N, V) * uRimStrength * light;
    }

    for (int i = 0; i < uPointLightCount; ++i) {
        vec3  toLight = uPointLights[i].position - worldPos;
        float d = length(toLight);
        if (d >= uPointLights[i].radius)
            continue;
        vec3  Lp = toLight / d;
        float NdotLp = dot(N, Lp);
        float att = attenuation(d, uPointLights[i].radius);
        vec3  lightColor = uPointLights[i].color * uPointLights[i].intensity;
        float spec = pow(max(dot(N, normalize(Lp + V)), 0.0), uShininess);

        if (!uToon) {
            color += albedo * lightColor * max(NdotLp, 0.0) * att
                   + lightColor * spec * uSpecularStrength * att * step(0.0, NdotLp);
        } else {
            float rings = 0.3 * softStep(0.12, att) + 0.7 * softStep(0.5, att);
            float lit = toonDiffuse(NdotLp) * rings;
            color += albedo * lightColor * lit;
            color += lightColor * softStep(0.5, spec) * uSpecularStrength * lit;
            color += lightColor * rimFactor(N, V) * uRimStrength * 0.5 * rings
                   * smoothstep(-0.2, 0.3, NdotLp);
        }
    }
    return color;
}

vec3 applyFog(vec3 color, vec3 worldPos)
{
    float distance = length(worldPos - uCameraPos);
    float fog = 1.0 - exp(-pow(distance * uFogDensity, 2.0));
    return mix(color, uFogColor, fog);
}
