#include <algorithm>
#include <utility>

#include "lab_m1/Tema1/game_rules.h"

namespace tema1 {

std::vector<int> Game::Sources(CardType type) const {
  std::vector<int> ids;

  for (const Player& player : players_) {
    for (int id : player.board) {
      if (cards_[id].type == type && cards_[id].value > 0) {
        ids.push_back(id);
      }
    }
  }

  return ids;
}

void Game::Destroy(int id, std::vector<Event>& events) {
  if (!IsValidCard(id) || cards_[id].zone != CardZone::kBoard) {
    return;
  }

  const int owner = cards_[id].owner;
  Move(id, owner, CardZone::kDiscard);
  events.emplace_back(Event::kDestroy, owner, id, 0);
}

void Game::Decay(const std::vector<int>& sources, std::vector<Event>& events) {
  for (int id : sources) {
    if (cards_[id].zone == CardZone::kBoard) {
      cards_[id].value -= kTemporaryDecay;
      events.emplace_back(Event::kValue, cards_[id].owner, id,
                          -kTemporaryDecay);

      if (cards_[id].value <= 0) {
        Destroy(id, events);
      }
    }
  }
}

std::vector<Event> Game::Resolve(EffectPhase phase) {
  std::vector<Event> events;

  if (!IsRunning()) {
    return events;
  }

  switch (phase) {
    case EffectPhase::kLeaders:
      ResolveLeaders(events);
      break;
    case EffectPhase::kScorch:
      ResolveScorch(events);
      break;
    case EffectPhase::kUtility:
      ResolveUtility(events);
      break;
    case EffectPhase::kSpy:
      ResolveSpy(events);
      break;
    case EffectPhase::kClear:
      ResolveClear(events);
      break;
    case EffectPhase::kWeather:
      ResolveWeather(events);
      break;
    case EffectPhase::kHorn:
      ResolveHorn(events);
      break;
    default:
      ResolveCombat(phase, events);
      break;
  }

  return events;
}

void Game::ResolveScorch(std::vector<Event>& events) {
  const auto sources = Sources(CardType::kScorch);
  std::vector<int> cancel;

  for (int id : sources) {
    const int target = played_[OpponentOf(cards_[id].owner)];

    if (IsOnBoard(target) && EffectiveValue(target) >= EffectiveValue(id)) {
      cancel.push_back(target);
    }
  }

  for (int id : cancel) {
    if (!cards_[id].suppressed) {
      cards_[id].suppressed = true;
      events.emplace_back(Event::kSuppress, cards_[id].owner, id, 1);
    }
  }

  Decay(sources, events);
}

void Game::ResolveSpy(std::vector<Event>& events) {
  const auto sources = Sources(CardType::kSpy);
  std::array<std::vector<int>, kPlayerCount> available = {
      {players_[kHumanPlayer].hand, players_[kEnemyPlayer].hand}};
  std::vector<std::pair<int, int>> transfers;

  for (int id : sources) {
    if (!cards_[id].suppressed) {
      const int owner = cards_[id].owner;
      auto& candidates = available[OpponentOf(owner)];

      if (candidates.empty()) {
        continue;
      }

      const int index = PickIndex(static_cast<int>(candidates.size()));
      transfers.emplace_back(candidates[index], owner);
      candidates.erase(candidates.begin() + index);
    }
  }

  for (const auto& transfer : transfers) {
    Move(transfer.first, transfer.second, CardZone::kHand);
    events.emplace_back(Event::kTransfer, transfer.second, transfer.first, 1);
  }

  Decay(sources, events);
}

void Game::ResolveClear(std::vector<Event>& events) {
  const auto sources = Sources(CardType::kClear);
  bool clear = false;

  for (int id : sources) {
    if (!cards_[id].suppressed) {
      clear = true;
    }
  }

  if (clear) {
    ClearWeather(events);
  }

  Decay(sources, events);
}

void Game::ResolveHorn(std::vector<Event>& events) {
  const auto sources = Sources(CardType::kHorn);

  for (int id : sources) {
    if (!cards_[id].suppressed) {
      ArmHorn(cards_[id].owner, Lane::kAny, events);
    }
  }

  Decay(sources, events);
}

void Game::ResolveCombat(EffectPhase phase, std::vector<Event>& events) {
  const auto sources = Sources(EffectType(phase));
  std::array<int, kPlayerCount> totals = {{0, 0}};

  for (int id : sources) {
    if (!cards_[id].suppressed) {
      totals[cards_[id].owner] += EffectiveValue(id);
    }
  }

  for (int owner = 0; owner < kPlayerCount; ++owner) {
    Player& player = players_[owner];

    if (phase == EffectPhase::kShield) {
      player.shield += totals[owner];

      if (totals[owner]) {
        events.emplace_back(Event::kGuard, owner, -1, totals[owner]);
      }
    } else if (phase == EffectPhase::kDamage) {
      const int absorbed = std::min(player.shield, totals[OpponentOf(owner)]);
      player.shield -= absorbed;
      const int damage = totals[OpponentOf(owner)] - absorbed;
      player.hp = std::max(0, player.hp - damage);

      if (totals[OpponentOf(owner)]) {
        events.emplace_back(Event::kHit, owner, -1, damage);
      }
    } else if (phase == EffectPhase::kHeal) {
      const int old = player.hp;
      player.hp = std::min(static_cast<int>(kMaxHp), player.hp + totals[owner]);

      if (player.hp != old) {
        events.emplace_back(Event::kRestore, owner, -1, player.hp - old);
      }
    }
  }

  if (phase == EffectPhase::kDamage) {
    CheckLethal();

    if (round_result_ != GameResult::kPlaying) {
      events.emplace_back(Event::kRoundEnd, kNoOwner, kNoCard,
                          static_cast<int>(round_result_));
    }
  }
}

void Game::ClearWeather(std::vector<Event>& events) {
  std::vector<int> weather;

  for (const Player& player : players_) {
    for (int id : player.board) {
      if (IsWeather(cards_[id].type)) {
        weather.push_back(id);
      }
    }
  }

  for (int id : weather) {
    Destroy(id, events);
  }
}

void Game::ArmHorn(int owner, Lane lane, std::vector<Event>& events) {
  if (horn_turn_[owner] != turn_ + 1 || horn_lane_[owner] != Lane::kAny) {
    horn_lane_[owner] = lane;
  }

  horn_turn_[owner] = turn_ + 1;
  events.emplace_back(Event::kBoost, owner, -1, static_cast<int>(lane));
}

void Game::ResolveLeaders(std::vector<Event>& events) {
  std::vector<int> destroy;
  std::array<std::vector<int>, kPlayerCount> reveals;
  bool clear = false;
  // Select every target before applying destructive effects.
  for (int owner = 0; owner < kPlayerCount; ++owner) {
    if (!committed_[owner] || actions_[owner].kind != Action::kUseLeader) {
      continue;
    }

    const Leader leader = players_[owner].leader;
    events.emplace_back(Event::kLeaderUsed, owner, -1,
                        static_cast<int>(leader));

    if (leader == Leader::kFoltestClear) {
      clear = true;
    } else if (leader == Leader::kEmhyr) {
      reveals[owner] = players_[OpponentOf(owner)].hand;
      std::shuffle(reveals[owner].begin(), reveals[owner].end(), random_);

      if (reveals[owner].size() > kLeaderRevealCount) {
        reveals[owner].resize(kLeaderRevealCount);
      }
    } else if (leader == Leader::kEredin) {
      melee_boost_[owner] = true;
    } else if (leader == Leader::kFrancesca) {
      int strongest = -1;
      std::vector<int> tied;

      for (int id : players_[OpponentOf(owner)].board) {
        if (cards_[id].lane == Lane::kMelee) {
          const int value = cards_[id].value;

          if (value > strongest) {
            strongest = value;
            tied.clear();
          }

          if (value == strongest) {
            tied.push_back(id);
          }
        }
      }

      if (!tied.empty()) {
        destroy.push_back(tied[PickIndex(static_cast<int>(tied.size()))]);
      }
    } else if (leader == Leader::kFoltestSiege) {
      ArmHorn(owner, Lane::kSiege, events);
    }
  }

  if (clear) {
    ClearWeather(events);
  }

  for (int id : destroy) {
    Destroy(id, events);
  }

  for (int owner = 0; owner < kPlayerCount; ++owner) {
    for (int id : reveals[owner]) {
      if (owner == kHumanPlayer && cards_[id].owner == kEnemyPlayer) {
        cards_[id].known = true;
      }

      events.emplace_back(Event::kReveal, owner, id, 1);
    }
  }
}

void Game::ResolveUtility(std::vector<Event>& events) {
  // Medic and Decoy trigger once when played; replaying them triggers them
  // again.
  for (int owner = 0; owner < kPlayerCount; ++owner) {
    if (utility_used_[owner]) {
      continue;
    }

    utility_used_[owner] = true;
    const int id = played_[owner];

    if (!IsOnBoard(id) || cards_[id].suppressed) {
      continue;
    }

    if (cards_[id].type == CardType::kMedic) {
      const auto available = players_[owner].discard;

      if (available.empty()) {
        continue;
      }

      const int revived =
          available[PickIndex(static_cast<int>(available.size()))];
      Move(revived, owner, CardZone::kHand);
      cards_[revived].value = cards_[revived].printed_value;
      events.emplace_back(Event::kRevive, owner, revived,
                          cards_[revived].value);
    } else if (cards_[id].type == CardType::kDecoy) {
      const int target = targets_[owner];

      if (!IsOnBoard(target) || cards_[target].owner != owner) {
        continue;
      }

      auto& board = players_[owner].board;
      const int position = static_cast<int>(
          std::find(board.begin(), board.end(), target) - board.begin());
      Move(target, owner, CardZone::kHand);
      cards_[target].value = cards_[target].printed_value;
      RemoveCard(board, id);
      board.insert(
          board.begin() + std::min(position, static_cast<int>(board.size())),
          id);
      events.emplace_back(Event::kReturn, owner, target, cards_[target].value);
    }
  }
}

void Game::ResolveWeather(std::vector<Event>& events) {
  std::array<std::array<int, kLaneCount>, kPlayerCount> reductions = {
      {{{0, 0, 0}}, {{0, 0, 0}}}};
  std::vector<int> board;

  for (const Player& player : players_) {
    for (int id : player.board) {
      board.push_back(id);

      if (IsWeather(cards_[id].type) && !cards_[id].suppressed) {
        reductions[OpponentOf(cards_[id].owner)]
                  [static_cast<int>(WeatherLane(cards_[id].type))] +=
            kWeatherReduction;
      }
    }
  }

  for (int id : board) {
    const int loss =
        reductions[cards_[id].owner][static_cast<int>(cards_[id].lane)] +
        (IsWeather(cards_[id].type) ? kTemporaryDecay : 0);
    cards_[id].value -= loss;

    if (loss) {
      events.emplace_back(Event::kValue, cards_[id].owner, id, -loss);
    }

    if (cards_[id].value <= 0) {
      Destroy(id, events);
    }
  }
}

}  // namespace tema1
