#pragma once

#include "Collectibles.h"
#include "UIRenderer.h"

#include <glm/glm.hpp>

#include <string>

namespace Hud {

void drawHearts(UIRenderer& ui, int hearts, int maxHearts, float hurtFlash, float time);

void drawClock(UIRenderer& ui, const std::string& clock, bool night, bool paused, float speed, float time,
               int weather = 0);

void drawGemCounters(UIRenderer& ui, const GemCounts& counts);

void drawPortalHint(UIRenderer& ui, const std::string& destination);

void drawTalkPrompt(UIRenderer& ui, const glm::vec2& screen, float time);

void drawQuestMarker(UIRenderer& ui, const glm::vec2& screen, bool ready, float distance, float time);

void drawDemoTag(UIRenderer& ui, const std::string& message, float messageAlpha);

void drawSaveNotice(UIRenderer& ui, float alpha);

}
