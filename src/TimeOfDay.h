#pragma once

#include <cmath>
#include <string>

struct TimeSettings {
    float dayLengthSeconds = 720.0f;
    float startTime        = 0.65f;
    float maxElevation     = 62.0f;
    float maxSpeed         = 64.0f;
};

class TimeOfDay {
public:
    explicit TimeOfDay(const TimeSettings& settings = {});

    TimeSettings settings;

    void update(float deltaTime);

    float time() const { return m_time; }
    void  setTime(float t) { m_time = t - std::floor(t); }
    float sunAzimuth() const;
    float sunElevation() const;

    bool  isPaused() const { return m_paused; }
    void  setPaused(bool paused) { m_paused = paused; }
    float speed() const { return m_speed; }
    void  faster() { m_speed = m_speed * 2.0f > settings.maxSpeed ? settings.maxSpeed : m_speed * 2.0f; }
    void  slower() { m_speed = m_speed * 0.5f < 1.0f ? 1.0f : m_speed * 0.5f; }

    void syncToSun(float azimuthDegrees);

    std::string clockText() const;

private:
    float m_time;
    float m_speed = 1.0f;
    bool  m_paused = false;
};
