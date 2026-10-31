#include "lab_m1/Tema1/text_renderer.h"

#include "lab_m1/Tema1/config/drawing.h"

// Glyph drawing adapted from components/text_renderer.cpp (Breakout, CC
// BY 4.0).
namespace tema1 {
namespace glyph_layout = ui::drawing;

TextRenderer::TextRenderer(const std::string& self_dir, GLuint width,
                           GLuint height)
    : gfxc::TextRenderer(self_dir, width, height) {
  glGenVertexArrays(1, &vertex_array_);
  glGenBuffers(1, &vertex_buffer_);
  glBindVertexArray(vertex_array_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(GLfloat) * glyph_layout::kGlyphVertexCount *
                   glyph_layout::kGlyphComponents,
               nullptr, GL_STREAM_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, glyph_layout::kGlyphComponents, GL_FLOAT, GL_FALSE,
                        glyph_layout::kGlyphComponents * sizeof(GLfloat),
                        nullptr);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

TextRenderer::~TextRenderer() {
  glDeleteBuffers(1, &vertex_buffer_);
  glDeleteVertexArrays(1, &vertex_array_);
}

void TextRenderer::RenderText(const std::string& text, GLfloat x, GLfloat y,
                              GLfloat scale, glm::vec3 color) {
  glUseProgram(m_textShader->program);
  CheckOpenGLError();
  const int color_location = glGetUniformLocation(
      m_textShader->program, glyph_layout::kTextColorUniform);
  glUniform3f(color_location, color.r, color.g, color.b);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(vertex_array_);

  for (auto character = text.cbegin(); character != text.cend(); ++character) {
    const gfxc::Character& glyph = Characters[*character];
    const GLfloat left = x + glyph.Bearing.x * scale;
    const GLfloat top =
        y + (Characters['H'].Bearing.y - glyph.Bearing.y) * scale;
    const GLfloat width = glyph.Size.x * scale;
    const GLfloat height = glyph.Size.y * scale;
    const GLfloat vertices[glyph_layout::kGlyphVertexCount]
                          [glyph_layout::kGlyphComponents] = {
                              {left, top + height, 0, 1},
                              {left + width, top, 1, 0},
                              {left, top, 0, 0},
                              {left, top + height, 0, 1},
                              {left + width, top + height, 1, 1},
                              {left + width, top, 1, 0}};

    glBindTexture(GL_TEXTURE_2D, glyph.TextureID);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    // glBufferData is also used in Lab 2. Fresh storage lets the GPU
    // finish the preceding glyph without making this upload wait.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, glyph_layout::kGlyphVertexCount);
    glDisable(GL_BLEND);
    x += (glyph.Advance >> ui::kGlyphAdvanceShift) * scale;
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace tema1
