#ifndef LAB_M1_TEMA1_CONFIG_OVERLAYS_H_
#define LAB_M1_TEMA1_CONFIG_OVERLAYS_H_

#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/geometry.h"

namespace tema1 {
namespace ui {
namespace rules {
const Rect kPanel(190, 75, 1220, 750);
constexpr float kBorder = 1;
const TextPlacement kTitle = {kCanvasWidth / 2, 791, 28};
const TextPlacement kInstruction = {kTitle.x, 749, 12};
constexpr int kCardColumns = 2;
constexpr float kCardsLeft = 219;
constexpr float kCardsTop = 704;
constexpr float kColumnStride = 595;
constexpr float kRowStride = 56;
constexpr float kSymbolX = 15;
constexpr float kSymbolDrop = 10;
constexpr float kSymbolSize = 24;
constexpr float kTextX = 40;
constexpr float kNameLift = 1;
constexpr float kNameSize = 12;
constexpr float kDescriptionDrop = 18;
constexpr float kDescriptionSize = 10;
constexpr float kDescriptionWidth = 505;
constexpr float kDescriptionLineHeight = 15;
constexpr float kDividerLeft = 225;
constexpr float kDividerRight = kCanvasWidth - kDividerLeft;
constexpr float kDividerY = 374;
const TextPlacement kOrderHeading = {kTitle.x, 350, 10};
const TextPlacement kOrder = {kTitle.x, 329, 11};
constexpr float kNotesLeft = 245;
constexpr float kNotesColumnStride = 400;
constexpr float kNotesHeadingTop = 279;
constexpr float kNotesHeadingSize = 12;
constexpr float kNotesTop = 252;
constexpr float kNotesRowStride = 19;
constexpr float kNotesSize = 11;
const TextPlacement kFooter = {kTitle.x, 113, 11};
}  // namespace rules

namespace popup {
const Rect kPanel(510, 276, 580, 348);
constexpr float kCenterX = kCanvasWidth / 2;
constexpr float kBorder = 1;
const TextPlacement kTitle = {kCenterX, 594, 28};
constexpr float kDividerInset = 32;
constexpr float kDividerY = 552;
constexpr float kDividerIntensity = 0.55f;
constexpr float kButtonTextLift = 7.5f;
constexpr float kButtonTextSize = 15;
constexpr float kColumnOffset = 140;
constexpr float kOwnerTop = 518;
constexpr float kOwnerSize = 11;
constexpr float kHpTop = 482;
constexpr float kHpSize = 24;
constexpr float kRoundCrystalsTop = 424;
constexpr float kRoundCrystalsSize = 24;
constexpr float kMatchCrystalsTop = 474;
constexpr float kMatchCrystalsSize = 28;
const TextPlacement kHpLabel = {kCenterX, 475, 11};
const TextPlacement kRoundCrystalsLabel = {kCenterX, 417, 11};
const TextPlacement kMatchCrystalsLabel = {kCenterX, 465.5f, 11};
const TextPlacement kDrawLabel = {kCenterX, 408, 11};
constexpr float kInspectionStride = 156;
constexpr float kInspectionWidth = 126;
constexpr float kInspectionHeight = kInspectionWidth * kCardHeight / kCardWidth;
constexpr float kInspectionBottom = 364;
}  // namespace popup

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_OVERLAYS_H_
