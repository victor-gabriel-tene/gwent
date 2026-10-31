#include "lab_m1/Tema1/config/symbols.h"

#include <cmath>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/scene.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
namespace glyph = tema1::ui::symbol;
namespace heal = glyph::heal;
namespace shield = glyph::shield;
namespace spy = glyph::spy;
namespace frost = glyph::frost;
namespace clear = glyph::clear;
namespace fog = glyph::fog;
namespace rain = glyph::rain;
namespace horn = glyph::horn;
namespace scorch = glyph::scorch;
namespace medic = glyph::medic;
namespace decoy = glyph::decoy;
using tema1::CardType;
using tema1::Leader;
using ui::kPi;
}  // namespace

void Tema1::DrawCardSymbol(CardType type, float x, float y, float size,
                           glm::vec3 color) {
  switch (type) {
    case CardType::kDamage:
      DrawDamageSymbol(x, y, size, color);
      break;
    case CardType::kHeal:
      DrawHealSymbol(x, y, size, color);
      break;
    case CardType::kShield:
      DrawShieldSymbol(x, y, size, color);
      break;
    case CardType::kSpy:
      DrawSpySymbol(x, y, size, color);
      break;
    case CardType::kFrost:
      DrawFrostSymbol(x, y, size, color);
      break;
    case CardType::kClear:
      DrawClearSymbol(x, y, size, color);
      break;
    case CardType::kFog:
      DrawFogSymbol(x, y, size, color);
      break;
    case CardType::kRain:
      DrawRainSymbol(x, y, size, color);
      break;
    case CardType::kHorn:
      DrawHornSymbol(x, y, size, color);
      break;
    case CardType::kScorch:
      DrawScorchSymbol(x, y, size, color);
      break;
    case CardType::kMedic:
      DrawMedicSymbol(x, y, size, color);
      break;
    case CardType::kDecoy:
      DrawDecoySymbol(x, y, size, color);
      break;
    case CardType::kCount:
      break;
  }
}

void Tema1::DrawDamageSymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kTriangleMesh, x, y, size, size, color);
}

void Tema1::DrawHealSymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kQuadMesh, x, y, size * heal::kDiamondScale,
            size * heal::kDiamondScale, color, glyph::kDiagonal);
}

void Tema1::DrawShieldSymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kCircleMesh, x, y, size, size, color);
  DrawShape(ui::kCircleMesh, x, y + shield::kInsetLift,
            size * shield::kInsetScale, size * shield::kInsetScale,
            CardColor(CardType::kShield));
  DrawShape(ui::kQuadMesh, x, y, size * shield::kStemWidth,
            size * shield::kStemHeight, color);
}

void Tema1::DrawSpySymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kQuadMesh, x - size * spy::kEyeOffset, y, size * spy::kEyeSize,
            size * spy::kEyeSize, color, glyph::kDiagonal);
  DrawShape(ui::kQuadMesh, x + size * spy::kEyeOffset, y, size * spy::kEyeSize,
            size * spy::kEyeSize, color, glyph::kDiagonal);
  DrawShape(ui::kCircleMesh, x, y, size * spy::kPupilSize,
            size * spy::kPupilSize, CardColor(CardType::kSpy));
}

void Tema1::DrawFrostSymbol(float x, float y, float size, glm::vec3 color) {
  for (int i = 0; i < frost::kAxes; ++i) {
    const float angle = i * kPi / frost::kAxes;
    const glm::vec2 direction(std::cos(angle), std::sin(angle));
    DrawLine(x - direction.x * size / 2, y - direction.y * size / 2,
             x + direction.x * size / 2, y + direction.y * size / 2,
             size * frost::kStroke, color);
  }

  DrawShape(ui::kQuadMesh, x, y, size * frost::kCenterSize,
            size * frost::kCenterSize, color, glyph::kDiagonal);
}

void Tema1::DrawClearSymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kCircleMesh, x, y, size * clear::kCenterSize,
            size * clear::kCenterSize, color);

  for (int i = 0; i < clear::kRays; ++i) {
    const float angle = i * kPi / (clear::kRays / 2);
    DrawShape(ui::kTriangleMesh, x + std::cos(angle) * size * clear::kRayRadius,
              y + std::sin(angle) * size * clear::kRayRadius,
              size * clear::kRayWidth, size * clear::kRayHeight, color,
              angle - glyph::kQuarterTurn);
  }
}

void Tema1::DrawFogSymbol(float x, float y, float size, glm::vec3 color) {
  for (int i = -fog::kRowsEachSide; i <= fog::kRowsEachSide; ++i) {
    DrawLine(x - size * (i == 0 ? fog::kCenterHalfWidth : fog::kOuterHalfWidth),
             y + i * size * fog::kRowStep,
             x + size * (i == 0 ? fog::kCenterHalfWidth : fog::kOuterHalfWidth),
             y + i * size * fog::kRowStep, size * fog::kStroke, color);
  }
}

