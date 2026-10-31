#pragma once

// Glyph drawing adapted from components/text_renderer.cpp, which credits
// Breakout and is licensed under CC BY 4.0 (or any later version).
#include "components/text_renderer.h"

namespace tema1 {

// Keep the framework's font loading, glyphs and shader. Only the drawing buffer
// is local, because the framework's VAO/VBO are private.
class TextRenderer : public gfxc::TextRenderer {
 public:
    TextRenderer(const std::string& selfDir, GLuint width, GLuint height)
        : gfxc::TextRenderer(selfDir, width, height) {
        glGenVertexArrays(1, &vertexArray);
        glGenBuffers(1, &vertexBuffer);
        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr, GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    ~TextRenderer() {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteVertexArrays(1, &vertexArray);
    }

    // Same glyph positions, texture coordinates and draw order as the supplied
    // RenderText. Replace the complete buffer instead of overwriting busy data.
    void RenderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale,
                    glm::vec3 color = glm::vec3(1)) {
        glUseProgram(m_textShader->program);
        CheckOpenGLError();
        const int colorLocation = glGetUniformLocation(m_textShader->program, "textColor");
        glUniform3f(colorLocation, color.r, color.g, color.b);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vertexArray);

        for (auto c = text.cbegin(); c != text.cend(); ++c) {
            const gfxc::Character& ch = Characters[*c];
            const GLfloat xpos = x + ch.Bearing.x * scale;
            const GLfloat ypos = y + (Characters['H'].Bearing.y - ch.Bearing.y) * scale;
            const GLfloat w = ch.Size.x * scale;
            const GLfloat h = ch.Size.y * scale;
            const GLfloat vertices[6][4] = {
                {xpos,     ypos + h, 0, 1},
                {xpos + w, ypos,     1, 0},
                {xpos,     ypos,     0, 0},
                {xpos,     ypos + h, 0, 1},
                {xpos + w, ypos + h, 1, 1},
                {xpos + w, ypos,     1, 0}
            };

            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            // glBufferData is also used in Lab 2. Fresh storage lets the GPU
            // finish the preceding glyph without making this upload wait.
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisable(GL_BLEND);
            x += (ch.Advance >> 6) * scale;
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

 private:
    GLuint vertexArray = 0, vertexBuffer = 0;
};

}  // namespace tema1
