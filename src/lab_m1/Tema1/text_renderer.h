#ifndef LAB_M1_TEMA1_TEXT_RENDERER_H_
#define LAB_M1_TEMA1_TEXT_RENDERER_H_

#include <string>

#include "components/text_renderer.h"

namespace tema1 {

// Keep the framework's font loading, glyphs and shader. Only the drawing buffer
// is local, because the framework's VAO/VBO are private.
class TextRenderer : public gfxc::TextRenderer {
 public:
  TextRenderer(const std::string& self_dir, GLuint width, GLuint height);

  ~TextRenderer();
  TextRenderer(const TextRenderer&) = delete;
  TextRenderer& operator=(const TextRenderer&) = delete;

  // Same glyph positions, texture coordinates and draw order as the supplied
  // RenderText. Replace the complete buffer instead of overwriting busy data.
  void RenderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale,
                  glm::vec3 color = glm::vec3(1));

 private:
  GLuint vertex_array_ = 0, vertex_buffer_ = 0;
};

}  // namespace tema1

#endif  // LAB_M1_TEMA1_TEXT_RENDERER_H_
