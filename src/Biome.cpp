#include "Biome.h"

BiomeSettings BiomeSettings::meadow()
{
    BiomeSettings b;
    b.name = "Meadow";
    b.kind = BiomeKind::Meadow;
    b.colors = {
        {0.94f, 0.87f, 0.67f},
        {0.40f, 0.70f, 0.38f},
        {0.56f, 0.82f, 0.44f},
        {0.74f, 0.89f, 0.55f},
        {0.64f, 0.59f, 0.55f},
        {0.58f, 0.45f, 0.36f},
        {0.56f, 0.50f, 0.49f},
        {0.40f, 0.36f, 0.40f},
    };
    b.portalColor = {0.45f, 0.95f, 0.55f};
    b.plateauHeight = 2.0f;
    b.bumpHeight = 6.0f;
    b.noiseScale = 0.045f;
    return b;
}

BiomeSettings BiomeSettings::crystal()
{
    BiomeSettings b;
    b.name = "Crystal";
    b.kind = BiomeKind::Crystal;
    b.colors = {
        {0.80f, 0.78f, 0.90f},
        {0.70f, 0.66f, 0.88f},
        {0.82f, 0.79f, 0.96f},
        {0.88f, 0.87f, 0.99f},
        {0.46f, 0.42f, 0.74f},
        {0.42f, 0.34f, 0.56f},
        {0.40f, 0.36f, 0.64f},
        {0.22f, 0.20f, 0.44f},
    };
    b.portalColor = {0.70f, 0.45f, 1.00f};
    b.plateauHeight = 3.0f;
    b.bumpHeight = 0.8f;
    b.noiseScale = 0.05f;
    b.octaves = 4;
    b.falloffStart = 0.75f;
    b.coastNoise = 0.15f;
    b.rockSlope = 0.3f;
    b.undersideRough = 0.45f;
    return b;
}

BiomeSettings BiomeSettings::sakura()
{
    BiomeSettings b;
    b.name = "Sakura";
    b.kind = BiomeKind::Sakura;
    b.colors = {
        {0.98f, 0.88f, 0.80f},
        {0.58f, 0.78f, 0.52f},
        {0.97f, 0.78f, 0.85f},
        {0.99f, 0.87f, 0.91f},
        {0.72f, 0.62f, 0.62f},
        {0.62f, 0.46f, 0.42f},
        {0.62f, 0.52f, 0.55f},
        {0.44f, 0.36f, 0.45f},
    };
    b.portalColor = {1.00f, 0.55f, 0.75f};
    b.plateauHeight = 2.5f;
    b.bumpHeight = 0.7f;
    b.noiseScale = 0.05f;
    b.octaves = 3;
    b.falloffStart = 0.75f;
    b.coastNoise = 0.15f;
    return b;
}

BiomeSettings BiomeSettings::frost()
{
    BiomeSettings b;
    b.name = "Frost";
    b.kind = BiomeKind::Frost;
    b.colors = {
        {0.78f, 0.88f, 0.97f},
        {0.80f, 0.88f, 1.00f},
        {0.90f, 0.95f, 1.00f},
        {0.96f, 0.98f, 1.00f},
        {0.62f, 0.72f, 0.86f},
        {0.55f, 0.62f, 0.74f},
        {0.56f, 0.63f, 0.78f},
        {0.34f, 0.40f, 0.58f},
    };
    b.portalColor = {0.55f, 0.85f, 1.00f};
    b.plateauHeight = 3.5f;
    b.bumpHeight = 0.9f;
    b.noiseScale = 0.05f;
    b.octaves = 4;
    b.falloffStart = 0.72f;
    b.coastNoise = 0.15f;
    b.rockSlope = 0.4f;
    return b;
}

BiomeSettings BiomeSettings::rocky()
{
    BiomeSettings b;
    b.name = "Rocky Islet";
    b.kind = BiomeKind::Rocky;
    b.colors = {
        {0.82f, 0.77f, 0.70f},
        {0.56f, 0.72f, 0.48f},
        {0.66f, 0.78f, 0.52f},
        {0.70f, 0.70f, 0.66f},
        {0.60f, 0.56f, 0.54f},
        {0.52f, 0.44f, 0.38f},
        {0.54f, 0.50f, 0.50f},
        {0.38f, 0.35f, 0.40f},
    };
    b.portalColor = {0.62f, 0.78f, 0.60f};
    b.plateauHeight = 2.0f;
    b.bumpHeight = 0.8f;
    b.noiseScale = 0.12f;
    b.octaves = 3;
    b.falloffStart = 0.7f;
    b.coastNoise = 0.2f;
    b.undersideDepthRatio = 1.3f;
    return b;
}

BiomeSettings BiomeSettings::skyRock()
{
    BiomeSettings b = rocky();
    b.name = "Sky Rock";
    b.kind = BiomeKind::SkyRock;
    b.plateauHeight = 1.2f;
    b.bumpHeight = 0.3f;
    b.falloffStart = 0.6f;
    b.coastNoise = 0.05f;
    b.undersideDepthRatio = 1.8f;
    return b;
}

BiomeSettings BiomeSettings::cave()
{
    BiomeSettings b;
    b.name = "Cave";
    b.kind = BiomeKind::Cave;
    b.colors = {
        {0.46f, 0.48f, 0.52f},
        {0.24f, 0.36f, 0.28f},
        {0.32f, 0.44f, 0.33f},
        {0.40f, 0.48f, 0.42f},
        {0.36f, 0.40f, 0.50f},
        {0.30f, 0.28f, 0.30f},
        {0.34f, 0.37f, 0.46f},
        {0.18f, 0.20f, 0.28f},
    };
    b.portalColor = {1.00f, 0.62f, 0.22f};
    b.plateauHeight = 3.0f;
    b.bumpHeight = 1.2f;
    b.noiseScale = 0.06f;
    b.octaves = 4;
    b.falloffStart = 0.85f;
    b.coastNoise = 0.08f;
    b.rockSlope = 0.3f;
    b.undersideDepthRatio = 1.3f;
    b.undersideRough = 0.12f;
    return b;
}
