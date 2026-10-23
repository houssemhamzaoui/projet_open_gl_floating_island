#version 330 core

in vec3 vDirection;
out vec4 FragColor;

uniform float uTime;

uniform vec3  uSunDirection;
uniform vec3  uSunDiscColor;
uniform float uSunCos;
uniform float uSunEdgeCos;
uniform float uGlowStrength;
uniform float uGlowSharpness;

uniform vec3  uZenith;
uniform vec3  uHorizon;
uniform vec3  uSunsetGlow;
uniform float uSunsetAmount;
uniform float uNightAmount;

uniform vec3  uMoonDirection;
uniform vec3  uMoonColor;
uniform float uMoonCos;
uniform float uMoonEdgeCos;
uniform float uStarAmount;
uniform float uStarDensity;
uniform float uDayAngle;

uniform bool  uCloudsEnabled;
uniform float uCloudCoverage;
uniform float uCloudSoftness;
uniform float uCloudScale;
uniform float uCloudSpeed;
uniform vec3  uCloudLight;
uniform vec3  uCloudShadow;
uniform float uCloudSeaScale;

float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float valueNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 f    = fract(p);
    vec2 u    = f * f * (3.0 - 2.0 * f);

    float a = hash(cell);
    float b = hash(cell + vec2(1.0, 0.0));
    float c = hash(cell + vec2(0.0, 1.0));
    float d = hash(cell + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p)
{
    float sum = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 5; ++i) {
        sum += amplitude * valueNoise(p);
        p = p * 2.03 + vec2(17.1, 9.7);
        amplitude *= 0.5;
    }
    return sum;
}

float cloudMask(vec2 uv)
{
    float threshold = 1.0 - uCloudCoverage;
    return smoothstep(threshold - uCloudSoftness, threshold + uCloudSoftness, fbm(uv));
}

void cloudColors(float cosSun, out vec3 lit, out vec3 shade)
{
    float brightness = mix(1.0, 0.18, uNightAmount);
    float warm = uSunsetAmount * (0.35 + 0.65 * pow(max(cosSun, 0.0), 3.0));
    lit   = mix(uCloudLight,  uCloudLight  * uSunsetGlow, warm)        * brightness;
    shade = mix(uCloudShadow, uCloudShadow * uSunsetGlow, warm * 0.5)  * brightness;
}

float hash3(vec3 p)
{
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.x + p.y) * p.z);
}

vec3 stars(vec3 dir)
{
    float a = uDayAngle;
    vec3 axis = normalize(vec3(0.3, 0.9, 0.2));
    vec3 d = dir * cos(a) + cross(axis, dir) * sin(a) + axis * dot(axis, dir) * (1.0 - cos(a));

    vec3 p = d * 140.0;
    vec3 cell = floor(p);
    float h = hash3(cell);
    if (h > uStarDensity)
        return vec3(0.0);
    vec3 spot = vec3(hash3(cell + 1.7), hash3(cell + 3.1), hash3(cell + 5.9)) * 0.6 + 0.2;
    float dist = length(fract(p) - spot);
    float size = 0.12 + 0.18 * hash3(cell + 9.3);
    float twinkle = 0.65 + 0.35 * sin(uTime * (1.5 + 3.0 * hash3(cell + 4.2)) + h * 100.0);
    float brightness = smoothstep(size, 0.0, dist) * twinkle * (0.5 + 0.5 * hash3(cell + 7.7));
    vec3 tint = mix(vec3(0.8, 0.88, 1.0), vec3(1.0, 0.92, 0.75), hash3(cell + 2.4));
    return tint * brightness * 1.6;
}

void main()
{
    vec3 dir = normalize(vDirection);
    float cosSun = dot(dir, uSunDirection);

    float up = clamp(dir.y, 0.0, 1.0);
    vec3 color = mix(uHorizon, uZenith, 1.0 - pow(1.0 - up, 3.0));

    float horizonBand = 1.0 - smoothstep(0.0, 0.35, abs(dir.y));
    float sunSide = pow(cosSun * 0.5 + 0.5, 4.0);
    color = mix(color, uSunsetGlow, uSunsetAmount * horizonBand * sunSide);

    if (uStarAmount > 0.01 && dir.y > 0.0)
        color += stars(dir) * uStarAmount * smoothstep(0.0, 0.15, dir.y);
    float cosMoon = dot(dir, uMoonDirection);
    float moonVisible = max(uStarAmount, 0.25 * uNightAmount);
    color += uMoonColor * pow(max(cosMoon, 0.0), 300.0) * 0.5 * moonVisible;
    float moonDisc = smoothstep(uMoonEdgeCos, uMoonCos, cosMoon) * smoothstep(-0.01, 0.01, dir.y);
    float seas = 0.82 + 0.18 * valueNoise(dir.xz * 90.0 + dir.y * 40.0);
    color = mix(color, uMoonColor * seas, moonDisc * smoothstep(0.0, 0.3, moonVisible + 0.15));

    color += uSunDiscColor * pow(max(cosSun, 0.0), uGlowSharpness) * uGlowStrength;
    float disc = smoothstep(uSunEdgeCos, uSunCos, cosSun) * smoothstep(-0.01, 0.01, dir.y);
    color = mix(color, mix(uSunDiscColor, vec3(1.0), 0.3), disc);

    vec3 cloudLit, cloudShade;
    cloudColors(cosSun, cloudLit, cloudShade);

    if (uCloudsEnabled && dir.y > 0.0) {
        vec2 uv = dir.xz / (dir.y + 0.2) * uCloudScale;
        uv += uTime * uCloudSpeed * vec2(1.0, 0.3);

        float cloud = cloudMask(uv);

        float awayEdge = 1.0 - cloudMask(uv - uSunDirection.xz * 0.15);
        vec3 cloudColor = mix(cloudLit, cloudShade, awayEdge);

        cloud *= smoothstep(0.0, 0.15, dir.y);
        color = mix(color, cloudColor, cloud);
    }

    if (dir.y < 0.0) {
        vec2 uv = dir.xz / (-dir.y + 0.03) * uCloudSeaScale;
        uv += uTime * uCloudSpeed * 0.5 * vec2(1.0, 0.3);

        float puffs = smoothstep(0.47, 0.53, fbm(uv));
        vec3 sea = mix(mix(cloudShade, uHorizon, 0.3), cloudLit, puffs);

        float haze = 1.0 - smoothstep(0.0, 0.12, -dir.y);
        color = mix(sea, color, haze);
    }

    FragColor = vec4(color, 1.0);
}
