#pragma once

#include "utils/glm_utils.h"
#include "core/gpu/shader.h"
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tema1 {

// Painter-ordered geometry and text share a shader, font atlas and draw call.
// No per-shape uniforms, per-glyph textures or in-place glyph buffer updates.
class BatchRenderer {
 public:
    struct Vertex { float x, y, r, g, b, a, u, v, style; };
    using DrawList = std::vector<Vertex>;

    BatchRenderer();
    ~BatchRenderer();
    void Init(const std::string& font);
    void Begin();
    void Submit(bool forceUpload = false);
    void Redraw();
    void Shape(const char* mesh, const glm::mat3& transform, glm::vec4 color, int pattern);
    void Text(const std::string& value, float x, float top, float size, glm::vec3 color);
    float TextWidth(const std::string& value, float size) const;

    size_t Mark() const { return vertices.size(); }
    DrawList CopyFrom(size_t mark) const;
    DrawList ExtractFrom(size_t mark);
    void Append(const DrawList& list);
    void Append(const DrawList& list, const glm::mat3& transform);
    size_t VertexCount() const { return uploaded.size(); }

 private:
    struct Glyph {
        float width = 0, height = 0, bearingX = 0, bearingY = 0, advance = 0;
        float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    };
    struct Letter { float x, top; unsigned char glyph; };
    struct TextLayout { std::vector<Letter> letters; float width = 0; };
    const TextLayout& Layout(const std::string& value) const;
    void Quad(float x, float top, float w, float h, glm::vec4 color, const Glyph& glyph);

    std::unique_ptr<Shader> shader;
    std::array<Glyph, 128> glyphs;
    std::array<GLuint, 3> vaos = {{0, 0, 0}}, buffers = {{0, 0, 0}};
    GLuint atlas = 0;
    int bufferIndex = 0;
    float capBearing = 0;
    DrawList vertices, uploaded;
    std::array<std::vector<glm::vec2>, 3> primitives;
    mutable std::unordered_map<std::string, TextLayout> textLayouts;
};
}
