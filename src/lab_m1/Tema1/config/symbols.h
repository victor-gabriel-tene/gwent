#ifndef LAB_M1_TEMA1_CONFIG_SYMBOLS_H_
#define LAB_M1_TEMA1_CONFIG_SYMBOLS_H_

#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/geometry.h"

namespace tema1 {
namespace ui {
namespace symbol {
constexpr float kQuarterTurn = kPi / 2;
constexpr float kDiagonal = kPi / 4;

namespace heal {
constexpr float kDiamondScale = 0.70f;
}

namespace shield {
constexpr float kInsetLift = 1;
constexpr float kInsetScale = 0.65f;
constexpr float kStemWidth = 0.12f;
constexpr float kStemHeight = 0.48f;
}  // namespace shield

namespace spy {
constexpr float kEyeOffset = 0.18f;
constexpr float kEyeSize = 0.48f;
constexpr float kPupilSize = 0.26f;
}  // namespace spy

namespace frost {
constexpr int kAxes = 3;
constexpr float kStroke = 0.065f;
constexpr float kCenterSize = 0.26f;
}  // namespace frost

namespace clear {
constexpr int kRays = 8;
constexpr float kCenterSize = 0.55f;
constexpr float kRayRadius = 0.42f;
constexpr float kRayWidth = 0.14f;
constexpr float kRayHeight = 0.20f;
}  // namespace clear

namespace fog {
constexpr int kRowsEachSide = 1;
constexpr float kCenterHalfWidth = 0.48f;
constexpr float kOuterHalfWidth = 0.36f;
constexpr float kRowStep = 0.25f;
constexpr float kStroke = 0.10f;
}  // namespace fog

namespace rain {
constexpr float kLeftCloudX = 0.17f;
constexpr float kLeftCloudY = 0.17f;
constexpr float kLeftCloudWidth = 0.42f;
constexpr float kLeftCloudHeight = 0.32f;
constexpr float kRightCloudX = 0.12f;
constexpr float kRightCloudY = 0.21f;
constexpr float kRightCloudWidth = 0.55f;
constexpr float kRightCloudHeight = 0.42f;
constexpr int kDropsEachSide = 1;
constexpr float kDropStep = 0.25f;
constexpr float kDropTop = 0.12f;
constexpr float kDropSlant = 0.12f;
constexpr float kDropBottom = 0.43f;
constexpr float kStroke = 0.065f;
}  // namespace rain

namespace horn {
constexpr float kBellX = 0.12f;
constexpr float kBellWidth = 0.65f;
constexpr float kBellHeight = 0.8f;
constexpr float kTubeLeft = 0.43f;
constexpr float kTubeBottom = 0.10f;
constexpr float kTubeWidth = 0.40f;
constexpr float kTubeHeight = 0.20f;
constexpr float kRimX = 0.48f;
constexpr float kRimHalfHeight = 0.29f;
constexpr float kRimStroke = 0.08f;
}  // namespace horn

namespace scorch {
constexpr float kCenterWidth = 0.75f;
constexpr float kSideOffset = 0.27f;
constexpr float kSideDrop = 0.13f;
constexpr float kSideWidth = 0.40f;
constexpr float kSideHeight = 0.58f;
constexpr float kSideAngle = 0.3f;
constexpr float kInsetDrop = 0.14f;
constexpr float kInsetWidth = 0.25f;
constexpr float kInsetHeight = 0.45f;
}  // namespace scorch

namespace medic {
constexpr float kHalfThickness = 0.14f;
constexpr float kHalfLength = 0.48f;
constexpr float kThickness = 2 * kHalfThickness;
constexpr float kLength = 2 * kHalfLength;
}  // namespace medic

namespace decoy {
constexpr float kHeadY = 0.28f;
constexpr float kHeadSize = 0.31f;
constexpr float kNeckY = 0.08f;
constexpr float kFootY = 0.47f;
constexpr float kStroke = 0.09f;
constexpr float kArmHalfWidth = 0.43f;
constexpr float kArmDrop = 0.03f;
constexpr float kBaseDrop = 0.20f;
constexpr float kBaseWidth = 0.62f;
constexpr float kBaseHeight = 0.50f;
}  // namespace decoy

namespace leader {
constexpr float kDiamondScale = 0.73f;
constexpr int kHexagonSides = 6;
constexpr float kHexagonRadius = 0.53f;
constexpr float kHexagonApothemRatio = 0.8660254f;
constexpr float kTriangleWidth = 1.04f;
constexpr int kStarPoints = 5;
constexpr float kStarRadius = 0.57f;
constexpr float kStarInnerRatio = 0.44f;
}  // namespace leader
}  // namespace symbol

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_SYMBOLS_H_
