#ifndef LAB_M1_TEMA1_CONFIG_CARDS_H_
#define LAB_M1_TEMA1_CONFIG_CARDS_H_

#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/geometry.h"

namespace tema1 {
namespace ui {
namespace card {
// Coordinates within the card's own logical rectangle.
const Rect kFace(0, 0, kCardWidth, kCardHeight);
constexpr float kSelectionPadding = 3;
constexpr float kSelectionBorder = 2;
constexpr float kSymbolX = 49;
constexpr float kSymbolY = kCardHeight / 2;
constexpr float kSymbolSize = 38;

constexpr int kSingleColumnLimit = 6;
constexpr int kValueColumns = 2;
constexpr float kValueLeft = 10;
constexpr float kValueTop = 104;
constexpr float kValueColumnStride = 8;
constexpr float kValueWidth = 6;
constexpr float kValueMaxHeight = 7;
constexpr float kValueMaxStep = 10;
constexpr float kValueSpace = 52;
constexpr float kValueFill = 0.75f;

constexpr TextPlacement kLaneLabel = {75, 115, 12};
constexpr float kCategoryX = kLaneLabel.x;
constexpr float kCategoryY = 15;
constexpr float kCategorySize = 8;
const Rect kSuppressionBounds(34, 47, 31, 33);
constexpr float kSuppressionStroke = 2;

namespace preview {
constexpr float kWidth = 144;
constexpr float kHeight = kWidth * kCardHeight / kCardWidth;
const Rect kBounds(1400, 350, kWidth, kHeight);
const TextPlacement kName = {kBounds.CenterX(), 332, 13};
const TextPlacement kCategory = {kName.x, 309, 9};
const Rect kValueBounds(1380, 250, 180, 39);
constexpr float kValueSize = 9;
const Rect kDescriptionBounds(kValueBounds.x, 100, kValueBounds.width, 133);
constexpr float kDescriptionSize = 10;
}  // namespace preview
}  // namespace card

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_CARDS_H_
