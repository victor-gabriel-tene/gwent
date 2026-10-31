#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/config/text.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
using tema1::Action;
using tema1::CardType;
using tema1::Lane;
using tema1::Leader;
}  // namespace

Action Tema1::DropAction(int id, glm::vec2 point) const {
  Lane lane = Lane::kNone;

  for (int row = 0; row < tema1::kLaneCount; ++row) {
    if (LaneRect(tema1::kHumanPlayer, static_cast<Lane>(row)).Contains(point)) {
      lane = static_cast<Lane>(row);
    }
  }

  int target = -1;

  if (id >= 0 && game_.cards_[id].type == CardType::kDecoy &&
      lane != Lane::kNone) {
    const auto& board = game_.players_[tema1::kHumanPlayer].board;

    for (auto it = board.rbegin(); it != board.rend(); ++it) {
      const auto v = card_visuals_.find(*it);

      if (game_.CanPlay(tema1::kHumanPlayer, id, lane, *it) &&
          v != card_visuals_.end() && v->second.bounds.Contains(point)) {
        target = *it;
        break;
      }
    }
  }

  return Action(Action::kPlayCard, id, lane, target);
}

void Tema1::OnMouseMove(int x, int y, int, int) {
  mouse_position_ = ToLogicalPosition(x, y);

  if (dragged_card_ >= 0) {
    card_visuals_[dragged_card_].bounds.x = mouse_position_.x - grab_offset_.x;
    card_visuals_[dragged_card_].bounds.y = mouse_position_.y - grab_offset_.y;
  }
}

void Tema1::OnMouseBtnPress(int x, int y, int buttons, int) {
  if (!IS_BIT_SET(buttons, GLFW_MOUSE_BUTTON_LEFT)) {
    return;
  }

  mouse_position_ = ToLogicalPosition(x, y);

  if (show_rules_) {
    show_rules_ = false;
    return;
  }

  if (show_inspection_) {
    if (PopupButtonRect().Contains(mouse_position_)) {
      show_inspection_ = false;
    }

    return;
  }

  if (state_ == State::kSelectLeader) {
    for (int i = 0; i < static_cast<int>(Leader::kCount); ++i) {
      if (LeaderChoiceRect(i).Contains(mouse_position_)) {
        if (selected_leader_ == i) {
          StartMatch();
        } else {
          selected_leader_ = i;
        }

        return;
      }
    }

    return;
  }

  if (state_ == State::kOver && PopupButtonRect().Contains(mouse_position_)) {
    Restart();
    return;
  }

  if (state_ == State::kRoundOver &&
      PopupButtonRect().Contains(mouse_position_)) {
    BeginNextRound();
    return;
  }

  if (RulesRect().Contains(mouse_position_)) {
    CancelDrag();
    show_rules_ = true;
    return;
  }

  if (state_ != State::kReady || dragged_card_ >= 0) {
    return;
  }

  if (LeaderRect(tema1::kHumanPlayer).Contains(mouse_position_)) {
    SubmitAction(Action(Action::kUseLeader));
    return;
  }

  for (auto it = game_.players_[tema1::kHumanPlayer].hand.rbegin();
       it != game_.players_[tema1::kHumanPlayer].hand.rend(); ++it) {
    if (card_visuals_[*it].bounds.Contains(mouse_position_)) {
      dragged_card_ = *it;
      grab_offset_ = mouse_position_ - glm::vec2(card_visuals_[*it].bounds.x,
                                                 card_visuals_[*it].bounds.y);
      return;
    }
  }
}

void Tema1::OnMouseBtnRelease(int x, int y, int buttons, int) {
  if (!IS_BIT_SET(buttons, GLFW_MOUSE_BUTTON_LEFT) || dragged_card_ < 0) {
    return;
  }

  mouse_position_ = ToLogicalPosition(x, y);
  const int id = dragged_card_;
  const Action action = DropAction(id, mouse_position_);
  dragged_card_ = tema1::kNoCard;

  if (game_.CanAct(tema1::kHumanPlayer, action)) {
    SubmitAction(action);
  } else {
    LayoutCards();
    message_ = tema1::text::kReturningCard;
    SetState(State::kReturn, ui::kInvalidDropDuration);
  }
}

void Tema1::CancelDrag() {
  if (dragged_card_ < 0) {
    return;
  }

  dragged_card_ = tema1::kNoCard;
  LayoutCards();
  SetState(State::kReturn, ui::kInvalidDropDuration);
}

void Tema1::OnKeyPress(int key, int) {
  if (show_inspection_) {
    if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) {
      show_inspection_ = false;
    }

    return;
  }

  if (key == GLFW_KEY_H && state_ != State::kSelectLeader) {
    CancelDrag();
    show_rules_ = !show_rules_;
    return;
  }

  if (show_rules_) {
    return;
  }

  if (key == GLFW_KEY_R && state_ == State::kOver) {
    Restart();
  }

  if (key == GLFW_KEY_ENTER && state_ == State::kRoundOver) {
    BeginNextRound();
  }

  if (key == GLFW_KEY_BACKSPACE && dragged_card_ >= 0) {
    CancelDrag();
  }
}

void Tema1::OnWindowResize(int, int) {
  UpdateViewport();
}

}  // namespace m1
