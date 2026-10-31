#include "lab_m1/Tema1/Tema1.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include "lab_m1/Tema1/config/palette.h"
#include "lab_m1/Tema1/config/scene.h"
#include "lab_m1/Tema1/config/text.h"

namespace m1 {
namespace {
namespace ui = tema1::ui;
using tema1::Action;
using tema1::CardName;
using tema1::CardZone;
using tema1::EffectPhase;
using tema1::EffectPhases;
using tema1::Event;
using tema1::GameResult;
using tema1::Leader;
using tema1::PhaseName;
using ui::kCanvasHeight;
using ui::kCanvasWidth;

unsigned MatchSeed() {
  const char* value = std::getenv(ui::kSeedEnvironment);

  return value ? static_cast<unsigned>(
                     std::strtoul(value, nullptr, ui::kSeedNumberBase))
               : static_cast<unsigned>(std::time(nullptr)) ^
                     static_cast<unsigned>(std::rand());
}
}  // namespace

Tema1::Tema1() : game_(tema1::kDefaultSeed) {}

Tema1::~Tema1() = default;

void Tema1::Init() {
  // Same fixed, orthographic 2D camera setup as Lab 3.
  auto camera = GetSceneCamera();
  camera->SetOrthographic(0, kCanvasWidth, 0, kCanvasHeight, ui::kCameraNear,
                          ui::kCameraFar);
  camera->SetPosition(glm::vec3(0, 0, ui::kCameraDistance));
  camera->SetRotation(glm::vec3(0));
  camera->Update();
  GetCameraInput()->SetActive(false);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glfwSetWindowTitle(glfwGetCurrentContext(), ui::kWindowTitle);
  InitMeshes();

  text_renderer_.reset(new tema1::TextRenderer(window->props.selfDir,
                                               kCanvasWidth, kCanvasHeight));
  text_renderer_->Load(
      PATH_JOIN(window->props.selfDir, "assets", "fonts", ui::kFontFile),
      ui::kFontSize);

  Restart();
}

void Tema1::Restart() {
  game_.ResetEmpty(MatchSeed());
  card_visuals_.clear();
  selected_leader_ = ui::kNoSelection;
  ResetPresentationState();

  cards_to_deal_ = tema1::kPlayerCount * tema1::kOpeningHandSize;
  message_ = tema1::text::kChooseLeader;
  SetState(State::kSelectLeader, 0);
}

void Tema1::StartMatch() {
  if (selected_leader_ < 0 ||
      selected_leader_ >= static_cast<int>(Leader::kCount)) {
    return;
  }

  game_.Reset(MatchSeed(), false);
  game_.players_[tema1::kHumanPlayer].leader =
      static_cast<Leader>(selected_leader_);
  card_visuals_.clear();
  ResetPresentationState();

  cards_to_deal_ = tema1::kPlayerCount * tema1::kOpeningHandSize;
  message_ = std::string(tema1::text::kRoundPrefix) +
             std::to_string(game_.round_) + tema1::text::kDealSuffix;
  SetState(State::kDeal, ui::kInitialDealDelay);
}

void Tema1::BeginNextRound() {
  if (!game_.NextRound(false)) {
    return;
  }

  // Preserve continuing hand visuals and stable IDs; discard visuals belong to
  // the old round.
  for (auto it = card_visuals_.begin(); it != card_visuals_.end();) {
    if (game_.cards_[it->first].zone != CardZone::kHand) {
      it = card_visuals_.erase(it);
    } else {
      ++it;
    }
  }

  ResetPresentationState();

  cards_to_deal_ = tema1::kPlayerCount * tema1::kCardsPerRound;
  message_ = tema1::text::kRoundPrefix + std::to_string(game_.round_);
  LayoutCards();
  SetState(State::kDeal, ui::kRoundDealDelay);
}

void Tema1::ResetPresentationState() {
  revealed_cards_.clear();
  dealt_card_count_ = 0;
  phase_index_ = 0;

  dragged_card_ = tema1::kNoCard;
  hovered_card_ = tema1::kNoCard;
  enemy_card_ = tema1::kNoCard;

  hit_flash_.fill(0);
  show_rules_ = false;
  show_inspection_ = false;
}

void Tema1::FrameStart() {
  glDisable(GL_SCISSOR_TEST);
  glClearColor(ui::kClearColor.r, ui::kClearColor.g, ui::kClearColor.b, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  UpdateViewport();
}

void Tema1::SetState(State next_state, float duration) {
  state_ = next_state;
  state_elapsed_ = 0;
  state_duration_ = duration;
}

void Tema1::Update(float delta_time) {
  delta_time = std::max(0.0f, delta_time);
  elapsed_time_ += delta_time;
  const glm::ivec2 cursor = window->GetCursorPosition();
  mouse_position_ = ToLogicalPosition(cursor.x, cursor.y);

  if (dragged_card_ >= 0 &&
      (!glfwGetWindowAttrib(glfwGetCurrentContext(), GLFW_FOCUSED) ||
       !window->MouseHold(GLFW_MOUSE_BUTTON_LEFT))) {
    CancelDrag();
  }

  AnimateCards(delta_time);

  if (!show_rules_ && !show_inspection_) {
    state_elapsed_ += delta_time;

    if (state_ != State::kReady && state_ != State::kOver &&
        state_ != State::kRoundOver && state_ != State::kSelectLeader &&
        state_elapsed_ >= state_duration_) {
      AdvanceState();
    }
  }

  hovered_card_ = tema1::kNoCard;

  if (!show_rules_ && !show_inspection_ && state_ == State::kReady &&
      dragged_card_ < 0) {
    for (auto it = game_.players_[tema1::kHumanPlayer].hand.rbegin();
         it != game_.players_[tema1::kHumanPlayer].hand.rend(); ++it) {
      if (card_visuals_[*it].bounds.Contains(mouse_position_)) {
        hovered_card_ = *it;
        break;
      }
    }

    if (hovered_card_ < 0) {
      for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
        for (int id : game_.players_[owner].board) {
          if (card_visuals_[id].bounds.Contains(mouse_position_)) {
            hovered_card_ = id;
          }
        }

        if (owner == tema1::kEnemyPlayer) {
          for (int id : game_.players_[owner].hand) {
            if (game_.cards_[id].known &&
                card_visuals_[id].bounds.Contains(mouse_position_)) {
              hovered_card_ = id;
            }
          }
        }
      }
    }
  }

  RenderScene();

  if (const char* directory = std::getenv(ui::kVerificationEnvironment)) {
    VerifyScene(directory);
  }
}

void Tema1::AdvanceState() {
  if (state_ == State::kDeal) {
    if (dealt_card_count_ < cards_to_deal_) {
      const int owner = dealt_card_count_ % tema1::kPlayerCount,
                id = game_.Draw(owner);

      if (id >= 0) {
        Visual& visual = card_visuals_[id];
        visual.initialized = true;
        visual.bounds = DeckRect(owner);
        LayoutCards();
        RetargetCard(id, card_visuals_[id].target_bounds, ui::kDrawDuration);
      }

      ++dealt_card_count_;
      SetState(State::kDeal, dealt_card_count_ == cards_to_deal_
                                 ? ui::kLastDealDelay
                                 : ui::kDealInterval);
    } else {
      BeginTurn();
    }
  } else if (state_ == State::kReturn) {
    BeginTurn();
  } else if (state_ == State::kPlayerPlace) {
    PlayEnemyAction();
  } else if (state_ == State::kEnemyPlace) {
    if (enemy_card_ >= 0) {
      SetState(State::kReveal, ui::kRevealDuration);
    } else {
      phase_index_ = 0;
      SetState(State::kResolve, ui::kBeforeResolveDelay);
    }
  } else if (state_ == State::kReveal) {
    phase_index_ = 0;
    SetState(State::kResolve, ui::kAfterRevealDelay);
  } else if (state_ == State::kResolve) {
    const auto& phases = EffectPhases();

    if (phase_index_ < static_cast<int>(phases.size()) &&
        game_.round_result_ == GameResult::kPlaying) {
      const EffectPhase phase = phases[phase_index_++];
      message_ = std::string(tema1::text::kResolvingPrefix) + PhaseName(phase);
      const auto events = game_.Resolve(phase);
      HandleEvents(events);
      LayoutCards();
      SetState(State::kResolve,
               events.empty() ? ui::kEmptyPhaseDelay : ui::kPhaseDelay);
    } else {
      HandleEvents(game_.FinishTurn());
      enemy_card_ = tema1::kNoCard;
      BeginTurn();
    }
  }
}

void Tema1::SubmitAction(const Action& action) {
  if (!game_.Commit(tema1::kHumanPlayer, action)) {
    return;
  }

  hovered_card_ = tema1::kNoCard;
  LayoutCards();

  if (action.kind == Action::kPlayCard) {
    message_ = tema1::text::kPlayPrefix +
               std::string(CardName(game_.cards_[action.card].type));
  } else if (action.kind == Action::kUseLeader) {
    message_ = tema1::text::kCallLeader;
  } else {
    message_ = tema1::text::kNoAction;
  }

  SetState(State::kPlayerPlace, ui::kPlayerPlaceDelay);
}

void Tema1::PlayEnemyAction() {
  const Action action = game_.ChooseAction(tema1::kEnemyPlayer);
  game_.Commit(tema1::kEnemyPlayer, action);
  enemy_card_ = action.kind == Action::kPlayCard ? action.card : -1;
  LayoutCards();

  if (!game_.AnyCommitted()) {
    HandleEvents(game_.FinishTurn());
    enemy_card_ = tema1::kNoCard;
    BeginTurn();
    return;
  }

  message_ = enemy_card_ >= 0                    ? tema1::text::kOpponentPlays
             : action.kind == Action::kUseLeader ? tema1::text::kOpponentLeader
                                                 : tema1::text::kNoAction;
  SetState(State::kEnemyPlace, ui::kEnemyPlaceDelay);
}

void Tema1::BeginTurn() {
  if (game_.result_ != GameResult::kPlaying) {
    message_ = game_.result_ == GameResult::kPlayerWin  ? tema1::text::kVictory
               : game_.result_ == GameResult::kEnemyWin ? tema1::text::kDefeat
                                                        : tema1::text::kDraw;
    SetState(State::kOver, 0);
  } else if (game_.round_result_ != GameResult::kPlaying) {
    message_ = game_.round_result_ == GameResult::kPlayerWin
                   ? tema1::text::kRoundWon
               : game_.round_result_ == GameResult::kEnemyWin
                   ? tema1::text::kRoundLost
                   : tema1::text::kRoundDrawn;
    SetState(State::kRoundOver, 0);
  } else if (!game_.HasAction(tema1::kHumanPlayer)) {
    game_.Commit(tema1::kHumanPlayer, Action(Action::kSkip));
    message_ = tema1::text::kNoAction;
    SetState(State::kPlayerPlace, ui::kSkipDelay);
  } else {
    message_ = tema1::text::kYourTurn;
    SetState(State::kReady, 0);
  }
}

void Tema1::HandleEvents(const std::vector<Event>& events) {
  std::vector<int> inspection_cards;

  for (const Event& event : events) {
    if (event.kind == Event::kDestroy) {
      Visual& visual = card_visuals_[event.card];
      visual.dying = true;
      visual.vanish_progress = 0;
    } else if (event.kind == Event::kReturn || event.kind == Event::kRevive) {
      Visual& visual = card_visuals_[event.card];

      if (!visual.initialized) {
        visual.initialized = true;
        visual.bounds = DeckRect(event.owner);
      }

      visual.dying = false;
      visual.vanish_progress = 0;
    } else if (event.kind == Event::kReveal &&
               event.owner == tema1::kHumanPlayer) {
      inspection_cards.push_back(event.card);
    } else if (event.kind == Event::kHit && event.amount > 0) {
      hit_flash_[event.owner] = ui::kHitFlashDuration;
    }
  }

  if (!inspection_cards.empty()) {
    revealed_cards_ = inspection_cards;
    show_inspection_ = true;
  }
}

}  // namespace m1
