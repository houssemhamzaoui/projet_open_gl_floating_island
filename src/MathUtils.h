#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>

inline float shortestAngle(float from, float to)
{
    float difference = std::fmod(to - from, glm::two_pi<float>());
    if (difference >  glm::pi<float>()) difference -= glm::two_pi<float>();
    if (difference < -glm::pi<float>()) difference += glm::two_pi<float>();
    return difference;
}

inline float wrapPeriodic(float x, float period)
{
    float wrapped = x - std::floor(x / period) * period;
    if (wrapped >= period)
        wrapped = 0.0f;
    return wrapped;
}

inline glm::vec3 wrapPeriodic(const glm::vec3& v, const glm::vec3& period)
{
    return {wrapPeriodic(v.x, period.x), wrapPeriodic(v.y, period.y), wrapPeriodic(v.z, period.z)};
}
