#ifndef LAB_M1_TEMA1_CONFIG_LEADERS_H_
#define LAB_M1_TEMA1_CONFIG_LEADERS_H_

#include "lab_m1/Tema1/config/cards.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/geometry.h"

namespace tema1 {
namespace ui {
namespace leader_panel {
constexpr float kSymbolX = 24;
constexpr float kSymbolY = 23;
constexpr float kHoverSize = 44;
constexpr float kActiveSize = 34;
constexpr float kNormalSize = 30;
constexpr float kTextX = 50;
constexpr float kOwnerTop = 42;
constexpr float kOwnerSize = 10;
constexpr float kNameTop = 21;
constexpr float kNameSize = 10;
constexpr float kNameWidth = 175;
constexpr float kCrystalX = 240;
constexpr float kCrystalStride = 20;
constexpr float kCrystalWidth = 10;
constexpr float kCrystalHeight = 13;
}  // namespace leader_panel

namespace leader_preview {
const float kCenterX = card::preview::kName.x;
constexpr float kSymbolY = 480;
constexpr float kSymbolSize = 90;
const Rect kNameBounds(1380, 380, 180, 30);
constexpr float kNameSize = 12;
const TextPlacement kAvailability = {kCenterX, 360, 9};
const Rect kDescriptionBounds(kNameBounds.x, 200, kNameBounds.width, 130);
constexpr float kDescriptionSize = 10;
}  // namespace leader_preview

namespace leader_menu {
const TextPlacement kTitle = {kCanvasWidth / 2, 792, 42};
const TextPlacement kSubtitle = {kTitle.x, 716, 23};
const TextPlacement kInstruction = {kTitle.x, 675, 12};
constexpr float kBorder = 1;
constexpr float kSymbolTop = 211;
constexpr float kSymbolSize = 105;
constexpr float kSelectedScale = 1.15f;
constexpr float kNameTop = 132;
constexpr float kNameSize = 16;
constexpr float kNameSidePadding = 14;
constexpr float kDescriptionSidePadding = 20;
constexpr float kDescriptionBottom = 30;
constexpr float kDescriptionHeight = 58;
constexpr float kDescriptionSize = 10;
}  // namespace leader_menu

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_LEADERS_H_
