#include <algorithm>
#include <cstddef>
#include <vector>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/scene.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
using tema1::Lane;
using ui::kCanvasHeight;
using ui::kCanvasWidth;
using ui::kLanePadding;

// Cubic smoothstep starts and ends with zero velocity.
float SmoothStep(float progress) {
  return progress * progress * (3 - 2 * progress);
}

}  // namespace

Tema1::Rect Tema1::LaneRect(int owner, Lane lane) const {
  const int row = static_cast<int>(lane);

  if (owner < 0 || owner >= tema1::kPlayerCount || row < 0 ||
      row >= tema1::kLaneCount) {
    return Rect();
  }

  const float y = owner == tema1::kHumanPlayer
                      ? ui::kPlayerMeleeBottom - row * ui::kLaneStride
                      : ui::kEnemyMeleeBottom + row * ui::kLaneStride;

  return Rect(ui::kContentLeft, y, ui::kContentWidth, ui::kLaneHeight);
}

Tema1::Rect Tema1::HandRect(int owner) const {
  return Rect(ui::kContentLeft,
              owner == tema1::kHumanPlayer ? ui::kPlayerHandBottom
                                           : ui::kEnemyHandBottom,
              ui::kContentWidth, ui::kHandHeight);
}

Tema1::Rect Tema1::DeckRect(int owner) const {
  const Rect hand = HandRect(owner);

  return Rect(hand.x + hand.width + ui::kDeckGap,
              hand.y + (hand.height - ui::kDeckHeight) / 2, ui::kDeckWidth,
              ui::kDeckHeight);
}

Tema1::Rect Tema1::RulesRect() const {
  return Rect(ui::kRulesLeft, ui::kRulesBottom, ui::kRulesWidth,
              ui::kRulesHeight);
}

Tema1::Rect Tema1::PopupButtonRect() const {
  return Rect(ui::kPopupButtonLeft, ui::kPopupButtonBottom,
              ui::kPopupButtonWidth, ui::kPopupButtonHeight);
}

Tema1::Rect Tema1::LeaderRect(int owner) const {
  return Rect(ui::kLeaderLeft,
              owner == tema1::kHumanPlayer ? ui::kPlayerLeaderBottom
                                           : ui::kEnemyLeaderBottom,
              ui::kLeaderWidth, ui::kLeaderHeight);
}

Tema1::Rect Tema1::LeaderChoiceRect(int index) const {
  return Rect(ui::kLeaderChoiceLeft + index * ui::kLeaderChoiceStride,
              ui::kLeaderChoiceBottom, ui::kLeaderChoiceWidth,
              ui::kLeaderChoiceHeight);
}

void Tema1::UpdateViewport() {
  GLFWwindow* handle = glfwGetCurrentContext();

  if (!handle) {
    return;
  }

  glfwGetFramebufferSize(handle, &framebuffer_width_, &framebuffer_height_);
  const float scale = std::min(framebuffer_width_ / kCanvasWidth,
                               framebuffer_height_ / kCanvasHeight);
  viewport_width_ = std::max(1, static_cast<int>(kCanvasWidth * scale));
  viewport_height_ = std::max(1, static_cast<int>(kCanvasHeight * scale));
  viewport_x_ = (framebuffer_width_ - viewport_width_) / 2;
  viewport_y_ = (framebuffer_height_ - viewport_height_) / 2;
  glViewport(viewport_x_, viewport_y_, viewport_width_, viewport_height_);
}

glm::vec2 Tema1::ToLogicalPosition(int x, int y) const {
  int width = 0, height = 0;
  GLFWwindow* handle = glfwGetCurrentContext();

  if (!handle) {
    return glm::vec2(-1);
  }

  glfwGetWindowSize(handle, &width, &height);

  if (width <= 0 || height <= 0) {
    return glm::vec2(-1);
  }

  return glm::vec2(
      (x * static_cast<float>(framebuffer_width_) / width - viewport_x_) *
          kCanvasWidth / viewport_width_,
      (framebuffer_height_ -
       y * static_cast<float>(framebuffer_height_) / height - viewport_y_) *
          kCanvasHeight / viewport_height_);
}

