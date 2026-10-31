#ifndef LAB_M1_TEMA1_GAME_RULES_H_
#define LAB_M1_TEMA1_GAME_RULES_H_

#include <array>
#include <random>
#include <vector>

#include "lab_m1/Tema1/game_types.h"

namespace tema1 {

// Owns the match state and applies actions without graphics dependencies.
class Game {
 public:
  static constexpr int kMaxHp = kStartingHp;

  // Plain match data inspected by the scene and deterministic rule fixtures.
  std::array<Player, kPlayerCount> players_;
  std::vector<Card> cards_;
  GameResult result_ = GameResult::kPlaying;
  GameResult round_result_ = GameResult::kPlaying;
  int round_ = kFirstRound;
  int turn_ = kFirstTurn;
  std::mt19937 random_;

  explicit Game(unsigned seed = kDefaultSeed);
  int Add(int owner, CardType type, int value, CardZone zone,
          Lane allowed = Lane::kAny);
  void ResetEmpty(unsigned seed = kDefaultSeed);
  void Reset(unsigned seed, bool deal = true);
  void Move(int id, int owner, CardZone zone);
  int Draw(int owner);
  std::vector<Lane> LegalLanes(int owner, int id) const;
  bool CanPlay(int owner, int id, Lane lane = Lane::kNone,
               int target = kNoCard) const;
  bool CanUseLeader(int owner) const;
  // Availability for a future turn, independent of the current commitment.
  bool HasAction(int owner) const;
  bool CanAct(int owner, const Action& action) const;
  bool Commit(int owner, const Action& requested);
  bool Play(int owner, int id, Lane lane = Lane::kNone, int target = kNoCard);
  bool AnyCommitted() const;
  bool HasCommittedAction(int owner) const;
  Action ChooseAction(int owner);
  int EffectiveValue(int id) const;
  void Destroy(int id, std::vector<Event>& events);
  // Each phase snapshots its sources. Cancellation never freezes lifetime.
  std::vector<Event> Resolve(EffectPhase phase);
  void CheckLethal();
  std::vector<Event> FinishTurn();
  bool NextRound(bool deal = true);
  bool HasValidState() const;

 private:
  static void RemoveCard(std::vector<int>& list, int id);
  std::vector<int>& CardsInZone(int owner, CardZone zone);
  std::vector<int> Sources(CardType type) const;
  void Decay(const std::vector<int>& sources, std::vector<Event>& events);
  void BuildDeck(int owner);
  static bool IsValidOwner(int owner);
  static bool IsBoardLane(Lane lane);
  bool IsValidCard(int id) const;
  bool IsOnBoard(int id) const;
  bool IsHandCard(int owner, int id) const;
  bool IsRunning() const;
  int PickIndex(int size);
  bool LegalPlacement(int owner, int id, Lane lane, int target) const;
  void ClearTurn();
  void EndRound(GameResult outcome);
  void ScoreRound();
  void ClearWeather(std::vector<Event>& events);
  void ArmHorn(int owner, Lane lane, std::vector<Event>& events);
  void ResolveScorch(std::vector<Event>& events);
  void ResolveSpy(std::vector<Event>& events);
  void ResolveClear(std::vector<Event>& events);
  void ResolveHorn(std::vector<Event>& events);
  void ResolveCombat(EffectPhase phase, std::vector<Event>& events);
  void ResolveLeaders(std::vector<Event>& events);
  void ResolveUtility(std::vector<Event>& events);
  void ResolveWeather(std::vector<Event>& events);

  std::array<Action, kPlayerCount> actions_;
  std::array<bool, kPlayerCount> committed_ = {};
  std::array<bool, kPlayerCount> utility_used_ = {};
  std::array<int, kPlayerCount> played_ = {};
  std::array<int, kPlayerCount> targets_ = {};
  std::array<bool, kPlayerCount> melee_boost_ = {};
  std::array<int, kPlayerCount> horn_turn_ = {};
  std::array<Lane, kPlayerCount> horn_lane_ = {};
  bool round_scored_ = false;
};

}  // namespace tema1

#endif  // LAB_M1_TEMA1_GAME_RULES_H_
