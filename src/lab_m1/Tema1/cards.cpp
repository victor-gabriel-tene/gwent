#include "lab_m1/Tema1/config/cards.h"

#include <algorithm>
#include <string>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/palette.h"
#include "lab_m1/Tema1/config/text.h"
#include "lab_m1/lab3/transform2D.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
namespace face_layout = ui::card;
namespace preview = ui::card::preview;
}  // namespace

void Tema1::DrawCard(int id, Rect bounds, bool face, bool selected,
                     float flip) {
  const glm::mat3 saved = group_transform_;
  const tema1::Card& card = game_.cards_[id];

  group_transform_ =
      group_transform_ *
      transform2D::Translate(bounds.CenterX(), bounds.CenterY()) *
      transform2D::Scale(bounds.width / ui::kCardWidth * flip,
                         bounds.height / ui::kCardHeight) *
      transform2D::Translate(-ui::kCardWidth / 2, -ui::kCardHeight / 2);

  DrawCardBody(card, face);

  if (selected) {
    DrawFrame(face_layout::kFace.Expanded(face_layout::kSelectionPadding),
              ui::kGold, face_layout::kSelectionBorder);
  }

  group_transform_ = saved;
}

void Tema1::DrawCardBody(const tema1::Card& card, bool face) {
  DrawBox(face_layout::kFace,
          face ? CardColor(card.type) : ui::kInsetBackground);

  if (!face) {
    return;
  }

  const bool dark_ink = card.type == tema1::CardType::kFrost ||
                        card.type == tema1::CardType::kClear;
  const glm::vec3 ink = dark_ink ? ui::kDark : ui::kIvory;

  DrawCardSymbol(card.type, face_layout::kSymbolX, face_layout::kSymbolY,
                 face_layout::kSymbolSize, ink);

  DrawCardValues(card, ink);
  DrawCardMarkings(card, ink);
}

void Tema1::DrawCardValues(const tema1::Card& card, glm::vec3 ink) {
  const int value = std::max(0, game_.EffectiveValue(card.id));
  const bool boosted = value > card.value || card.value > card.printed_value;
  const bool multiple_columns = value > face_layout::kSingleColumnLimit;
  const int rows = multiple_columns ? (value + face_layout::kValueColumns - 1) /
                                          face_layout::kValueColumns
                                    : value;

  const float step = std::min(face_layout::kValueMaxStep,
                              face_layout::kValueSpace / std::max(1, rows));
  const float height =
      std::min(face_layout::kValueMaxHeight, step * face_layout::kValueFill);

  for (int i = 0; i < value; ++i) {
    const int column = multiple_columns ? i / rows : 0;
    const int row = multiple_columns ? i % rows : i;
    const Rect pip(
        face_layout::kValueLeft + column * face_layout::kValueColumnStride,
        face_layout::kValueTop - row * step, face_layout::kValueWidth, height);

    DrawBox(pip, boosted ? ui::kBoostedValue : ink);
  }
}

void Tema1::DrawCardMarkings(const tema1::Card& card, glm::vec3 ink) {
  const tema1::Lane lane =
      card.zone == tema1::CardZone::kBoard ? card.lane : card.printed_lane;

  if (lane != tema1::Lane::kNone) {
    DrawCenteredLabel(tema1::LaneSymbol(lane), face_layout::kLaneLabel, ink);
  }

  const char* category_mesh =
      tema1::IsTemporary(card.type) ? ui::kTriangleMesh : ui::kCircleMesh;

  DrawShape(category_mesh, face_layout::kCategoryX, face_layout::kCategoryY,
            face_layout::kCategorySize, face_layout::kCategorySize, ink);

  if (card.suppressed) {
    const Rect bounds = face_layout::kSuppressionBounds;

    DrawLine(bounds.x, bounds.y, bounds.x + bounds.width,
             bounds.y + bounds.height, face_layout::kSuppressionStroke,
             ui::kIvory);
    DrawLine(bounds.x, bounds.y + bounds.height, bounds.x + bounds.width,
             bounds.y, face_layout::kSuppressionStroke, ui::kIvory);
  }
}

std::string Tema1::CardValueLabel(int id) const {
  const tema1::Card& card = game_.cards_[id];
  const int effective = game_.EffectiveValue(id);
  const bool temporary = tema1::IsTemporary(card.type);
  std::string label = tema1::text::kValuePrefix + std::to_string(effective);

  if (effective != card.value) {
    label +=
        std::string(tema1::text::kValueSeparator) +
        (temporary ? tema1::text::kLifetimePrefix : tema1::text::kBasePrefix) +
        std::to_string(card.value);
  }

  if (card.value > card.printed_value) {
    label +=
        tema1::text::kPrintedSeparator + std::to_string(card.printed_value);

    if (temporary && effective == card.value) {
      label += tema1::text::kLifetimeSeparator + std::to_string(card.value);
    }
  }

  return label;
}

void Tema1::DrawCardTooltip(int id) {
  const tema1::Card& card = game_.cards_[id];

  if (card.owner == tema1::kEnemyPlayer &&
      card.zone == tema1::CardZone::kHand && !card.known) {
    return;
  }

  DrawCard(id, preview::kBounds, true);
  DrawCenteredLabel(tema1::CardName(card.type), preview::kName, ui::kGold);
  DrawCenteredLabel(tema1::IsTemporary(card.type) ? tema1::text::kTemporary
                                                  : tema1::text::kPermanent,
                    preview::kCategory, ui::kMuted);

  DrawParagraph(CardValueLabel(id), preview::kValueBounds, preview::kValueSize,
                ui::kIvory);
  DrawParagraph(tema1::CardDescription(card.type), preview::kDescriptionBounds,
                preview::kDescriptionSize, ui::kMuted);
}

}  // namespace m1