void Tema1::RetargetCard(int id, Rect destination, float duration) {
  Visual& visual = card_visuals_[id];

  if (!visual.initialized) {
    visual.bounds = destination;
    visual.initialized = true;
  }

  visual.dying = false;
  visual.vanish_progress = 0;
  visual.start_bounds = visual.bounds;
  visual.target_bounds = destination;
  visual.elapsed = 0;
  visual.duration = duration;
}

void Tema1::LayoutCards() {
  for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
    const auto& hand = game_.players_[owner].hand;

    if (!hand.empty()) {
      const Rect bounds = HandRect(owner);
      const float scale =
          std::min(1.0f, bounds.width / (hand.size() * ui::kHandCardStride -
                                         ui::kHandCardGap));
      const float width =
          hand.size() * ui::kHandCardStride * scale - ui::kHandCardGap * scale;

      for (std::size_t i = 0; i < hand.size(); ++i) {
        if (hand[i] != dragged_card_) {
          RetargetCard(
              hand[i],
              Rect(bounds.x + (bounds.width - width) / 2 +
                       i * ui::kHandCardStride * scale,
                   bounds.y + (bounds.height - ui::kCardHeight * scale) / 2,
                   ui::kCardWidth * scale, ui::kCardHeight * scale));
        }
      }
    }

    for (int row = 0; row < tema1::kLaneCount; ++row) {
      const Lane lane = static_cast<Lane>(row);
      const Rect bounds = LaneRect(owner, lane);
      std::vector<int> ids;

      for (int id : game_.players_[owner].board) {
        if (game_.cards_[id].lane == lane) {
          ids.push_back(id);
        }
      }

      if (ids.empty()) {
        continue;
      }

      const float scale = std::min(
          1.0f, (bounds.width - 2 * kLanePadding) /
                    (ids.size() * ui::kBoardCardStride - ui::kBoardCardGap));
      const float width =
          ids.size() * ui::kBoardCardStride * scale - ui::kBoardCardGap * scale;
      const float card_width = ui::kBoardCardWidth * scale,
                  card_height = card_width * ui::kCardAspectRatio;
      const float center = bounds.x + bounds.width / 2;

      for (std::size_t i = 0; i < ids.size(); ++i) {
        RetargetCard(ids[i],
                     Rect(center - width / 2 + i * ui::kBoardCardStride * scale,
                          bounds.y + (bounds.height - card_height) / 2,
                          card_width, card_height));
      }
    }
  }
}

void Tema1::AnimateCards(float delta_time) {
  for (auto& entry : card_visuals_) {
    Visual& visual = entry.second;

    if (entry.first == dragged_card_) {
      continue;
    }

    if (visual.elapsed < visual.duration) {
      visual.elapsed = std::min(visual.duration, visual.elapsed + delta_time);
      float progress = visual.elapsed /
                       std::max(ui::kMinimumAnimationDuration, visual.duration);
      progress = SmoothStep(progress);
      visual.bounds =
          Rect(visual.start_bounds.x +
                   (visual.target_bounds.x - visual.start_bounds.x) * progress,
               visual.start_bounds.y +
                   (visual.target_bounds.y - visual.start_bounds.y) * progress,
               visual.start_bounds.width +
                   (visual.target_bounds.width - visual.start_bounds.width) *
                       progress,
               visual.start_bounds.height +
                   (visual.target_bounds.height - visual.start_bounds.height) *
                       progress);
    }

    if (visual.dying && visual.vanish_progress < 1) {
      visual.vanish_progress = std::min(
          1.0f, visual.vanish_progress + delta_time / ui::kVanishDuration);
    }
  }

  for (float& flash : hit_flash_) {
    if (flash > 0) {
      flash = std::max(0.0f, flash - delta_time);
    }
  }
}

}  // namespace m1
