#include "lab_m1/Tema1/config/overlays.h"

#include <cstddef>
#include <sstream>
#include <string>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/palette.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/config/text.h"
#include "lab_m1/Tema1/rules_reference.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
namespace rules = ui::rules;
namespace popup = ui::popup;
using tema1::CardDescription;
using tema1::CardName;
using tema1::CardType;
using tema1::GameResult;
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

void Tema1::DrawRulesOverlay() {
  DrawBox(Rect(0, 0, kCanvasWidth, kCanvasHeight), kDark);
  DrawFrame(rules::kPanel, kBrass, rules::kBorder);
  DrawCenteredLabel(tema1::text::kRulesTitle, rules::kTitle, kGold);
  DrawCenteredLabel(tema1::text::kRulesInstruction, rules::kInstruction,
                    kMuted);
  constexpr int kCardsPerColumn =
      (tema1::kCardTypeCount + rules::kCardColumns - 1) / rules::kCardColumns;

  for (int i = 0; i < tema1::kCardTypeCount; ++i) {
    const CardType type = static_cast<CardType>(i);
    const int column = i / kCardsPerColumn, row = i % kCardsPerColumn;
    const float x = rules::kCardsLeft + column * rules::kColumnStride,
                y = rules::kCardsTop - row * rules::kRowStride;
    DrawCardSymbol(type, x + rules::kSymbolX, y - rules::kSymbolDrop,
                   rules::kSymbolSize,
                   type == CardType::kSpy ? kIvory : CardColor(type));
    DrawLabel(CardName(type), x + rules::kTextX, y + rules::kNameLift,
              rules::kNameSize, kGold);
    std::istringstream words(CardDescription(type));
    std::string word, line;
    float top = y - rules::kDescriptionDrop;

    while (words >> word) {
      const std::string next = line.empty() ? word : line + " " + word;

      if (!line.empty() && MeasureTextWidth(next, rules::kDescriptionSize) >
                               rules::kDescriptionWidth) {
        DrawLabel(line, x + rules::kTextX, top, rules::kDescriptionSize,
                  kIvory);
        top -= rules::kDescriptionLineHeight;
        line = word;
      } else {
        line = next;
      }
    }

    if (!line.empty()) {
      DrawLabel(line, x + rules::kTextX, top, rules::kDescriptionSize, kIvory);
    }
  }

  DrawLine(rules::kDividerLeft, rules::kDividerY, rules::kDividerRight,
           rules::kDividerY, rules::kBorder, kBrass);
  DrawCenteredLabel(tema1::text::kResolutionOrder, rules::kOrderHeading,
                    kMuted);
  DrawCenteredLabel(tema1::ResolutionOrder(), rules::kOrder, kGold);
  const auto& sections = tema1::text::RuleSections();

  for (std::size_t column = 0; column < sections.size(); ++column) {
    const float x = rules::kNotesLeft + column * rules::kNotesColumnStride;
    DrawLabel(sections[column].heading, x, rules::kNotesHeadingTop,
              rules::kNotesHeadingSize, kGold);

    for (std::size_t row = 0; row < sections[column].lines.size(); ++row) {
      DrawLabel(sections[column].lines[row], x,
                rules::kNotesTop - row * rules::kNotesRowStride,
                rules::kNotesSize, kIvory);
    }
  }

  DrawCenteredLabel(tema1::text::kRulesFooter, rules::kFooter, kMuted);
}

void Tema1::DrawPopupPanel(const std::string& title) {
  const Rect panel = popup::kPanel;
  DrawBox(Rect(0, 0, kCanvasWidth, kCanvasHeight), kDark);
  DrawBox(panel, ui::kPanelBackground);
  DrawFrame(panel, kBrass, popup::kBorder);
  DrawCenteredLabel(title, popup::kTitle, kGold);
  DrawLine(panel.x + popup::kDividerInset, popup::kDividerY,
           panel.x + panel.width - popup::kDividerInset, popup::kDividerY,
           popup::kBorder, kBrass * popup::kDividerIntensity);
}

void Tema1::DrawPopupButton(const std::string& label) {
  const Rect bounds = PopupButtonRect();
  const bool hover = bounds.Contains(mouse_position_);
  DrawBox(bounds, hover ? ui::kButtonHover : ui::kInsetBackground);
  DrawFrame(bounds, hover ? kGold : kBrass, popup::kBorder);
  DrawCenteredLabel(label, bounds.x + bounds.width / 2,
                    bounds.y + bounds.height / 2 + popup::kButtonTextLift,
                    popup::kButtonTextSize, hover ? kGold : kIvory);
}

void Tema1::DrawResultPopup() {
  const bool match = state_ == State::kOver;
  DrawPopupPanel(message_);

  for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
    const float x =
        popup::kCenterX + (owner == tema1::kHumanPlayer ? -popup::kColumnOffset
                                                        : popup::kColumnOffset);
    const Player& player = game_.players_[owner];
    DrawCenteredLabel(owner == tema1::kHumanPlayer ? tema1::text::kYou
                                                   : tema1::text::kOpponent,
                      x, popup::kOwnerTop, popup::kOwnerSize, kMuted);

    if (!match) {
      DrawCenteredLabel(std::to_string(player.hp), x, popup::kHpTop,
                        popup::kHpSize, kIvory);
    }

    DrawCenteredLabel(
        std::to_string(player.crystals), x,
        match ? popup::kMatchCrystalsTop : popup::kRoundCrystalsTop,
        match ? popup::kMatchCrystalsSize : popup::kRoundCrystalsSize, kBlue);
  }

  // Each stat label sits between its two values, on the same visual baseline.
  if (!match) {
    DrawCenteredLabel(tema1::text::kHpLabel, popup::kHpLabel, kMuted);
  }

  DrawCenteredLabel(
      tema1::text::kCrystalsLabel,
      match ? popup::kMatchCrystalsLabel : popup::kRoundCrystalsLabel, kBlue);

  if (match && game_.round_result_ == GameResult::kDraw) {
    DrawCenteredLabel(tema1::text::kFinalRoundDrawn, popup::kDrawLabel, kMuted);
  }

  DrawPopupButton(match ? tema1::text::kPlayAgain : tema1::text::kNextRound);
}

void Tema1::DrawInspectionPopup() {
  DrawPopupPanel(tema1::text::kRevealedCards);
  const float step = popup::kInspectionStride,
              width = revealed_cards_.empty()
                          ? 0
                          : (revealed_cards_.size() - 1) * step +
                                popup::kInspectionWidth;

  for (std::size_t i = 0; i < revealed_cards_.size(); ++i) {
    DrawCard(
        revealed_cards_[i],
        Rect(popup::kCenterX - width / 2 + i * step, popup::kInspectionBottom,
             popup::kInspectionWidth, popup::kInspectionHeight),
        true);
  }

  DrawPopupButton(tema1::text::kContinue);
}

}  // namespace m1
