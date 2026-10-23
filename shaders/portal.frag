#version 330 core

in vec2 vLocal;
out vec4 FragColor;

uniform vec3  uColor;
uniform float uTime;
uniform float uSpeed;

float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float valueNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(cell), hash(cell + vec2(1.0, 0.0)), u.x),
               mix(hash(cell + vec2(0.0, 1.0)), hash(cell + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p)
{
    float sum = 0.0, amplitude = 0.5;
    for (int i = 0; i < 3; ++i) {
        sum += amplitude * valueNoise(p);
        p = p * 2.1 + vec2(13.7, 5.3);
        amplitude *= 0.5;
    }
    return sum;
}

void main()
{
    float r = length(vLocal);
    if (r > 1.0)
        discard;
    float angle = atan(vLocal.y, vLocal.x);

    float twisted = angle + (1.0 - r) * 3.0 + uTime * uSpeed;

    vec2 p = vec2(cos(twisted), sin(twisted)) * r;
    float clouds = fbm(p * 3.0 + vec2(0.0, uTime * uSpeed * 0.3));

    float arms = 0.5 + 0.5 * sin(angle * 3.0 + r * 8.0 - uTime * uSpeed * 3.0);

    float pattern = 0.6 * clouds + 0.4 * arms;

    vec3 dark  = uColor * 0.55;
    vec3 light = mix(uColor, vec3(1.0), 0.55);
    vec3 color = mix(dark, light, smoothstep(0.42, 0.62, pattern));
    color += vec3(1.0) * pow(1.0 - r, 3.0) * 0.6;
    color += uColor * smoothstep(0.7, 0.95, r) * 0.7;

    float alpha = (0.75 + 0.25 * pattern) * (1.0 - smoothstep(0.88, 1.0, r));

    FragColor = vec4(color, alpha);
}
