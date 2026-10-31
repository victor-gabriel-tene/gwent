#include "lab_m1/Tema1/game_rules.h"

#include <algorithm>
#include <utility>

#include "lab_m1/Tema1/config/deck.h"

namespace tema1 {

Game::Game(unsigned seed) {
  Reset(seed);
}

void Game::RemoveCard(std::vector<int>& list, int id) {
  list.erase(std::remove(list.begin(), list.end(), id), list.end());
}

std::vector<int>& Game::CardsInZone(int owner, CardZone zone) {
  Player& player = players_[owner];

  if (zone == CardZone::kDeck) {
    return player.deck;
  }

  if (zone == CardZone::kHand) {
    return player.hand;
  }

  if (zone == CardZone::kBoard) {
    return player.board;
  }

  return player.discard;
}

int Game::Add(int owner, CardType type, int value, CardZone zone,
              Lane allowed) {
  if (IsWeather(type)) {
    allowed = WeatherLane(type);
  }

  if (type == CardType::kMedic && allowed == Lane::kAny) {
    allowed = Lane::kRanged;
  }

  const int id = static_cast<int>(cards_.size());
  cards_.emplace_back(id, owner, type, value, zone, allowed);
  Card& card = cards_.back();

  if (zone == CardZone::kBoard) {
    card.lane = allowed == Lane::kAny ? Lane::kMelee : allowed;
  }

  card.known = owner == kHumanPlayer || zone == CardZone::kBoard ||
               zone == CardZone::kDiscard;
  CardsInZone(owner, zone).push_back(id);

  return id;
}

void Game::ResetEmpty(unsigned seed) {
  cards_.clear();
  players_ = std::array<Player, kPlayerCount>();

  for (Player& player : players_) {
    player.leader_used = true;
  }

  result_ = round_result_ = GameResult::kPlaying;
  round_scored_ = false;
  round_ = kFirstRound;
  turn_ = kFirstTurn;
  random_.seed(seed);
  horn_turn_.fill(kNoTurn);
  horn_lane_.fill(Lane::kNone);
  ClearTurn();
}

void Game::Reset(unsigned seed, bool deal) {
  ResetEmpty(seed);

  for (int owner = 0; owner < kPlayerCount; ++owner) {
    players_[owner].leader_used = false;
    BuildDeck(owner);
    std::shuffle(players_[owner].deck.begin(), players_[owner].deck.end(),
                 random_);
  }

  std::uniform_int_distribution<int> leader(
      0, static_cast<int>(Leader::kCount) - 1);
  players_[kEnemyPlayer].leader = static_cast<Leader>(leader(random_));

  if (deal) {
    for (int i = 0; i < kOpeningHandSize; ++i) {
      for (int owner = 0; owner < kPlayerCount; ++owner) {
        Draw(owner);
      }
    }
  }
}

void Game::BuildDeck(int owner) {
  int next_lane = 0;

  for (const DeckGroup& group : kDeckRecipe) {
    for (int value = group.first_value; value <= group.last_value; ++value) {
      for (int copy = 0; copy < group.copies_per_value; ++copy) {
        const Lane lane = group.lane == Lane::kNone
                              ? static_cast<Lane>(next_lane++ % kLaneCount)
                              : group.lane;
        Add(owner, group.type, value, CardZone::kDeck, lane);
      }
    }
  }
}

void Game::Move(int id, int owner, CardZone zone) {
  Card& card = cards_.at(id);
  RemoveCard(CardsInZone(card.owner, card.zone), id);
  card.owner = owner;
  card.zone = zone;

  if (zone != CardZone::kBoard) {
    card.lane = Lane::kNone;
    card.suppressed = false;
  }

  // Hand inspection lasts only while that exact card remains in that hand.
  card.known = owner == kHumanPlayer || zone == CardZone::kBoard ||
               zone == CardZone::kDiscard;
  CardsInZone(owner, zone).push_back(id);
}

int Game::Draw(int owner) {
  if (players_[owner].deck.empty()) {
    return kNoCard;
  }

  const int id = players_[owner].deck.back();
  Move(id, owner, CardZone::kHand);

  return id;
}

std::vector<Lane> Game::LegalLanes(int owner, int id) const {
  std::vector<Lane> lanes;

  if (!IsRunning() || !IsValidOwner(owner) || !IsHandCard(owner, id)) {
    return lanes;
  }

  const Card& card = cards_[id];

  if (card.type == CardType::kDecoy) {
    for (int target : players_[owner].board) {
      if (IsOnBoard(target) && std::find(lanes.begin(), lanes.end(),
                                         cards_[target].lane) == lanes.end()) {
        lanes.push_back(cards_[target].lane);
      }
    }

    std::sort(lanes.begin(), lanes.end(), [](Lane action, Lane b) {
      return static_cast<int>(action) < static_cast<int>(b);
    });
  } else if (card.printed_lane == Lane::kAny) {
    lanes = {Lane::kMelee, Lane::kRanged, Lane::kSiege};
  } else if (IsBoardLane(card.printed_lane)) {
    lanes.push_back(card.printed_lane);
  }

  return lanes;
}

bool Game::CanPlay(int owner, int id, Lane lane, int target) const {
  if (lane == Lane::kNone) {
    const auto lanes = LegalLanes(owner, id);

    if (!lanes.empty()) {
      lane = lanes.front();
    }
  }

  return IsValidOwner(owner) && !committed_[owner] &&
         LegalPlacement(owner, id, lane, target);
}

bool Game::CanUseLeader(int owner) const {
  return IsRunning() && IsValidOwner(owner) && !committed_[owner] &&
         !players_[owner].leader_used;
}

bool Game::HasAction(int owner) const {
  if (!IsRunning() || !IsValidOwner(owner)) {
    return false;
  }

  if (!players_[owner].leader_used) {
    return true;
  }

  for (int id : players_[owner].hand) {
    if (!LegalLanes(owner, id).empty()) {
      return true;
    }
  }

  return false;
}

bool Game::CanAct(int owner, const Action& action) const {
  if (!IsRunning() || !IsValidOwner(owner) || committed_[owner]) {
    return false;
  }

  if (action.kind == Action::kSkip) {
    return !HasAction(owner);
  }

  if (action.kind == Action::kUseLeader) {
    return CanUseLeader(owner);
  }

  return action.kind == Action::kPlayCard &&
         LegalPlacement(owner, action.card, action.lane, action.target);
}

bool Game::Commit(int owner, const Action& requested) {
  if (!CanAct(owner, requested)) {
    return false;
  }

  Action action = requested;
  committed_[owner] = true;
  actions_[owner] = action;

  if (action.kind == Action::kPlayCard) {
    Card& card = cards_[action.card];

    if (horn_turn_[owner] == turn_ &&
        (horn_lane_[owner] == Lane::kAny || horn_lane_[owner] == action.lane)) {
      card.value *= kValueMultiplier;
    }

    Move(action.card, owner, CardZone::kBoard);
    card.lane = action.lane;
    card.suppressed = false;
    played_[owner] = action.card;
    targets_[owner] = action.target;
  } else if (action.kind == Action::kUseLeader) {
    players_[owner].leader_used = true;
  }

  if (horn_turn_[owner] == turn_) {
    horn_turn_[owner] = kNoTurn;
    horn_lane_[owner] = Lane::kNone;
  }

  return true;
}

bool Game::Play(int owner, int id, Lane lane, int target) {
  if (lane == Lane::kNone) {
    const auto lanes = LegalLanes(owner, id);

    if (!lanes.empty()) {
      lane = lanes.front();
    }
  }

  return Commit(owner, Action(Action::kPlayCard, id, lane, target));
}

bool Game::AnyCommitted() const {
  for (int owner = 0; owner < kPlayerCount; ++owner) {
    if (committed_[owner] && (actions_[owner].kind == Action::kPlayCard ||
                              actions_[owner].kind == Action::kUseLeader)) {
      return true;
    }
  }

  return false;
}

bool Game::HasCommittedAction(int owner) const {
  return IsValidOwner(owner) && committed_[owner];
}

Action Game::ChooseAction(int owner) {
  if (!IsRunning() || !IsValidOwner(owner) || committed_[owner]) {
    return Action();
  }

  std::vector<Action> choices;

  for (int id : players_[owner].hand) {
    if (!LegalLanes(owner, id).empty()) {
      choices.emplace_back(Action::kPlayCard, id);
    }
  }

  // A normal response always plays a random legal card. Leaders are a fallback.
  if (choices.empty()) {
    return CanUseLeader(owner) ? Action(Action::kUseLeader)
                               : Action(Action::kSkip);
  }

  Action action = choices[PickIndex(static_cast<int>(choices.size()))];

  if (action.kind == Action::kPlayCard) {
    const auto lanes = LegalLanes(owner, action.card);
    action.lane = lanes[PickIndex(static_cast<int>(lanes.size()))];

    if (cards_[action.card].type == CardType::kDecoy) {
      std::vector<int> candidates;

      for (int id : players_[owner].board) {
        if (CanPlay(owner, action.card, action.lane, id)) {
          candidates.push_back(id);
        }
      }

      action.target =
          candidates[PickIndex(static_cast<int>(candidates.size()))];
    }
  }

  return action;
}

int Game::EffectiveValue(int id) const {
  const Card& card = cards_.at(id);

  return card.value * (card.zone == CardZone::kBoard &&
                               card.lane == Lane::kMelee &&
                               melee_boost_[card.owner]
                           ? kValueMultiplier
                           : 1);
}

void Game::CheckLethal() {
  if (!IsRunning()) {
    return;
  }

  if (players_[kHumanPlayer].hp <= 0 && players_[kEnemyPlayer].hp <= 0) {
    EndRound(GameResult::kDraw);
  } else if (players_[kHumanPlayer].hp <= 0) {
    EndRound(GameResult::kEnemyWin);
  } else if (players_[kEnemyPlayer].hp <= 0) {
    EndRound(GameResult::kPlayerWin);
  }
}

std::vector<Event> Game::FinishTurn() {
  std::vector<Event> events;
  const bool was_playing = round_result_ == GameResult::kPlaying;
  CheckLethal();
  const bool empty_hands = players_[kHumanPlayer].hand.empty() &&
                           players_[kEnemyPlayer].hand.empty();

  if (IsRunning() &&
      (empty_hands || (!HasAction(kHumanPlayer) && !HasAction(kEnemyPlayer)))) {
    EndRound(players_[kHumanPlayer].hp == players_[kEnemyPlayer].hp
                 ? GameResult::kDraw
             : players_[kHumanPlayer].hp > players_[kEnemyPlayer].hp
                 ? GameResult::kPlayerWin
                 : GameResult::kEnemyWin);
  }

  ScoreRound();

  if (was_playing && round_result_ != GameResult::kPlaying) {
    events.emplace_back(Event::kRoundEnd, kNoOwner, kNoCard,
                        static_cast<int>(round_result_));
  }

  if (IsRunning()) {
    ++turn_;
  }

  for (int owner = 0; owner < kPlayerCount; ++owner) {
    if (horn_turn_[owner] < turn_) {
      horn_turn_[owner] = kNoTurn;
      horn_lane_[owner] = Lane::kNone;
    }
  }

  ClearTurn();

  return events;
}

bool Game::NextRound(bool deal) {
  if (result_ != GameResult::kPlaying ||
      round_result_ == GameResult::kPlaying || !round_scored_ ||
      round_ >= kMaxRounds) {
    return false;
  }

  for (int owner = 0; owner < kPlayerCount; ++owner) {
    const auto board = players_[owner].board;

    for (int id : board) {
      Move(id, owner, CardZone::kDiscard);
    }

    players_[owner].hp = kMaxHp;
    players_[owner].shield = 0;
  }

  ++round_;
  turn_ = kFirstTurn;
  round_result_ = GameResult::kPlaying;
  round_scored_ = false;
  horn_turn_.fill(kNoTurn);
  horn_lane_.fill(Lane::kNone);
  ClearTurn();

  if (deal) {
    for (int i = 0; i < kCardsPerRound; ++i) {
      for (int owner = 0; owner < kPlayerCount; ++owner) {
        Draw(owner);
      }
    }
  }

  return true;
}

bool Game::HasValidState() const {
  if (round_ < kFirstRound || round_ > kMaxRounds || turn_ < kFirstTurn) {
    return false;
  }

  std::vector<int> seen(cards_.size(), 0);

  for (int owner = 0; owner < kPlayerCount; ++owner) {
    const Player& player = players_[owner];

    if (player.hp < 0 || player.hp > kMaxHp || player.shield < 0 ||
        player.crystals < 0 || player.crystals > kStartingCrystals) {
      return false;
    }

    const std::vector<int>* lists[] = {&player.deck, &player.hand,
                                       &player.board, &player.discard};

    for (CardZone zone : {CardZone::kDeck, CardZone::kHand, CardZone::kBoard,
                          CardZone::kDiscard}) {
      for (int id : *lists[static_cast<int>(zone)]) {
        if (!IsValidCard(id) || ++seen[id] != 1) {
          return false;
        }

        const Card& card = cards_[id];

        if (card.id != id || card.owner != owner || card.zone != zone) {
          return false;
        }

        if (zone == CardZone::kBoard) {
          if (card.value <= 0 || !IsBoardLane(card.lane)) {
            return false;
          }

          if (card.printed_lane != Lane::kAny &&
              card.lane != card.printed_lane) {
            return false;
          }
        } else if (card.lane != Lane::kNone || card.suppressed) {
          return false;
        }
      }
    }
  }

  return std::all_of(seen.begin(), seen.end(), [](int n) { return n == 1; });
}

bool Game::IsValidOwner(int owner) {
  return owner == kHumanPlayer || owner == kEnemyPlayer;
}

bool Game::IsBoardLane(Lane lane) {
  return lane == Lane::kMelee || lane == Lane::kRanged || lane == Lane::kSiege;
}

bool Game::IsValidCard(int id) const {
  return id >= 0 && id < static_cast<int>(cards_.size());
}

bool Game::IsOnBoard(int id) const {
  return IsValidCard(id) && cards_[id].zone == CardZone::kBoard &&
         cards_[id].value > 0;
}

bool Game::IsHandCard(int owner, int id) const {
  return IsValidCard(id) && cards_[id].owner == owner &&
         cards_[id].zone == CardZone::kHand;
}

bool Game::IsRunning() const {
  return result_ == GameResult::kPlaying &&
         round_result_ == GameResult::kPlaying;
}

int Game::PickIndex(int size) {
  std::uniform_int_distribution<int> pick(0, size - 1);

  return pick(random_);
}

bool Game::LegalPlacement(int owner, int id, Lane lane, int target) const {
  if (!IsBoardLane(lane)) {
    return false;
  }

  const auto lanes = LegalLanes(owner, id);

  if (lanes.empty()) {
    return false;
  }

  if (std::find(lanes.begin(), lanes.end(), lane) == lanes.end()) {
    return false;
  }

  if (cards_[id].type == CardType::kDecoy) {
    return IsOnBoard(target) && cards_[target].owner == owner &&
           cards_[target].lane == lane;
  }

  return true;
}

void Game::ClearTurn() {
  actions_ = std::array<Action, kPlayerCount>();
  committed_.fill(false);
  utility_used_.fill(false);
  played_.fill(kNoCard);
  targets_.fill(kNoCard);
  melee_boost_.fill(false);

  for (Card& card : cards_) {
    card.suppressed = false;
  }
}

void Game::EndRound(GameResult outcome) {
  if (!IsRunning()) {
    return;
  }

  round_result_ = outcome;
}

void Game::ScoreRound() {
  if (round_result_ == GameResult::kPlaying || round_scored_) {
    return;
  }

  round_scored_ = true;

  if (round_result_ == GameResult::kEnemyWin ||
      round_result_ == GameResult::kDraw) {
    players_[kHumanPlayer].crystals -= kCrystalsPerLoss;
  }

  if (round_result_ == GameResult::kPlayerWin ||
      round_result_ == GameResult::kDraw) {
    players_[kEnemyPlayer].crystals -= kCrystalsPerLoss;
  }

  if (players_[kHumanPlayer].crystals == 0 &&
      players_[kEnemyPlayer].crystals == 0) {
    result_ = GameResult::kDraw;
  } else if (players_[kHumanPlayer].crystals == 0) {
    result_ = GameResult::kEnemyWin;
  } else if (players_[kEnemyPlayer].crystals == 0) {
    result_ = GameResult::kPlayerWin;
  }
}

}  // namespace tema1
