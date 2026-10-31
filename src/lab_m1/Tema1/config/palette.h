#ifndef LAB_M1_TEMA1_CONFIG_PALETTE_H_
#define LAB_M1_TEMA1_CONFIG_PALETTE_H_

#include <array>

#include "lab_m1/Tema1/game_types.h"
#include "utils/glm_utils.h"

namespace tema1 {
namespace ui {

extern const glm::vec3 kClearColor;
extern const glm::vec3 kBrass;
extern const glm::vec3 kGold;
extern const glm::vec3 kIvory;
extern const glm::vec3 kMuted;
extern const glm::vec3 kDark;
extern const glm::vec3 kBlue;
extern const glm::vec3 kTableBackground;
extern const glm::vec3 kFriendlyLane;
extern const glm::vec3 kEnemyLane;
extern const glm::vec3 kHoveredLane;
extern const glm::vec3 kLegalLane;
extern const glm::vec3 kButtonHover;
extern const glm::vec3 kInsetBackground;
extern const glm::vec3 kHealth;
extern const glm::vec3 kEmptyHealth;
extern const glm::vec3 kEmptyShield;
extern const glm::vec3 kLeaderHover;
extern const glm::vec3 kSpentLeader;
extern const glm::vec3 kEmptyCrystal;
extern const glm::vec3 kSelectedPanel;
extern const glm::vec3 kPanelBackground;
extern const glm::vec3 kBoostedValue;
extern const std::array<glm::vec3, kCardTypeCount> kCardColors;
extern const std::array<glm::vec3, static_cast<int>(Leader::kCount)>
    kLeaderColors;

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_PALETTE_H_
