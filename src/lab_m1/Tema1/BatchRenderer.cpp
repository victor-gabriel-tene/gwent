#include "lab_m1/Tema1/BatchRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include "ft2build.h"
#include FT_FREETYPE_H

namespace tema1 {

BatchRenderer::BatchRenderer() {
    vertices.reserve(32768);
    uploaded.reserve(32768);
    textLayouts.reserve(128);
    primitives[0] = {{-.5f,-.5f},{.5f,-.5f},{.5f,.5f},{-.5f,-.5f},{.5f,.5f},{-.5f,.5f}};
    primitives[1] = {{0,.5f},{-.5f,-.5f},{.5f,-.5f}};
    for (int i = 0; i < 48; ++i) {
        const float a = 6.28318530718f * i / 48, b = 6.28318530718f * (i+1) / 48;
        primitives[2].emplace_back(0,0);
        primitives[2].emplace_back(.5f*std::cos(a),.5f*std::sin(a));
        primitives[2].emplace_back(.5f*std::cos(b),.5f*std::sin(b));
    }
}

BatchRenderer::~BatchRenderer() {
    glDeleteBuffers(3,buffers.data());
    glDeleteVertexArrays(3,vaos.data());
    if (atlas) glDeleteTextures(1,&atlas);
}

void BatchRenderer::Init(const std::string& font) {
    // Build the ASCII atlas once. All text then samples one immutable texture.
    FT_Library library;
    if (FT_Init_FreeType(&library)) throw std::runtime_error("Cannot initialize Tema1 font library");
    FT_Face face;
    if (FT_New_Face(library,font.c_str(),0,&face)) {
        FT_Done_FreeType(library);
        throw std::runtime_error("Cannot load Tema1 font: " + font);
    }
    FT_Set_Pixel_Sizes(face,0,32);
    const int width = 1024, height = 256;
    std::vector<unsigned char> pixels(width*height,0);
    int x = 1, y = 1, rowHeight = 0;
    for (unsigned c = 0; c < 128; ++c) {
        if (FT_Load_Char(face,c,FT_LOAD_RENDER)) continue;
        const auto& bitmap = face->glyph->bitmap;
        if (x + static_cast<int>(bitmap.width) + 1 > width) {
            x = 1; y += rowHeight + 2; rowHeight = 0;
        }
        if (y + static_cast<int>(bitmap.rows) + 1 > height) {
            FT_Done_Face(face); FT_Done_FreeType(library);
            throw std::runtime_error("Tema1 font atlas is full");
        }
        Glyph& g = glyphs[c];
        g.width = static_cast<float>(bitmap.width); g.height = static_cast<float>(bitmap.rows);
        g.bearingX = static_cast<float>(face->glyph->bitmap_left);
        g.bearingY = static_cast<float>(face->glyph->bitmap_top);
        g.advance = static_cast<float>(face->glyph->advance.x >> 6);
        g.u0 = x/float(width); g.v0 = y/float(height);
        g.u1 = (x+bitmap.width)/float(width); g.v1 = (y+bitmap.rows)/float(height);
        for (unsigned row = 0; row < bitmap.rows; ++row) {
            const unsigned sourceRow = bitmap.pitch >= 0 ? row : bitmap.rows-1-row;
            const unsigned char* source = bitmap.buffer + sourceRow * std::abs(bitmap.pitch);
            std::copy(source,source+bitmap.width,pixels.begin()+(y+row)*width+x);
        }
        x += static_cast<int>(bitmap.width) + 2;
        rowHeight = std::max(rowHeight,static_cast<int>(bitmap.rows));
    }
    capBearing = glyphs['H'].bearingY;
    FT_Done_Face(face); FT_Done_FreeType(library);

    glGenTextures(1,&atlas);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,atlas);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width,height,0,GL_RED,GL_UNSIGNED_BYTE,pixels.data());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);

    shader.reset(new Shader("Tema1Batch"));
    shader->AddShaderCode(R"GLSL(#version 330 core
layout(location=0) in vec2 position;
layout(location=1) in vec4 tint;
layout(location=2) in vec2 uv;
layout(location=3) in float style;
out vec2 point;
out vec4 color;
out vec2 texcoord;
flat out int material;
void main() {
    point = position; color = tint; texcoord = uv; material = int(style);
    gl_Position = vec4(position.x/800.0-1.0, position.y/450.0-1.0, 0.0, 1.0);
})GLSL",GL_VERTEX_SHADER);
    shader->AddShaderCode(R"GLSL(#version 330 core
in vec2 point;
in vec4 color;
in vec2 texcoord;
flat in int material;
uniform sampler2D fontAtlas;
out vec4 outColor;
void main() {
    if (material == 3) {
        outColor = vec4(color.rgb,color.a*texture(fontAtlas,texcoord).r);
        return;
    }
    float grain = 1.0;
    if (material == 1) {
        float vein = sin(point.y*0.63+sin(point.x*0.012)*3.0);
        grain = 0.92+0.045*vein+0.035*sin(point.y*2.6+point.x*0.003);
    } else if (material == 2) {
        grain = 0.96+0.025*sin(point.x*2.1)*sin(point.y*2.3);
    }
    outColor = vec4(color.rgb*grain,color.a);
})GLSL",GL_FRAGMENT_SHADER);
    shader->CreateAndLink();
    if (!shader->program) throw std::runtime_error("Cannot link Tema1 batch shader");
    glUseProgram(shader->program);
    glUniform1i(shader->GetUniformLocation("fontAtlas"),0);

    glGenVertexArrays(3,vaos.data());
    glGenBuffers(3,buffers.data());
    for (int i = 0; i < 3; ++i) {
        glBindVertexArray(vaos[i]);
        glBindBuffer(GL_ARRAY_BUFFER,buffers[i]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,x)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,u)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,style)));
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
}

