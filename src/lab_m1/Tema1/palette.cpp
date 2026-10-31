#include "lab_m1/Tema1/config/palette.h"

namespace tema1 {
namespace ui {

const glm::vec3 kClearColor(0.025f, 0.021f, 0.018f);

const glm::vec3 kBrass(0.60f, 0.46f, 0.26f);
const glm::vec3 kGold(0.88f, 0.70f, 0.37f);
const glm::vec3 kIvory(0.91f, 0.86f, 0.74f);
const glm::vec3 kMuted(0.60f, 0.55f, 0.44f);
const glm::vec3 kDark(0.075f, 0.056f, 0.043f);
const glm::vec3 kBlue(0.31f, 0.59f, 0.76f);
const glm::vec3 kTableBackground(0.13f, 0.105f, 0.08f);
const glm::vec3 kFriendlyLane(0.21f, 0.185f, 0.145f);
const glm::vec3 kEnemyLane(0.18f, 0.16f, 0.13f);
const glm::vec3 kHoveredLane(0.34f, 0.28f, 0.18f);
const glm::vec3 kLegalLane(0.27f, 0.235f, 0.16f);
const glm::vec3 kButtonHover(0.23f, 0.19f, 0.13f);
const glm::vec3 kInsetBackground(0.17f, 0.14f, 0.10f);
const glm::vec3 kHealth(0.65f, 0.24f, 0.18f);
const glm::vec3 kEmptyHealth(0.18f, 0.14f, 0.11f);
const glm::vec3 kEmptyShield(0.12f, 0.18f, 0.21f);
const glm::vec3 kLeaderHover(0.30f, 0.25f, 0.16f);
const glm::vec3 kSpentLeader(0.42f);
const glm::vec3 kEmptyCrystal(0.23f, 0.21f, 0.18f);
const glm::vec3 kSelectedPanel(0.18f, 0.155f, 0.115f);
const glm::vec3 kPanelBackground(0.13f, 0.115f, 0.09f);
const glm::vec3 kBoostedValue(0.55f, 0.86f, 0.47f);

const std::array<glm::vec3, kCardTypeCount> kCardColors = {{
    {0.57f, 0.13f, 0.10f},
    {0.17f, 0.39f, 0.24f},
    {0.15f, 0.32f, 0.51f},
    {0.075f, 0.075f, 0.085f},
    {0.80f, 0.83f, 0.81f},
    {0.76f, 0.60f, 0.20f},
    {0.43f, 0.48f, 0.50f},
    {0.23f, 0.36f, 0.49f},
    {0.71f, 0.29f, 0.10f},
    {0.41f, 0.24f, 0.46f},
    {0.22f, 0.46f, 0.35f},
    {0.45f, 0.32f, 0.18f},
}};

const std::array<glm::vec3, static_cast<int>(Leader::kCount)> kLeaderColors = {{
    {0.73f, 0.56f, 0.26f},
    {0.70f, 0.64f, 0.41f},
    {0.50f, 0.65f, 0.72f},
    {0.46f, 0.67f, 0.42f},
    {0.72f, 0.45f, 0.25f},
}};

}  // namespace ui
}  // namespace tema1
