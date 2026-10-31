#ifndef LAB_M1_TEMA1_GEOMETRY_H_
#define LAB_M1_TEMA1_GEOMETRY_H_

#include "utils/glm_utils.h"

namespace tema1 {
namespace ui {

struct Rect {
  float x;
  float y;
  float width;
  float height;

  explicit Rect(float left = 0, float bottom = 0, float rect_width = 0,
                float rect_height = 0)
      : x(left), y(bottom), width(rect_width), height(rect_height) {}

  bool Contains(glm::vec2 point) const {
    return point.x >= x && point.x <= x + width && point.y >= y &&
           point.y <= y + height;
  }

  float CenterX() const {
    return x + width / 2;
  }

  float CenterY() const {
    return y + height / 2;
  }

  Rect Expanded(float padding) const {
    return Rect(x - padding, y - padding, width + 2 * padding,
                height + 2 * padding);
  }
};

struct TextPlacement {
  float x;
  float top;
  float size;
};

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_GEOMETRY_H_
