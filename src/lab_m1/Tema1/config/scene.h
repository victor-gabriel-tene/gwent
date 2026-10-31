#ifndef LAB_M1_TEMA1_CONFIG_SCENE_H_
#define LAB_M1_TEMA1_CONFIG_SCENE_H_

#include "lab_m1/Tema1/game_types.h"

namespace tema1 {
namespace ui {

// Base dimensions and margins; dependent geometry is calculated from them.
constexpr float kCanvasWidth = 1600;
constexpr float kCanvasHeight = 900;
constexpr float kPi = 3.14159265359f;

constexpr float kCardWidth = 90;
constexpr float kCardHeight = 126;
constexpr float kCardAspectRatio = kCardHeight / kCardWidth;
constexpr float kContentLeft = 260;
constexpr float kContentWidth = kCanvasWidth - 2 * kContentLeft;
constexpr float kPlayerHandBottom = 24;
constexpr float kHandHeight = kCardHeight;
constexpr float kEnemyHandBottom =
    kCanvasHeight - kPlayerHandBottom - kHandHeight;

constexpr float kLaneHeight = 70;
constexpr float kLaneGap = 4;
constexpr float kLaneStride = kLaneHeight + kLaneGap;
constexpr float kLanePadding = 72;
constexpr float kCenterGap = 12;
constexpr float kPlayerMeleeBottom =
    kCanvasHeight / 2 - kCenterGap / 2 - kLaneHeight;
constexpr float kEnemyMeleeBottom = kCanvasHeight / 2 + kCenterGap / 2;

constexpr float kHandCardGap = 10;
constexpr float kHandCardStride = kCardWidth + kHandCardGap;
constexpr float kBoardCardWidth = 48;
constexpr float kBoardCardGap = 8;
constexpr float kBoardCardStride = kBoardCardWidth + kBoardCardGap;
constexpr float kDeckWidth = 36;
constexpr float kDeckHeight = kDeckWidth * kCardHeight / kCardWidth;
constexpr float kDeckGap = 20;

constexpr float kRulesWidth = 130;
constexpr float kRulesRightInset = 50;
constexpr float kRulesLeft = kCanvasWidth - kRulesWidth - kRulesRightInset;
constexpr float kRulesBottom = 28;
constexpr float kRulesHeight = 36;

constexpr float kPopupButtonWidth = 220;
constexpr float kPopupButtonLeft = (kCanvasWidth - kPopupButtonWidth) / 2;
constexpr float kPopupButtonBottom = 302;
constexpr float kPopupButtonHeight = 44;

constexpr float kLeaderLeft = 270;
constexpr float kLeaderWidth = 280;
constexpr float kLeaderHeight = 46;
constexpr float kHandToLeaderGap = 18;
constexpr float kPlayerLeaderBottom =
    kPlayerHandBottom + kHandHeight + kHandToLeaderGap;
constexpr float kEnemyLeaderBottom =
    kEnemyHandBottom - kLeaderHeight - kHandToLeaderGap;
constexpr float kLeaderChoiceWidth = 250;
constexpr float kLeaderChoiceHeight = 310;
constexpr float kLeaderChoiceGap = 50;
constexpr float kLeaderChoiceStride = kLeaderChoiceWidth + kLeaderChoiceGap;
constexpr float kLeaderChoiceLeft =
    (kCanvasWidth -
     ((static_cast<int>(Leader::kCount) - 1) * kLeaderChoiceStride +
      kLeaderChoiceWidth)) /
    2;
constexpr float kLeaderChoiceBottom = 310;

constexpr unsigned kFontSize = 32;
constexpr unsigned kCircleSegments = 48;
constexpr int kGlyphAdvanceShift = 6;
constexpr int kAsciiLimit = 128;
constexpr int kShieldColumns = 30;
constexpr float kCameraNear = 0.01f;
constexpr float kCameraFar = 400;
constexpr float kCameraDistance = 50;

constexpr int kNoSelection = -1;
constexpr int kSeedNumberBase = 10;
constexpr char kSeedEnvironment[] = "TEMA1_SEED";
constexpr char kVerificationEnvironment[] = "TEMA1_VERIFY_DIR";

constexpr char kWindowTitle[] = "GWENT - The Card Table | EGC Tema 1";
constexpr char kFontFile[] = "Hack-Bold.ttf";
constexpr char kQuadMesh[] = "t1_quad";
constexpr char kTriangleMesh[] = "t1_triangle";
constexpr char kCircleMesh[] = "t1_circle";

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_SCENE_H_
