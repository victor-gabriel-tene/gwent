#include "lab_m1/Tema1/config/board.h"

#include <algorithm>
#include <string>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/palette.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/config/text.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
namespace board = ui::board;
namespace status = ui::status;
using tema1::CardType;
using tema1::Lane;
using tema1::LaneName;
using tema1::Player;
using ui::kBlue;
using ui::kBrass;
using ui::kCanvasHeight;
using ui::kCanvasWidth;
using ui::kGold;
using ui::kIvory;
using ui::kLanePadding;
using ui::kMuted;
}  // namespace

void Tema1::DrawBoardBackground() {
  DrawBox(Rect(0, 0, kCanvasWidth, kCanvasHeight), ui::kTableBackground);
  const Rect hand = HandRect(tema1::kHumanPlayer),
             enemy_hand = HandRect(tema1::kEnemyPlayer);
  DrawFrame(
      Rect(hand.x - board::kFrameSideInset, hand.y - board::kFrameBottomInset,
           hand.width + 2 * board::kFrameSideInset,
           enemy_hand.y + enemy_hand.height - hand.y +
               board::kFrameVerticalExpansion),
      kBrass, board::kBorder);

  for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
    for (int i = 0; i < tema1::kLaneCount; ++i) {
      const Lane lane = static_cast<Lane>(i);
      const Rect bounds = LaneRect(owner, lane);
      DrawBox(bounds, owner == tema1::kHumanPlayer ? ui::kFriendlyLane
                                                   : ui::kEnemyLane);
    }
  }

  const Rect lane = LaneRect(tema1::kHumanPlayer, Lane::kMelee);
  DrawLine(lane.x, board::kCenterY, lane.x + lane.width, board::kCenterY,
           board::kBorder, kBrass);
}

void Tema1::DrawBoard() {
  DrawBoardBackground();

  if (dragged_card_ >= 0) {
    for (Lane lane : game_.LegalLanes(tema1::kHumanPlayer, dragged_card_)) {
      const Rect bounds = LaneRect(tema1::kHumanPlayer, lane);
      const bool inside = bounds.Contains(mouse_position_);
      DrawBox(bounds, inside ? ui::kHoveredLane : ui::kLegalLane);
      DrawFrame(bounds, inside ? kGold : kBrass,
                inside ? board::kDropBorder : board::kBorder);
    }

    if (game_.cards_[dragged_card_].type == CardType::kDecoy) {
      for (int id : game_.players_[tema1::kHumanPlayer].board) {
        if (!game_.CanPlay(tema1::kHumanPlayer, dragged_card_,
                           game_.cards_[id].lane, id)) {
          continue;
        }

        const Rect bounds = card_visuals_[id].bounds;
        DrawFrame(bounds.Expanded(board::kTargetPadding),
                  bounds.Contains(mouse_position_) ? kGold : kBlue,
                  board::kTargetBorder);
      }
    }
  }

  // Draw labels after the drop highlights, inside the reserved lane margin.
  for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
    for (int i = 0; i < tema1::kLaneCount; ++i) {
      const Lane lane = static_cast<Lane>(i);
      const Rect bounds = LaneRect(owner, lane);
      DrawCenteredLabel(LaneName(lane), bounds.x + kLanePadding / 2,
                        bounds.y + bounds.height / 2 + board::kLaneLabelLift,
                        board::kLaneLabelSize, kMuted);
    }
  }

  const Rect rules = RulesRect();
  const bool hover = rules.Contains(mouse_position_);
  DrawBox(rules, hover ? ui::kButtonHover : ui::kInsetBackground);
  DrawFrame(rules, hover ? kGold : kBrass, board::kBorder);
  DrawCenteredLabel(tema1::text::kRulesButton, rules.x + rules.width / 2,
                    rules.y + rules.height / 2 + board::kRulesLabelLift,
                    board::kRulesLabelSize, hover ? kGold : kIvory);
}

void Tema1::DrawStatus(int owner) {
  const Player& player = game_.players_[owner];
  const float y = LeaderRect(owner).y;
  DrawCenteredLabel(
      std::string(tema1::text::kHpPrefix) + std::to_string(player.hp),
      status::kHealthCenterX, y + status::kLabelTop, status::kTextSize,
      hit_flash_[owner] > 0 ? kGold : kMuted);

  for (int i = 0; i < tema1::kStartingHp; ++i) {
    const Rect pip(status::kHealthLeft + i * status::kPipStride,
                   y + status::kHealthBottom, status::kPipWidth,
                   status::kPipHeight);
    DrawBox(pip, i < player.hp ? (hit_flash_[owner] > 0 ? kGold : ui::kHealth)
                               : ui::kEmptyHealth);
  }

  DrawCenteredLabel(
      std::string(tema1::text::kShieldPrefix) + std::to_string(player.shield),
      status::kShieldCenterX, y + status::kLabelTop, status::kTextSize, kBlue);
  const int columns = ui::kShieldColumns,
            rows = std::max(1, (player.shield + columns - 1) / columns);
  const float step = std::min(status::kShieldMaxRowStep,
                              status::kShieldSpace / rows),
              height = std::min(status::kPipHeight, step * status::kShieldFill);

  for (int i = 0; i < rows * columns; ++i) {
    DrawBox(Rect(status::kShieldLeft + (i % columns) * status::kPipStride,
                 y + status::kShieldTop - (i / columns) * step - height,
                 status::kPipWidth, height),
            i < player.shield ? kBlue : ui::kEmptyShield);
  }
}

}  // namespace m1
