#include <algorithm>
#include <cmath>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/drawing.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/config/symbols.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
namespace glyph = tema1::ui::symbol;
using ui::kPi;
}  // namespace

void Tema1::RenderScene() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDisable(GL_BLEND);
  group_transform_ = glm::mat3(1);

  if (state_ == State::kSelectLeader) {
    DrawLeaderMenu();
  } else {
    DrawBoard();
    DrawStatus(tema1::kHumanPlayer);
    DrawStatus(tema1::kEnemyPlayer);

    for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
      for (int id : game_.players_[owner].board) {
        bool face = true;
        float flip = 1;

        if (id == enemy_card_ &&
            (state_ == State::kEnemyPlace || state_ == State::kReveal)) {
          face = false;

          if (state_ == State::kReveal) {
            const float angle =
                kPi * std::min(1.0f, state_elapsed_ / ui::kRevealDuration);
            flip = std::max(ui::kMinimumFlipScale, std::abs(std::cos(angle)));
            face = angle >= glyph::kQuarterTurn;
          }
        }

        DrawCard(id, card_visuals_[id].bounds, face, id == hovered_card_, flip);
      }

      for (int id : game_.players_[owner].hand) {
        if (id != dragged_card_) {
          DrawCard(id, card_visuals_[id].bounds,
                   owner == tema1::kHumanPlayer || game_.cards_[id].known,
                   id == hovered_card_);
        }
      }
    }

    for (const auto& entry : card_visuals_) {
      if (entry.second.dying && entry.second.vanish_progress < 1) {
        const Visual& visual = entry.second;
        const float scale = 1 - visual.vanish_progress;
        const Rect bounds(
            visual.bounds.x + visual.bounds.width * (1 - scale) / 2,
            visual.bounds.y + visual.bounds.height * (1 - scale) / 2,
            visual.bounds.width * scale, visual.bounds.height * scale);

        if (scale > ui::drawing::kMinimumDyingCardScale) {
          DrawCard(entry.first, bounds, true);
        }
      }
    }

    DrawLeaderPanel(tema1::kHumanPlayer);
    DrawLeaderPanel(tema1::kEnemyPlayer);

    if (hovered_card_ >= 0 && dragged_card_ < 0) {
      DrawCardTooltip(hovered_card_);
    }

    if (dragged_card_ >= 0) {
      DrawCard(dragged_card_, card_visuals_[dragged_card_].bounds, true, true);
    }

    if (state_ == State::kRoundOver || state_ == State::kOver) {
      DrawResultPopup();
    }

    if (show_inspection_) {
      DrawInspectionPopup();
    }
  }

  if (show_rules_) {
    DrawRulesOverlay();
  }
}

}  // namespace m1
