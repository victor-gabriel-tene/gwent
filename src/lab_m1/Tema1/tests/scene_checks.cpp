#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lab_m1/Tema1/Tema1.h"
#include "lab_m1/Tema1/config/scene.h"

namespace m1 {
namespace {
using tema1::Action;
using tema1::CardType;
using tema1::CardZone;
using tema1::Game;
using tema1::GameResult;
using tema1::Lane;
using tema1::Leader;
using tema1::ui::kCanvasHeight;
using tema1::ui::kCanvasWidth;
using tema1::ui::kLanePadding;
}  // namespace

void Tema1::VerifyScene(const char* directory) {
  auto expect = [](bool valid, const char* message_) {
    if (!valid) {
      throw std::runtime_error(std::string("Tema1 scene check: ") + message_);
    }
  };
  auto pixel = [this](glm::vec2 logical) {
    int width, height;
    glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

    return glm::ivec2(
        std::lround((viewport_x_ + logical.x * viewport_width_ / kCanvasWidth) *
                    width / framebuffer_width_),
        std::lround((framebuffer_height_ - viewport_y_ -
                     logical.y * viewport_height_ / kCanvasHeight) *
                    height / framebuffer_height_));
  };
  auto capture = [this, directory](const char* name) {
    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(framebuffer_width_) * framebuffer_height_ * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, framebuffer_width_, framebuffer_height_, GL_RGB,
                 GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream file(std::string(directory) + "/" + name + ".ppm",
                       std::ios::binary);
    file << "P6\n"
         << framebuffer_width_ << " " << framebuffer_height_ << "\n255\n";

    for (int y = framebuffer_height_ - 1; y >= 0; --y) {
      file.write(reinterpret_cast<const char*>(
                     pixels.data() + std::size_t(y) * framebuffer_width_ * 3),
                 framebuffer_width_ * 3);
    }

    if (!file) {
      throw std::runtime_error("Cannot save scene verification capture");
    }
  };
  auto click = [this, &pixel](Rect r) {
    const auto p = pixel(glm::vec2(r.x + r.width / 2, r.y + r.height / 2));
    OnMouseBtnPress(p.x, p.y, 1, 0);
  };
  auto play = [this, &pixel, &expect, &capture](int id, glm::vec2 destination) {
    const Rect r = card_visuals_.at(id).bounds;
    const auto from = pixel(glm::vec2(r.x + r.width / 2, r.y + r.height / 2));
    const auto to = pixel(destination);
    OnMouseBtnPress(from.x, from.y, 1, 0);
    expect(dragged_card_ == id, "hand rectangle selects expected card");
    OnMouseMove(to.x, to.y, 0, 0);
    expect(glm::length(glm::vec2(card_visuals_[id].bounds.x,
                                 card_visuals_[id].bounds.y) +
                       grab_offset_ - mouse_position_) < 1,
           "drag preserves grab offset");

    if (verification_stage_ == 4 || verification_stage_ == 9) {
      RenderScene();
      capture(verification_stage_ == 4 ? "04b-lane-highlight"
                                       : "09b-decoy-target-highlight");
    }

    OnMouseBtnRelease(to.x, to.y, 1, 0);
  };
  auto center = [](Rect r) {
    return glm::vec2(r.x + r.width / 2, r.y + r.height / 2);
  };
  auto ready = [this]() {
    return state_ == State::kReady && state_elapsed_ > 0.35f;
  };
  auto settle_fixture = [this]() {
    card_visuals_.clear();
    revealed_cards_.clear();
    show_inspection_ = show_rules_ = false;
    enemy_card_ = dragged_card_ = hovered_card_ = -1;
    phase_index_ = 0;
    hit_flash_ = {{0, 0}};
    LayoutCards();
    AnimateCards(1);
    SetState(State::kReady, 0);
  };
  auto winning_play = [this, &settle_fixture, &play, &center]() {
    game_.players_[tema1::kEnemyPlayer].leader_used = true;
    game_.players_[tema1::kEnemyPlayer].shield = 0;
    game_.Add(0, CardType::kDamage, 60, CardZone::kBoard, Lane::kMelee);
    const int id =
        game_.Add(0, CardType::kDamage, 1, CardZone::kHand, Lane::kSiege);
    settle_fixture();
    play(id, center(LaneRect(0, Lane::kSiege)));
  };
  expect(game_.HasValidState(),
         "ownership and lane invariant after scene update");
  expect(elapsed_time_ < 180, "scene verification watchdog");

  if (verification_stage_ == 0 && state_ == State::kSelectLeader) {
    capture("01-leader-menu");
    click(LeaderChoiceRect(1));
    expect(selected_leader_ == 1 && state_ == State::kSelectLeader,
           "first click selects without starting");
    ++verification_stage_;
  } else if (verification_stage_ == 1) {
    capture("02-leader-highlight");
    click(LeaderChoiceRect(1));
    expect(state_ == State::kDeal &&
               game_.players_[tema1::kHumanPlayer].leader == Leader::kEmhyr,
           "second click starts selected leader");
    // Leader randomness is covered in the rule suite; this turn checks a card
    // response.
    game_.players_[tema1::kEnemyPlayer].leader_used = true;
    ++verification_stage_;
  } else if (verification_stage_ == 2 && ready()) {
    expect(game_.players_[tema1::kHumanPlayer].hand.size() == 10 &&
               game_.players_[tema1::kEnemyPlayer].hand.size() == 10,
           "opening hand sizes");
    // Choose the response seed only after the enemy has received its opening
    // hand.
    bool found = false;

    for (unsigned seed = 0; seed < 100; ++seed) {
      Game probe = game_;
      probe.random_.seed(seed);

      if (probe.ChooseAction(1).kind == Action::kPlayCard) {
        game_.random_.seed(seed);
        found = true;
        break;
      }
    }

    expect(found, "seeded opening enemy has a legal card response");

    for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
      const Rect hand = HandRect(owner);
      const auto& ids = game_.players_[owner].hand;
      const Rect first = card_visuals_[ids.front()].bounds,
                 last = card_visuals_[ids.back()].bounds;
      expect(std::abs((first.x + last.x + last.width) / 2 - kCanvasWidth / 2) <
                 0.1f,
             "hand group centered on screen");

      for (int id : ids) {
        const Rect c = card_visuals_[id].bounds;
        expect(c.x >= hand.x && c.x + c.width <= hand.x + hand.width &&
                   c.y >= hand.y && c.y + c.height <= hand.y + hand.height,
               "hand cards contained with window inset");
      }

      expect(hand.y >= 24 && hand.y + hand.height <= kCanvasHeight - 24,
             "hands clear top and bottom edges");
    }

    const Rect
        origin = card_visuals_[game_.players_[tema1::kEnemyPlayer].hand.back()]
                     .start_bounds,
        deck = DeckRect(1);
    expect(std::abs(origin.x - deck.x) < 0.1f &&
               std::abs(origin.y - deck.y) < 0.1f &&
               std::abs(origin.width - deck.width) < 0.1f,
           "draw animation starts at shared dealing origin");
    const int before = game_.turn_;
    OnKeyPress(GLFW_KEY_P, 0);
    expect(state_ == State::kReady && game_.turn_ == before &&
               !game_.HasCommittedAction(tema1::kHumanPlayer),
           "P has no action");

    for (glm::vec2 logical :
         {glm::vec2(0), glm::vec2(800, 325), glm::vec2(1600, 900)}) {
      const auto p = pixel(logical);
      expect(glm::length(ToLogicalPosition(p.x, p.y) - logical) < 3,
             "Retina coordinate round trip");
    }

    for (int id : game_.players_[tema1::kHumanPlayer].hand) {
      if (IsUnit(game_.cards_[id].type)) {
        verification_card_ = id;
        break;
      }
    }

    expect(verification_card_ >= 0, "opening fixture has a unit");
    verification_hand_ =
        static_cast<int>(game_.players_[tema1::kHumanPlayer].hand.size());
    capture("03-opening");
    const glm::vec2 saved_mouse = mouse_position_;
    mouse_position_ = center(RulesRect());
    RenderScene();
    capture("03e-rules-button-hover");
    mouse_position_ = center(LeaderRect(0));
    RenderScene();
    capture("03a-leader-hover");
    mouse_position_ = saved_mouse;
    const int saved_shield = game_.players_[tema1::kHumanPlayer].shield;
    game_.players_[tema1::kHumanPlayer].shield = 5;
    RenderScene();
    capture("03c-shield-partial");
    game_.players_[tema1::kHumanPlayer].shield = 35;
    RenderScene();
    capture("03d-shield-overflow");
    game_.players_[tema1::kHumanPlayer].shield = saved_shield;
    hovered_card_ = verification_card_;
    RenderScene();
    capture("03b-hover-preview");
    hovered_card_ = -1;
    play(verification_card_,
         glm::vec2(LaneRect(0, Lane::kMelee).x +
                       LaneRect(0, Lane::kMelee).width + 100,
                   450));
    expect(state_ == State::kReturn &&
               game_.cards_[verification_card_].zone == CardZone::kHand,
           "outside drop returns to hand");
    ++verification_stage_;
  } else if (verification_stage_ == 3 && ready()) {
    expect(
        game_.turn_ == 1 && game_.players_[tema1::kHumanPlayer].hand.size() ==
                                static_cast<std::size_t>(verification_hand_),
        "invalid drop spends no action");
    const Lane wrong = static_cast<Lane>(
        (static_cast<int>(game_.cards_[verification_card_].printed_lane) + 1) %
        3);
    play(verification_card_, center(LaneRect(0, wrong)));
    expect(state_ == State::kReturn &&
               game_.cards_[verification_card_].zone == CardZone::kHand,
           "wrong lane returns to hand");
    ++verification_stage_;
  } else if (verification_stage_ == 4 && ready()) {
    capture("04-invalid-return");
    const Lane lane = game_.cards_[verification_card_].printed_lane;
    // The 4px lane gap is invalid, as are the opposing half and letterbox
    // margins.
    const Rect melee = LaneRect(0, Lane::kMelee),
               ranged = LaneRect(0, Lane::kRanged);
    expect(
        !game_.CanAct(
            0, DropAction(verification_card_,
                          glm::vec2(center(melee).x,
                                    (melee.y + ranged.y + ranged.height) / 2))),
        "lane gap rejected");
    expect(!game_.CanAct(
               0, DropAction(verification_card_, center(LaneRect(1, lane)))),
           "enemy lane rejected");
    expect(
        !game_.CanAct(0, DropAction(verification_card_, glm::vec2(-10, -10))),
        "letterbox point rejected");
    play(verification_card_, center(LaneRect(0, lane)));
    expect(state_ == State::kPlayerPlace &&
               game_.cards_[verification_card_].lane == lane,
           "valid lane placement commits once");
    ++verification_stage_;
  } else if (verification_stage_ == 5 && ready() && game_.turn_ == 2) {
    expect(verification_hand_ == -1,
           "enemy rotating reveal observed before resolution");
    capture("05-after-turn");
    OnKeyPress(GLFW_KEY_H, 0);
    expect(show_rules_, "rules overlay opens");
    ++verification_stage_;
  } else if (verification_stage_ == 6) {
    capture("06-rules");
    click(RulesRect());
    expect(!show_rules_, "click closes rules");
    click(RulesRect());
    expect(show_rules_, "shared rules rectangle opens overlay");
    OnKeyPress(GLFW_KEY_H, 0);
    glfwSetWindowSize(glfwGetCurrentContext(), 1000, 800);
    ++verification_stage_;
  } else if (verification_stage_ == 7) {
    for (int row = 0; row < tema1::kLaneCount; ++row) {
      const glm::vec2 logical = center(LaneRect(0, static_cast<Lane>(row)));
      const auto p = pixel(logical);
      expect(glm::length(ToLogicalPosition(p.x, p.y) - logical) < 3,
             "resized lane pointer mapping");
    }

    capture("07-resized");
    OnKeyPress(GLFW_KEY_H, 0);
    RenderScene();
    capture("07c-resized-rules");
    OnKeyPress(GLFW_KEY_H, 0);
    hovered_card_ = game_.players_[tema1::kHumanPlayer].hand.front();
    RenderScene();
    capture("07b-resized-preview");
    hovered_card_ = -1;
    game_.ResetEmpty(123);

    for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
      for (int n = 0; n < 24; ++n) {
        game_.Add(owner, static_cast<CardType>(n % 3), n % 3 + 1,
                  CardZone::kBoard, Lane::kMelee);
      }
    }

