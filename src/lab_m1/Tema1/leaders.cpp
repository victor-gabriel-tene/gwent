#include "lab_m1/Tema1/config/leaders.h"

#include <algorithm>
#include <string>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/palette.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/config/symbols.h"
#include "lab_m1/Tema1/config/text.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
namespace panel = ui::leader_panel;
namespace preview = ui::leader_preview;
namespace menu = ui::leader_menu;
namespace glyph = tema1::ui::symbol;
using tema1::Leader;
using tema1::LeaderDescription;
using tema1::LeaderName;
using tema1::Player;
using ui::kBlue;
using ui::kBrass;
using ui::kCanvasHeight;
using ui::kCanvasWidth;
using ui::kDark;
using ui::kGold;
using ui::kIvory;
using ui::kMuted;
}  // namespace

void Tema1::DrawLeaderPanel(int owner) {
  const Player& player = game_.players_[owner];
  const Rect bounds = LeaderRect(owner);
  const bool usable = !player.leader_used;
  const bool hover = bounds.Contains(mouse_position_);
  const bool active =
      owner == tema1::kHumanPlayer
          ? state_ == State::kReady || state_ == State::kReturn ||
                dragged_card_ >= 0
          : state_ == State::kEnemyPlace || state_ == State::kReveal;
  const bool highlight = hover && owner == tema1::kHumanPlayer && usable &&
                         state_ == State::kReady && dragged_card_ < 0 &&
                         !show_rules_ && !show_inspection_;

  if (highlight) {
    DrawShape(ui::kCircleMesh, bounds.x + panel::kSymbolX,
              bounds.y + panel::kSymbolY, panel::kHoverSize, panel::kHoverSize,
              ui::kLeaderHover);
  }

  DrawLeaderShape(player.leader, bounds.x + panel::kSymbolX,
                  bounds.y + panel::kSymbolY,
                  highlight ? panel::kActiveSize : panel::kNormalSize,
                  player.leader_used ? ui::kSpentLeader : kGold);
  const std::string name = LeaderName(player.leader);
  DrawLabel(
      owner == tema1::kHumanPlayer ? tema1::text::kYou : tema1::text::kOpponent,
      bounds.x + panel::kTextX, bounds.y + panel::kOwnerTop, panel::kOwnerSize,
      active ? kGold : kMuted);
  DrawLabel(
      name, bounds.x + panel::kTextX, bounds.y + panel::kNameTop,
      std::min(panel::kNameSize,
               panel::kNameWidth / std::max(1.0f, MeasureTextWidth(name, 1))),
      kIvory);

  for (int i = 0; i < tema1::kStartingCrystals; ++i) {
    const float x = bounds.x + panel::kCrystalX + i * panel::kCrystalStride;
    DrawShape(ui::kQuadMesh, x, bounds.y + panel::kSymbolY,
              panel::kCrystalWidth, panel::kCrystalHeight,
              i < player.crystals ? kBlue : ui::kEmptyCrystal,
              glyph::kDiagonal);
  }

  if (hover && !show_rules_ && !show_inspection_ && state_ == State::kReady &&
      dragged_card_ < 0 && hovered_card_ < 0) {
    DrawLeaderShape(player.leader, preview::kCenterX, preview::kSymbolY,
                    preview::kSymbolSize,
                    player.leader_used ? ui::kSpentLeader : kGold);
    DrawParagraph(name, preview::kNameBounds, preview::kNameSize, kGold);
    DrawCenteredLabel(
        player.leader_used ? tema1::text::kUsed : tema1::text::kOncePerMatch,
        preview::kAvailability, kMuted);
    DrawParagraph(LeaderDescription(player.leader), preview::kDescriptionBounds,
                  preview::kDescriptionSize, kIvory);
  }
}

void Tema1::DrawLeaderMenu() {
  DrawBox(Rect(0, 0, kCanvasWidth, kCanvasHeight), kDark);
  DrawCenteredLabel(tema1::text::kTitle, menu::kTitle, kGold);
  DrawCenteredLabel(tema1::text::kChooseLeader, menu::kSubtitle, kIvory);
  DrawCenteredLabel(selected_leader_ >= 0 ? tema1::text::kConfirmLeader
                                          : tema1::text::kSelectLeader,
                    menu::kInstruction, kMuted);

  for (int i = 0; i < static_cast<int>(Leader::kCount); ++i) {
    const Leader leader = static_cast<Leader>(i);
    const Rect bounds = LeaderChoiceRect(i);
    const bool chosen = selected_leader_ == i,
               hover = bounds.Contains(mouse_position_);
    DrawBox(bounds, chosen ? ui::kSelectedPanel : ui::kPanelBackground);

    if (chosen || hover) {
      DrawFrame(bounds, chosen ? kGold : kBrass, menu::kBorder);
    }

    if (chosen) {
      DrawLeaderShape(leader, bounds.x + bounds.width / 2,
                      bounds.y + menu::kSymbolTop,
                      menu::kSymbolSize * menu::kSelectedScale, kBlue);
    }

    DrawLeaderShape(leader, bounds.x + bounds.width / 2,
                    bounds.y + menu::kSymbolTop, menu::kSymbolSize,
                    chosen ? kGold : ui::kLeaderColors[i]);
    const std::string name = LeaderName(leader);
    DrawCenteredLabel(name, bounds.x + bounds.width / 2,
                      bounds.y + menu::kNameTop,
                      std::min(menu::kNameSize,
                               (bounds.width - 2 * menu::kNameSidePadding) /
                                   std::max(1.0f, MeasureTextWidth(name, 1))),
                      kIvory);
    DrawParagraph(LeaderDescription(leader),
                  Rect(bounds.x + menu::kDescriptionSidePadding,
                       bounds.y + menu::kDescriptionBottom,
                       bounds.width - 2 * menu::kDescriptionSidePadding,
                       menu::kDescriptionHeight),
                  menu::kDescriptionSize, kMuted);
  }
}

}  // namespace m1
