#pragma once

#include "components/simple_scene.h"
#include "lab_m1/Tema1/BatchRenderer.h"
#include "lab_m1/Tema1/GameRules.h"
#include "lab_m1/Tema1/tests/PerformanceChecks.h"
#include <map>
#include <memory>

namespace m1 {
class Tema1 : public gfxc::SimpleScene {
 public:
    Tema1();
    ~Tema1() override;
    void Init() override;

 private:
    struct Rect {
        float x, y, w, h;
        Rect(float X = 0, float Y = 0, float W = 0, float H = 0) : x(X), y(Y), w(W), h(H) {}
        bool Contains(glm::vec2 p) const {
            return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
        }
    };
    struct Visual {
        Rect at, from, target;
        float elapsed = 0, duration = 0.25f, vanish = 0;
        bool initialized = false, dying = false;
    };
    enum class State { Deal, Ready, Return, PlayerPlace, EnemyPlace, Reveal, Resolve, Over };

    void FrameStart() override;
    void Update(float dt) override;
    void FrameEnd() override {}
    void OnMouseMove(int x, int y, int dx, int dy) override;
    void OnMouseBtnPress(int x, int y, int buttons, int mods) override;
    void OnMouseBtnRelease(int x, int y, int buttons, int mods) override;
    void OnKeyPress(int key, int mods) override;
    void OnWindowResize(int width, int height) override;

    void Restart();
    void Viewport();
    glm::vec2 Pointer(int x, int y) const;
    void SetState(State s, float duration);
    void Advance();
    void EnemyPlay();
    void NextTurn();
    void Retarget(int id, Rect r, float duration = 0.22f);
    void Layout();
    void Animate(float dt);
    void HandleEvents(const std::vector<tema1::Event>& events);
    void CancelDrag();
    void VerifyScene(const char* directory);
    void Render();
    void Board();
    void BoardStatic();
    void Status(int owner);
    void Deck(int owner);
    void Card(int id, Rect r, bool face, bool selected = false, float flip = 1);
    void CardBody(const tema1::Card& card, bool face);
    void Symbol(tema1::Type type, float x, float y, float size, glm::vec3 color);
    void Tooltip(int id);
    void Ornament(float x, float y, float width);
    void Shape(const char* mesh, float x, float y, float w, float h,
               glm::vec3 color, float angle = 0, float alpha = 1, int pattern = 0);
    void Box(Rect r, glm::vec3 color, float alpha = 1, int pattern = 0);
    void Frame(Rect r, glm::vec3 color, float thickness = 1.5f);
    void Line(float x1, float y1, float x2, float y2, float width, glm::vec3 color);
    void Label(const std::string& value, float x, float top, float size, glm::vec3 color);
    void Center(const std::string& value, float x, float top, float size, glm::vec3 color);
    float TextWidth(const std::string& value, float size) const;
    glm::vec3 TypeColor(tema1::Type type) const;

    tema1::Game game;
    tema1::FrameBenchmark benchmark;
    std::map<int, Visual> visuals;
    std::unique_ptr<tema1::BatchRenderer> renderer;
    tema1::BatchRenderer::DrawList boardCache;
    std::array<tema1::BatchRenderer::DrawList, 2> statusCache, deckCache;
    std::array<std::string, 2> statusKey, deckKey;
    std::map<std::pair<int,int>, tema1::BatchRenderer::DrawList> cardCache;
    glm::mat3 group = glm::mat3(1);
    State state = State::Deal;
    float clock = 0, wait = 0, time = 0;
    int dealIndex = 0, enemyCard = -1, effectIndex = 0, dragged = -1, hovered = -1;
    glm::vec2 mouse = glm::vec2(-1), grab = glm::vec2(0);
    int viewportX = 0, viewportY = 0, viewportW = 1600, viewportH = 900;
    int framebufferW = 1600, framebufferH = 900;
    std::string message, detail;
    std::vector<std::string> history;
    std::array<float, 2> hitFlash = {{0, 0}};
    bool help = false;
    bool frameDirty = true, forceRebuild = false;
    int verifyStage = 0, verifyCard = -1, verifyHand = 0;
};
}  // namespace m1