    game_.Add(0, CardType::kClear, 1, CardZone::kHand);
    settle_fixture();
    ++verification_stage_;
  } else if (verification_stage_ == 8) {
    capture("08-crowded-lane");

    for (int owner = 0; owner < tema1::kPlayerCount; ++owner) {
      const Rect r = LaneRect(owner, Lane::kMelee);

      for (int id : game_.players_[owner].board) {
        const Rect c = card_visuals_[id].bounds;
        expect(c.x >= r.x + kLanePadding &&
                   c.x + c.width <= r.x + r.width - kLanePadding &&
                   c.y >= r.y && c.y + c.height <= r.y + r.height,
               "crowded lane cards clear labels and keep equal side margins");
      }

      const auto& ids = game_.players_[owner].board;
      const Rect first = card_visuals_[ids.front()].bounds,
                 last = card_visuals_[ids.back()].bounds;
      expect(std::abs((first.x + last.x + last.width) / 2 - kCanvasWidth / 2) <
                 0.1f,
             "crowded lane centered on screen");
    }

    glfwSetWindowSize(glfwGetCurrentContext(), 1280, 720);
    game_.ResetEmpty(321);
    game_.players_[tema1::kHumanPlayer].leader = Leader::kEmhyr;
    game_.players_[tema1::kHumanPlayer].leader_used = false;
    game_.players_[tema1::kEnemyPlayer].leader = Leader::kFrancesca;
    game_.players_[tema1::kEnemyPlayer].leader_used = true;
    verification_card_ =
        game_.Add(0, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
    game_.Add(0, CardType::kShield, 2, CardZone::kBoard, Lane::kSiege);
    game_.Add(0, CardType::kMedic, 2, CardZone::kBoard, Lane::kRanged);
    game_.Add(1, CardType::kDamage, 1, CardZone::kBoard, Lane::kMelee);
    game_.Add(1, CardType::kHeal, 1, CardZone::kBoard, Lane::kRanged);

    for (CardType type :
         {CardType::kDecoy, CardType::kMedic, CardType::kFrost, CardType::kFog,
          CardType::kRain, CardType::kHorn, CardType::kScorch, CardType::kClear,
          CardType::kSpy}) {
      game_.Add(0, type,
                type == CardType::kClear || type == CardType::kDecoy ? 1 : 2,
                CardZone::kHand);
    }

    for (int i = 0; i < 6; ++i) {
      game_.Add(1, CardType::kShield, 1, CardZone::kHand, Lane::kSiege);
    }

    settle_fixture();
    ++verification_stage_;
  } else if (verification_stage_ == 9 && ready()) {
    capture("09-advanced-cards");
    hovered_card_ = verification_card_;
    RenderScene();
    capture("09c-unit-hover-preview");
    hovered_card_ = -1;
    int decoy = -1;

    for (int id : game_.players_[tema1::kHumanPlayer].hand) {
      if (game_.cards_[id].type == CardType::kDecoy) {
        decoy = id;
      }
    }

    expect(decoy >= 0, "Decoy fixture available");
    play(decoy, center(card_visuals_[verification_card_].bounds));
    expect(game_.cards_[verification_card_].zone == CardZone::kBoard,
           "Decoy waits until utility after Scorch");
    ++verification_stage_;
  } else if (verification_stage_ == 10 && ready()) {
    expect(game_.cards_[verification_card_].zone == CardZone::kHand &&
               game_.cards_[verification_card_].value == 3,
           "Decoy returns unit at printed V");
    capture("10-decoy-return");
    verification_hand_ = game_.Add(0, CardType::kRain, 2, CardZone::kDiscard);
    game_.cards_[verification_hand_].value = 0;
    int medic = -1;

    for (int id : game_.players_[tema1::kHumanPlayer].hand) {
      if (game_.cards_[id].type == CardType::kMedic) {
        medic = id;
      }
    }

    expect(medic >= 0, "Medic fixture available");
    play(medic, center(LaneRect(0, Lane::kRanged)));
    ++verification_stage_;
  } else if (verification_stage_ == 11 && ready()) {
    expect(game_.cards_[verification_hand_].zone == CardZone::kHand &&
               game_.cards_[verification_hand_].value == 2,
           "Medic revives a special at printed V");
    capture("11-medic-recovery");
    click(LeaderRect(0));
    expect(game_.players_[tema1::kHumanPlayer].leader_used &&
               state_ == State::kPlayerPlace,
           "leader click spends one action");
    ++verification_stage_;
  } else if (verification_stage_ == 12 && show_inspection_) {
    expect(revealed_cards_.size() == 3,
           "Emhyr reveals three distinct enemy cards");
    capture("12-emhyr-inspection");
    const int before = game_.turn_;
    click(
        card_visuals_[game_.players_[tema1::kHumanPlayer].hand.front()].bounds);
    expect(show_inspection_ && game_.turn_ == before && dragged_card_ < 0,
           "inspection blocks card selection");
    click(PopupButtonRect());
    expect(!show_inspection_, "inspection Continue closes modal");
    ++verification_stage_;
  } else if (verification_stage_ == 13 && ready()) {
    capture("13-used-leader-known-hand");
    int known = 0;

    for (int id : game_.players_[tema1::kEnemyPlayer].hand) {
      if (game_.cards_[id].known) {
        ++known;
      }
    }

    expect(known == 3, "revealed cards remain face up in hand");
    click(LeaderRect(0));
    expect(state_ == State::kReady &&
               !game_.HasCommittedAction(tema1::kHumanPlayer),
           "used leader cannot spend another action");
    game_.Add(0, CardType::kHeal, 2, CardZone::kDeck, Lane::kSiege);
    game_.Add(1, CardType::kShield, 2, CardZone::kDeck, Lane::kSiege);
    winning_play();
    ++verification_stage_;
  } else if (verification_stage_ == 14 && state_ == State::kRoundOver) {
    capture("14-round-intermission");
    verification_hand_ =
        static_cast<int>(game_.players_[tema1::kHumanPlayer].hand.size());
    verification_card_ =
        static_cast<int>(game_.players_[tema1::kEnemyPlayer].hand.size());
    click(PopupButtonRect());
    expect(state_ == State::kDeal && game_.round_ == 2,
           "Next round begins one-card deal");
    ++verification_stage_;
  } else if (verification_stage_ == 15 && ready()) {
    expect(game_.players_[tema1::kHumanPlayer].hand.size() ==
                   static_cast<std::size_t>(verification_hand_ + 1) &&
               game_.players_[tema1::kEnemyPlayer].hand.size() ==
                   static_cast<std::size_t>(verification_card_ + 1),
           "exactly one card each in round 2");
    expect(game_.players_[tema1::kHumanPlayer].leader_used &&
               game_.players_[tema1::kHumanPlayer].board.empty() &&
               game_.players_[tema1::kHumanPlayer].hp == 30 &&
               game_.players_[tema1::kHumanPlayer].shield == 0,
           "round resets battlefield/health but preserves leader use");
    capture("15-round-two");
    // Exercise the complete three-round controller deterministically.
    game_.players_[tema1::kHumanPlayer].crystals = 1;
    game_.players_[tema1::kEnemyPlayer].crystals = 2;
    game_.Add(0, CardType::kDamage, 2, CardZone::kDeck, Lane::kMelee);
    game_.Add(1, CardType::kHeal, 2, CardZone::kDeck, Lane::kRanged);
    winning_play();
    ++verification_stage_;
  } else if (verification_stage_ == 16 && state_ == State::kRoundOver) {
    expect(game_.players_[tema1::kHumanPlayer].crystals == 1 &&
               game_.players_[tema1::kEnemyPlayer].crystals == 1,
           "round 2 result scores one crystal");
    capture("16-round-two-result");
    verification_hand_ =
        static_cast<int>(game_.players_[tema1::kHumanPlayer].hand.size());
    verification_card_ =
        static_cast<int>(game_.players_[tema1::kEnemyPlayer].hand.size());
    click(PopupButtonRect());
    ++verification_stage_;
  } else if (verification_stage_ == 17 && ready()) {
    expect(game_.round_ == 3 &&
               game_.players_[tema1::kHumanPlayer].hand.size() ==
                   static_cast<std::size_t>(verification_hand_ + 1) &&
               game_.players_[tema1::kEnemyPlayer].hand.size() ==
                   static_cast<std::size_t>(verification_card_ + 1),
           "exactly one card each in round 3");
    capture("17-round-three");
    winning_play();
    ++verification_stage_;
  } else if (verification_stage_ == 18 && state_ == State::kOver) {
    expect(game_.result_ == GameResult::kPlayerWin &&
               game_.players_[tema1::kEnemyPlayer].crystals == 0,
           "last crystal ends match");
    capture("18-match-result");
    OnKeyPress(GLFW_KEY_R, 0);
    expect(state_ == State::kSelectLeader &&
               game_.players_[tema1::kHumanPlayer].hand.empty(),
           "Play again returns to leader menu");
    ++verification_stage_;
  } else if (verification_stage_ == 19) {
    capture("19-restarted-menu");
    click(LeaderChoiceRect(0));
    click(LeaderChoiceRect(0));
    ++verification_stage_;
  } else if (verification_stage_ == 20 && ready()) {
    expect(game_.round_ == 1 && game_.turn_ == 1 &&
               game_.players_[tema1::kHumanPlayer].hand.size() == 10 &&
               game_.players_[tema1::kEnemyPlayer].hand.size() == 10 &&
               game_.players_[tema1::kHumanPlayer].discard.empty() &&
               !game_.players_[tema1::kHumanPlayer].leader_used &&
               game_.players_[tema1::kHumanPlayer].crystals == 2,
           "fresh match resets all match state");
    capture("20-restarted-opening");
    game_.ResetEmpty(456);
    verification_card_ = game_.Add(0, CardType::kFog, 2, CardZone::kBoard);
    verification_hand_ = game_.Add(0, CardType::kDecoy, 1, CardZone::kHand);
    game_.Add(0, CardType::kDamage, 1, CardZone::kHand, Lane::kMelee);
    settle_fixture();
    ++verification_stage_;
  } else if (verification_stage_ == 21 && ready()) {
    const Action drop = DropAction(
        verification_hand_, center(card_visuals_[verification_card_].bounds));
    expect(drop.target == verification_card_ && game_.CanAct(0, drop),
           "Decoy hit test accepts friendly weather");
    play(verification_hand_, center(card_visuals_[verification_card_].bounds));
    expect(state_ == State::kPlayerPlace, "Decoy commits on a special card");
    ++verification_stage_;
  } else if (verification_stage_ == 22 && ready()) {
    expect(game_.cards_[verification_card_].zone == CardZone::kHand &&
               game_.cards_[verification_card_].value == 2 &&
               game_.cards_[verification_hand_].zone == CardZone::kBoard &&
               game_.cards_[verification_hand_].lane == Lane::kRanged,
           "Decoy replaces friendly weather and restores it to hand");
    capture("21-decoy-special-return");
    // Reproduce the reported first-round loss, then equal-HP match loss.
    game_.ResetEmpty(789);
    game_.players_[tema1::kHumanPlayer].hp = 0;
    game_.players_[tema1::kEnemyPlayer].hp = 5;
    game_.FinishTurn();
    BeginTurn();
    expect(state_ == State::kRoundOver &&
               game_.players_[tema1::kHumanPlayer].crystals == 1 &&
               game_.players_[tema1::kEnemyPlayer].crystals == 2,
           "reported first round loses one crystal");
    RenderScene();
    capture("22-round-lost");
    click(PopupButtonRect());
    expect(state_ == State::kDeal, "shared popup button starts next round");
    ++verification_stage_;
  } else if (verification_stage_ == 23 && state_ == State::kOver) {
    expect(game_.round_result_ == GameResult::kDraw &&
               game_.result_ == GameResult::kEnemyWin &&
               game_.players_[tema1::kHumanPlayer].hp == 30 &&
               game_.players_[tema1::kEnemyPlayer].hp == 30 &&
               game_.players_[tema1::kHumanPlayer].crystals == 0 &&
               game_.players_[tema1::kEnemyPlayer].crystals == 1,
           "equal HP draws round but zero crystals loses match");
    capture("23-equal-hp-match-loss");
    glfwSetWindowSize(glfwGetCurrentContext(), 1000, 800);
    ++verification_stage_;
  } else if (verification_stage_ == 24) {
    capture("24-resized-match-loss");
    click(PopupButtonRect());
    expect(state_ == State::kSelectLeader,
           "shared popup button restarts match");
    game_.ResetEmpty();
    game_.FinishTurn();
    BeginTurn();
    expect(
        state_ == State::kRoundOver && game_.round_result_ == GameResult::kDraw,
        "equal opening crystals keep a drawn round in play");
    RenderScene();
    capture("25-round-drawn");
    game_.ResetEmpty();
    game_.players_[tema1::kHumanPlayer].crystals =
        game_.players_[tema1::kEnemyPlayer].crystals = 1;
    game_.FinishTurn();
    BeginTurn();
    expect(state_ == State::kOver && game_.result_ == GameResult::kDraw,
           "both last crystals lost draws match");
    RenderScene();
    capture("26-match-drawn");
    game_.ResetEmpty();
    game_.players_[tema1::kEnemyPlayer].crystals = 1;
    game_.FinishTurn();
    BeginTurn();
    expect(state_ == State::kOver && game_.result_ == GameResult::kPlayerWin,
           "crystal lead wins match after equal HP draw");
    RenderScene();
    capture("27-drawn-round-match-win");
    std::cout
        << "PASS: advanced scene checks (leader selection/highlight, lanes, "
           "invalid/valid drag, crowded layout, "
           "Medic/Decoy including special targets, leader inspection/use, "
           "help, Retina/resize, three rounds, "
           "crystals, restart, result popups and equal-HP match outcomes)\n"
        << std::flush;
    ++verification_stage_;
    window->Close();
  }

  if (verification_stage_ == 5 && state_ == State::kReveal &&
      verification_hand_ != -1) {
    expect(enemy_card_ >= 0 &&
               game_.cards_[enemy_card_].zone == CardZone::kBoard &&
               game_.turn_ == 1,
           "enemy card is committed and rotates before the resolution cycle");
    state_elapsed_ = 0.20f;
    RenderScene();
    capture("04c-enemy-reveal");
    verification_hand_ = -1;
  }

  // Keep actual animation time, but avoid waiting through already-settled phase
  // gaps.
  if (state_ != State::kSelectLeader && state_ != State::kReady &&
      state_ != State::kOver && state_ != State::kRoundOver &&
      state_ != State::kReveal && !show_rules_ && !show_inspection_) {
    bool moving = false;

    for (const auto& entry : card_visuals_) {
      if (!entry.second.dying && entry.second.elapsed < entry.second.duration) {
        moving = true;
      }
    }

    if (!moving) {
      state_elapsed_ = state_duration_;
    }
  }
}

}  // namespace m1
