#include "lab_m1/Tema1/config/drawing.h"

#include <cmath>
#include <sstream>
#include <string>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/palette.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/lab3/transform2D.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
using tema1::CardType;
using ui::kCanvasHeight;
}  // namespace

void Tema1::DrawShape(const char* mesh, float x, float y, float width,
                      float height, glm::vec3 color, float angle) {
  const glm::mat3 model = group_transform_ * transform2D::Translate(x, y) *
                          transform2D::Rotate(angle) *
                          transform2D::Scale(width, height);
  RenderMesh2D(meshes.at(mesh), model, color);
}

void Tema1::DrawBox(Rect bounds, glm::vec3 color) {
  DrawShape(ui::kQuadMesh, bounds.x + bounds.width / 2,
            bounds.y + bounds.height / 2, bounds.width, bounds.height, color);
}

void Tema1::DrawFrame(Rect bounds, glm::vec3 color, float thickness) {
  DrawBox(Rect(bounds.x, bounds.y, bounds.width, thickness), color);
  DrawBox(Rect(bounds.x, bounds.y + bounds.height - thickness, bounds.width,
               thickness),
          color);
  DrawBox(Rect(bounds.x, bounds.y, thickness, bounds.height), color);
  DrawBox(Rect(bounds.x + bounds.width - thickness, bounds.y, thickness,
               bounds.height),
          color);
}

void Tema1::DrawLine(float x1, float y1, float x2, float y2, float width,
                     glm::vec3 color) {
  DrawShape(ui::kQuadMesh, (x1 + x2) / 2, (y1 + y2) / 2,
            std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)), width,
            color, std::atan2(y2 - y1, x2 - x1));
}

float Tema1::MeasureTextWidth(const std::string& value, float size) const {
  float width = 0;

  for (unsigned char character : value) {
    const auto glyph = text_renderer_->Characters.find(
        character < ui::kAsciiLimit ? character : '?');

    if (glyph != text_renderer_->Characters.end()) {
      width += glyph->second.Advance >> ui::kGlyphAdvanceShift;
    }
  }

  return width * size / ui::kFontSize;
}

void Tema1::DrawLabel(const std::string& value, float x, float top, float size,
                      glm::vec3 color) {
  const float scale_y = glm::length(
                  glm::vec2(group_transform_[ui::drawing::kVerticalAxis])),
              scale_x = glm::length(
                  glm::vec2(group_transform_[ui::drawing::kHorizontalAxis]));

  if (scale_x < scale_y * ui::drawing::kMinimumTextAspect) {
    return;
  }

  const glm::vec3 position = group_transform_ * glm::vec3(x, top, 1);
  // Geometry uses a bottom-left origin; the supplied text helper uses top-left.
  text_renderer_->RenderText(value, position.x, kCanvasHeight - position.y,
                             size * scale_y / ui::kFontSize, color);
}

void Tema1::DrawCenteredLabel(const std::string& value, float x, float top,
                              float size, glm::vec3 color) {
  DrawLabel(value, x - MeasureTextWidth(value, size) / 2, top, size, color);
}

void Tema1::DrawLabel(const std::string& value,
                      const tema1::ui::TextPlacement& placement,
                      glm::vec3 color) {
  DrawLabel(value, placement.x, placement.top, placement.size, color);
}

void Tema1::DrawCenteredLabel(const std::string& value,
                              const tema1::ui::TextPlacement& placement,
                              glm::vec3 color) {
  DrawCenteredLabel(value, placement.x, placement.top, placement.size, color);
}

void Tema1::DrawParagraph(const std::string& value, Rect bounds, float size,
                          glm::vec3 color) {
  std::istringstream words(value);
  std::string word, line;
  float top = bounds.y + bounds.height;

  while (words >> word) {
    const std::string next = line.empty() ? word : line + " " + word;

    if (!line.empty() && MeasureTextWidth(next, size) > bounds.width) {
      DrawCenteredLabel(line, bounds.x + bounds.width / 2, top, size, color);
      top -= size * ui::drawing::kParagraphLineHeight;
      line = word;
    } else {
      line = next;
    }
  }

  if (!line.empty()) {
    DrawCenteredLabel(line, bounds.x + bounds.width / 2, top, size, color);
  }
}

glm::vec3 Tema1::CardColor(CardType type) const {
  return ui::kCardColors[static_cast<int>(type)];
}

}  // namespace m1
