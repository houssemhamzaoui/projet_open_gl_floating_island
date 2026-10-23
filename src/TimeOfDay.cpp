#include "TimeOfDay.h"

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <cstdio>

TimeOfDay::TimeOfDay(const TimeSettings& settings)
    : settings(settings), m_time(settings.startTime)
{
}

void TimeOfDay::update(float deltaTime)
{
    if (m_paused)
        return;
    m_time += deltaTime * m_speed / settings.dayLengthSeconds;
    m_time -= std::floor(m_time);
}

float TimeOfDay::sunAzimuth() const
{
    return 360.0f * m_time - 90.0f;
}

float TimeOfDay::sunElevation() const
{
    return settings.maxElevation * std::sin(glm::two_pi<float>() * (m_time - 0.25f));
}

void TimeOfDay::syncToSun(float azimuthDegrees)
{
    m_time = (azimuthDegrees + 90.0f) / 360.0f;
    m_time -= std::floor(m_time);
}

std::string TimeOfDay::clockText() const
{
    const int minutes = static_cast<int>(m_time * 24.0f * 60.0f) % (24 * 60);
    char text[8];
    std::snprintf(text, sizeof(text), "%02d:%02d", minutes / 60, minutes % 60);
    return text;
}
