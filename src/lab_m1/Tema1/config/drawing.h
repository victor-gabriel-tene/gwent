#ifndef LAB_M1_TEMA1_CONFIG_DRAWING_H_
#define LAB_M1_TEMA1_CONFIG_DRAWING_H_

#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/geometry.h"

namespace tema1 {
namespace ui {
namespace drawing {
constexpr unsigned int kQuadIndices[] = {0, 1, 2, 0, 2, 3};
constexpr unsigned int kTriangleIndices[] = {0, 1, 2};
constexpr float kDefaultBorder = 1.5f;
constexpr float kMinimumTextAspect = 0.85f;
constexpr float kParagraphLineHeight = 1.5f;
constexpr float kMinimumDyingCardScale = 0.02f;
constexpr float kUnitHalfExtent = 0.5f;
constexpr int kHorizontalAxis = 0;
constexpr int kVerticalAxis = 1;
constexpr int kGlyphVertexCount = 6;
constexpr int kGlyphComponents = 4;
constexpr char kTextColorUniform[] = "textColor";
}  // namespace drawing

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_DRAWING_H_