void Tema1::DrawRainSymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kCircleMesh, x - size * rain::kLeftCloudX,
            y + size * rain::kLeftCloudY, size * rain::kLeftCloudWidth,
            size * rain::kLeftCloudHeight, color);
  DrawShape(ui::kCircleMesh, x + size * rain::kRightCloudX,
            y + size * rain::kRightCloudY, size * rain::kRightCloudWidth,
            size * rain::kRightCloudHeight, color);

  for (int i = -rain::kDropsEachSide; i <= rain::kDropsEachSide; ++i) {
    DrawLine(x + i * size * rain::kDropStep, y - size * rain::kDropTop,
             x + i * size * rain::kDropStep - size * rain::kDropSlant,
             y - size * rain::kDropBottom, size * rain::kStroke, color);
  }
}

void Tema1::DrawHornSymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kTriangleMesh, x + size * horn::kBellX, y,
            size * horn::kBellWidth, size * horn::kBellHeight, color,
            glyph::kQuarterTurn);
  DrawBox(Rect(x - size * horn::kTubeLeft, y - size * horn::kTubeBottom,
               size * horn::kTubeWidth, size * horn::kTubeHeight),
          color);
  DrawLine(x + size * horn::kRimX, y - size * horn::kRimHalfHeight,
           x + size * horn::kRimX, y + size * horn::kRimHalfHeight,
           size * horn::kRimStroke, color);
}

void Tema1::DrawScorchSymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kTriangleMesh, x, y, size * scorch::kCenterWidth, size, color);
  DrawShape(ui::kTriangleMesh, x - size * scorch::kSideOffset,
            y - size * scorch::kSideDrop, size * scorch::kSideWidth,
            size * scorch::kSideHeight, color, -scorch::kSideAngle);
  DrawShape(ui::kTriangleMesh, x + size * scorch::kSideOffset,
            y - size * scorch::kSideDrop, size * scorch::kSideWidth,
            size * scorch::kSideHeight, color, scorch::kSideAngle);
  DrawShape(ui::kTriangleMesh, x, y - size * scorch::kInsetDrop,
            size * scorch::kInsetWidth, size * scorch::kInsetHeight,
            CardColor(CardType::kScorch));
}

void Tema1::DrawMedicSymbol(float x, float y, float size, glm::vec3 color) {
  DrawBox(Rect(x - size * medic::kHalfThickness, y - size * medic::kHalfLength,
               size * medic::kThickness, size * medic::kLength),
          color);
  DrawBox(Rect(x - size * medic::kHalfLength, y - size * medic::kHalfThickness,
               size * medic::kLength, size * medic::kThickness),
          color);
}

void Tema1::DrawDecoySymbol(float x, float y, float size, glm::vec3 color) {
  DrawShape(ui::kCircleMesh, x, y + size * decoy::kHeadY,
            size * decoy::kHeadSize, size * decoy::kHeadSize, color);
  DrawLine(x, y + size * decoy::kNeckY, x, y - size * decoy::kFootY,
           size * decoy::kStroke, color);
  DrawLine(x - size * decoy::kArmHalfWidth, y - size * decoy::kArmDrop,
           x + size * decoy::kArmHalfWidth, y - size * decoy::kArmDrop,
           size * decoy::kStroke, color);
  DrawShape(ui::kTriangleMesh, x, y - size * decoy::kBaseDrop,
            size * decoy::kBaseWidth, size * decoy::kBaseHeight, color);
}

void Tema1::DrawLeaderShape(Leader leader, float x, float y, float size,
                            glm::vec3 color) {
  if (leader == Leader::kFoltestClear) {
    DrawShape(ui::kCircleMesh, x, y, size, size, color);
  } else if (leader == Leader::kEmhyr) {
    DrawShape(ui::kQuadMesh, x, y, size * glyph::leader::kDiamondScale,
              size * glyph::leader::kDiamondScale, color, glyph::kDiagonal);
  } else if (leader == Leader::kEredin) {
    const float radius = size * glyph::leader::kHexagonRadius,
                height = radius * glyph::leader::kHexagonApothemRatio;

    for (int i = 0; i < glyph::leader::kHexagonSides; ++i) {
      const float angle = i * kPi / (glyph::leader::kHexagonSides / 2);
      DrawShape(ui::kTriangleMesh, x + std::cos(angle) * height / 2,
                y + std::sin(angle) * height / 2, radius, height, color,
                angle + glyph::kQuarterTurn);
    }
  } else if (leader == Leader::kFrancesca) {
    DrawShape(ui::kTriangleMesh, x, y, size * glyph::leader::kTriangleWidth,
              size, color);
  } else if (leader == Leader::kFoltestSiege) {
    const float radius = size * glyph::leader::kStarRadius,
                inner = radius * glyph::leader::kStarInnerRatio;
    const float base_width =
                    2 * inner * std::sin(kPi / glyph::leader::kStarPoints),
                base_axis = inner * std::cos(kPi / glyph::leader::kStarPoints);

    for (int i = 0; i < glyph::leader::kStarPoints; ++i) {
      const float angle =
          glyph::kQuarterTurn + i * 2 * kPi / glyph::leader::kStarPoints;
      const float center = (radius + base_axis) / 2;
      DrawShape(ui::kTriangleMesh, x + std::cos(angle) * center,
                y + std::sin(angle) * center, base_width, radius - base_axis,
                color, angle - glyph::kQuarterTurn);
      DrawShape(ui::kTriangleMesh, x + std::cos(angle) * base_axis / 2,
                y + std::sin(angle) * base_axis / 2, base_width, base_axis,
                color, angle + glyph::kQuarterTurn);
    }
  }
}

}  // namespace m1
