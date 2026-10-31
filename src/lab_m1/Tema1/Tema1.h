#ifndef LAB_M1_TEMA1_TEMA1_H_
#define LAB_M1_TEMA1_TEMA1_H_

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "components/simple_scene.h"
#include "lab_m1/Tema1/config/animation.h"
#include "lab_m1/Tema1/config/drawing.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/game_rules.h"
#include "lab_m1/Tema1/geometry.h"
#include "lab_m1/Tema1/text_renderer.h"

namespace m1 {
class Tema1 : public gfxc::SimpleScene {
 public:
  Tema1();
  Tema1(const Tema1&) = delete;
  Tema1& operator=(const Tema1&) = delete;
  ~Tema1() override;
  void Init() override;

 private:
  using Rect = tema1::ui::Rect;

  struct Visual {
    Rect bounds;
    Rect start_bounds;
    Rect target_bounds;
    float elapsed = 0;
    float duration = tema1::ui::kDefaultAnimationDuration;
    float vanish_progress = 0;
    bool initialized = false;
    bool dying = false;
  };

  enum class State {
    kSelectLeader,
    kDeal,
    kReady,
    kReturn,
    kPlayerPlace,
    kEnemyPlace,
    kReveal,
    kResolve,
    kRoundOver,
    kOver
  };

  // Framework callbacks.
  void FrameStart() override;
  void Update(float delta_time) override;

  void FrameEnd() override {}

  void OnMouseMove(int x, int y, int dx, int dy) override;
  void OnMouseBtnPress(int x, int y, int buttons, int mods) override;
  void OnMouseBtnRelease(int x, int y, int buttons, int mods) override;
  void OnKeyPress(int key, int mods) override;
  void OnWindowResize(int width, int height) override;

  // Match and turn progression.
  void Restart();
  void StartMatch();
  void BeginNextRound();
  void ResetPresentationState();
  void SetState(State next_state, float duration);
  void AdvanceState();
  void SubmitAction(const tema1::Action& action);
  void PlayEnemyAction();
  void BeginTurn();
  void HandleEvents(const std::vector<tema1::Event>& events);

  // Input and shared layout.
  tema1::Action DropAction(int id, glm::vec2 point) const;
  void CancelDrag();
  Rect LaneRect(int owner, tema1::Lane lane) const;
  Rect HandRect(int owner) const;
  Rect DeckRect(int owner) const;
  Rect RulesRect() const;
  Rect PopupButtonRect() const;
  Rect LeaderRect(int owner) const;
  Rect LeaderChoiceRect(int index) const;
  void UpdateViewport();
  glm::vec2 ToLogicalPosition(int x, int y) const;

  // Card layout and animation.
  void RetargetCard(int id, Rect bounds,
                    float duration = tema1::ui::kMoveDuration);
  void LayoutCards();
  void AnimateCards(float delta_time);

  // Scene rendering.
  void InitMeshes();
  void RenderScene();
  void DrawBoardBackground();
  void DrawBoard();
  void DrawStatus(int owner);
  void DrawLeaderMenu();
  void DrawLeaderPanel(int owner);

  // Card presentation.
  void DrawCard(int id, Rect bounds, bool face, bool selected = false,
                float flip = 1);
  void DrawCardBody(const tema1::Card& card, bool face);
  void DrawCardValues(const tema1::Card& card, glm::vec3 ink);
  void DrawCardMarkings(const tema1::Card& card, glm::vec3 ink);
  std::string CardValueLabel(int id) const;
  void DrawCardTooltip(int id);