void BatchRenderer::Begin() { vertices.clear(); }

void BatchRenderer::Submit(bool forceUpload) {
    // A stationary frame can reuse its immutable uploaded buffer. For changed
    // frames rotate buffers AND replace the store, never overwrite pending draws.
    const bool changed = forceUpload || vertices.size() != uploaded.size()
        || (!vertices.empty() && std::memcmp(vertices.data(),uploaded.data(),vertices.size()*sizeof(Vertex)) != 0);
    if (changed) {
        bufferIndex = (bufferIndex+1)%3;
        glBindBuffer(GL_ARRAY_BUFFER,buffers[bufferIndex]);
        glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
        uploaded = vertices;
    }
    Redraw();
}

void BatchRenderer::Redraw() {
    if (uploaded.empty()) return;
    glUseProgram(shader->program);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,atlas);
    glBindVertexArray(vaos[bufferIndex]);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(uploaded.size()));
    glBindVertexArray(0);
}

void BatchRenderer::Shape(const char* mesh, const glm::mat3& m, glm::vec4 color, int pattern) {
    const int primitive = std::strcmp(mesh,"t1_quad") == 0 ? 0
        : std::strcmp(mesh,"t1_triangle") == 0 ? 1 : 2;
    for (const glm::vec2& p : primitives[primitive]) {
        vertices.push_back({m[0][0]*p.x+m[1][0]*p.y+m[2][0],
                           m[0][1]*p.x+m[1][1]*p.y+m[2][1],
                           color.r,color.g,color.b,color.a,0,0,static_cast<float>(pattern)});
    }
}

const BatchRenderer::TextLayout& BatchRenderer::Layout(const std::string& value) const {
    auto found = textLayouts.find(value);
    if (found != textLayouts.end()) return found->second;
    // Bound the cache across arbitrarily many restarted games / changing labels.
    if (textLayouts.size() >= 1024) textLayouts.clear();
    TextLayout layout;
    layout.letters.reserve(value.size());
    for (unsigned char c : value) {
        if (c >= 128) c = '?';
        const Glyph& g = glyphs[c];
        if (g.width > 0 && g.height > 0)
            layout.letters.push_back({layout.width+g.bearingX,g.bearingY-capBearing,c});
        layout.width += g.advance;
    }
    return textLayouts.emplace(value,std::move(layout)).first->second;
}

float BatchRenderer::TextWidth(const std::string& value, float size) const {
    return Layout(value).width*size/32;
}

void BatchRenderer::Quad(float x, float top, float w, float h, glm::vec4 c, const Glyph& g) {
    const Vertex a = {x,top-h,c.r,c.g,c.b,c.a,g.u0,g.v1,3};
    const Vertex b = {x+w,top-h,c.r,c.g,c.b,c.a,g.u1,g.v1,3};
    const Vertex d = {x,top,c.r,c.g,c.b,c.a,g.u0,g.v0,3};
    const Vertex e = {x+w,top,c.r,c.g,c.b,c.a,g.u1,g.v0,3};
    vertices.insert(vertices.end(),{a,b,e,a,e,d});
}

void BatchRenderer::Text(const std::string& value, float x, float top, float size, glm::vec3 color) {
    const float scale = size/32;
    const auto& layout = Layout(value);
    for (const auto& letter : layout.letters) {
        const Glyph& g = glyphs[letter.glyph];
        Quad(x+letter.x*scale,top+letter.top*scale,g.width*scale,g.height*scale,glm::vec4(color,1),g);
    }
}

BatchRenderer::DrawList BatchRenderer::CopyFrom(size_t mark) const {
    return DrawList(vertices.begin()+mark,vertices.end());
}
BatchRenderer::DrawList BatchRenderer::ExtractFrom(size_t mark) {
    DrawList result = CopyFrom(mark);
    vertices.resize(mark);
    return result;
}
void BatchRenderer::Append(const DrawList& list) { vertices.insert(vertices.end(),list.begin(),list.end()); }
void BatchRenderer::Append(const DrawList& list, const glm::mat3& m) {
    for (const Vertex& original : list) {
        Vertex v = original;
        v.x = m[0][0]*original.x+m[1][0]*original.y+m[2][0];
        v.y = m[0][1]*original.x+m[1][1]*original.y+m[2][1];
        vertices.push_back(v);
    }
}
}
