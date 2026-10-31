#ifndef LAB_M1_TEMA1_CONFIG_BOARD_H_
#define LAB_M1_TEMA1_CONFIG_BOARD_H_

#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/geometry.h"

namespace tema1 {
namespace ui {
namespace board {
constexpr float kFrameSideInset = 16;
constexpr float kFrameBottomInset = 12;
constexpr float kFrameVerticalExpansion = 2 * kFrameBottomInset;
constexpr float kCenterY = kCanvasHeight / 2;
constexpr float kBorder = 1;
constexpr float kDropBorder = 2;
constexpr float kTargetPadding = 3;
constexpr float kTargetBorder = 2;
constexpr float kLaneLabelLift = 4;
constexpr float kLaneLabelSize = 9;
constexpr float kRulesLabelLift = 5.5f;
constexpr float kRulesLabelSize = 11;
}  // namespace board

namespace status {
constexpr float kLabelTop = 42;
constexpr float kTextSize = 10;
constexpr float kPipWidth = 6;
constexpr float kPipHeight = 13;
constexpr float kPipStride = 8;
constexpr float kHealthCenterX = kCanvasWidth / 2;
constexpr float kHealthLeft =
    kHealthCenterX - ((kStartingHp - 1) * kPipStride + kPipWidth) / 2;
constexpr float kHealthBottom = 8;
constexpr float kShieldCenterX = 1189;
constexpr float kShieldLeft =
    kShieldCenterX - ((kShieldColumns - 1) * kPipStride + kPipWidth) / 2;
constexpr float kShieldTop = 21;
constexpr float kShieldMaxRowStep = 15;
constexpr float kShieldSpace = 22;
constexpr float kShieldFill = 0.85f;
}  // namespace status

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_BOARD_H_