  // Geometric symbols.
  void DrawCardSymbol(tema1::CardType type, float x, float y, float size,
                      glm::vec3 color);
  void DrawLeaderShape(tema1::Leader leader, float x, float y, float size,
                       glm::vec3 color);
  void DrawDamageSymbol(float x, float y, float size, glm::vec3 color);
  void DrawHealSymbol(float x, float y, float size, glm::vec3 color);
  void DrawShieldSymbol(float x, float y, float size, glm::vec3 color);
  void DrawSpySymbol(float x, float y, float size, glm::vec3 color);
  void DrawFrostSymbol(float x, float y, float size, glm::vec3 color);
  void DrawClearSymbol(float x, float y, float size, glm::vec3 color);
  void DrawFogSymbol(float x, float y, float size, glm::vec3 color);
  void DrawRainSymbol(float x, float y, float size, glm::vec3 color);
  void DrawHornSymbol(float x, float y, float size, glm::vec3 color);
  void DrawScorchSymbol(float x, float y, float size, glm::vec3 color);
  void DrawMedicSymbol(float x, float y, float size, glm::vec3 color);
  void DrawDecoySymbol(float x, float y, float size, glm::vec3 color);

  // Overlays.
  void DrawRulesOverlay();
  void DrawResultPopup();
  void DrawInspectionPopup();
  void DrawPopupPanel(const std::string& title);
  void DrawPopupButton(const std::string& label);

  // Basic drawing helpers.
  void DrawShape(const char* mesh, float x, float y, float width, float height,
                 glm::vec3 color, float angle = 0);
  void DrawBox(Rect bounds, glm::vec3 color);
  void DrawFrame(Rect bounds, glm::vec3 color,
                 float thickness = tema1::ui::drawing::kDefaultBorder);
  void DrawLine(float x1, float y1, float x2, float y2, float width,
                glm::vec3 color);
  void DrawLabel(const std::string& value, float x, float top, float size,
                 glm::vec3 color);
  void DrawLabel(const std::string& value,
                 const tema1::ui::TextPlacement& placement, glm::vec3 color);
  void DrawCenteredLabel(const std::string& value, float x, float top,
                         float size, glm::vec3 color);
  void DrawCenteredLabel(const std::string& value,
                         const tema1::ui::TextPlacement& placement,
                         glm::vec3 color);
  void DrawParagraph(const std::string& value, Rect bounds, float size,
                     glm::vec3 color);
  float MeasureTextWidth(const std::string& value, float size) const;
  glm::vec3 CardColor(tema1::CardType type) const;

  // Opt-in verification.
  void VerifyScene(const char* directory);

  tema1::Game game_;
  std::map<int, Visual> card_visuals_;
  std::unique_ptr<tema1::TextRenderer> text_renderer_;
  glm::mat3 group_transform_ = glm::mat3(1);

  State state_ = State::kSelectLeader;
  float state_elapsed_ = 0;
  float state_duration_ = 0;
  float elapsed_time_ = 0;

  int dealt_card_count_ = 0;
  int enemy_card_ = tema1::kNoCard;
  int phase_index_ = 0;
  int dragged_card_ = tema1::kNoCard;
  int hovered_card_ = tema1::kNoCard;
  int cards_to_deal_ = tema1::kPlayerCount * tema1::kOpeningHandSize;
  int selected_leader_ = tema1::ui::kNoSelection;

  std::vector<int> revealed_cards_;
  glm::vec2 mouse_position_ = glm::vec2(-1);
  glm::vec2 grab_offset_ = glm::vec2(0);

  int viewport_x_ = 0;
  int viewport_y_ = 0;
  int viewport_width_ = tema1::ui::kCanvasWidth;
  int viewport_height_ = tema1::ui::kCanvasHeight;
  int framebuffer_width_ = tema1::ui::kCanvasWidth;
  int framebuffer_height_ = tema1::ui::kCanvasHeight;

  std::string message_;
  std::array<float, tema1::kPlayerCount> hit_flash_ = {{0, 0}};
  bool show_rules_ = false;
  bool show_inspection_ = false;

  // State for the opt-in scene verification driver.
  int verification_stage_ = 0;
  int verification_card_ = tema1::kNoCard;
  int verification_hand_ = 0;
};
}  // namespace m1

#endif  // LAB_M1_TEMA1_TEMA1_H_
