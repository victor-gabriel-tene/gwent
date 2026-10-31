#ifndef LAB_M1_TEMA1_GAME_TYPES_H_
#define LAB_M1_TEMA1_GAME_TYPES_H_

#include <array>
#include <string>
#include <vector>

#include "lab_m1/Tema1/game_config.h"

namespace tema1 {

enum class CardType {
  kDamage,
  kHeal,
  kShield,
  kSpy,
  kFrost,
  kClear,
  kFog,
  kRain,
  kHorn,
  kScorch,
  kMedic,
  kDecoy,
  kCount
};
enum class CardZone { kDeck, kHand, kBoard, kDiscard };
enum class Lane { kNone = -1, kMelee, kRanged, kSiege, kAny };
enum class Leader {
  kFoltestClear,
  kEmhyr,
  kEredin,
  kFrancesca,
  kFoltestSiege,
  kCount
};
enum class GameResult { kPlaying, kPlayerWin, kEnemyWin, kDraw };
enum class EffectPhase {
  kLeaders,
  kScorch,
  kUtility,
  kSpy,
  kClear,
  kWeather,
  kHorn,
  kShield,
  kDamage,
  kHeal,
  kCount,
  kFrost = kWeather
};

constexpr int kLaneCount = static_cast<int>(Lane::kAny);
constexpr int kCardTypeCount = static_cast<int>(CardType::kCount);
constexpr int kEffectPhaseCount = static_cast<int>(EffectPhase::kCount);

struct Card {
  int id;
  int owner;
  int value;
  int printed_value;
  CardType type;
  CardZone zone;
  Lane printed_lane;
  Lane lane;
  bool known = false;
  bool suppressed = false;

  Card(int card_id, int card_owner, CardType card_type, int card_value,
       CardZone card_zone, Lane allowed = Lane::kAny)
      : id(card_id),
        owner(card_owner),
        value(card_value),
        printed_value(card_value),
        type(card_type),
        zone(card_zone),
        printed_lane(allowed),
        lane(Lane::kNone) {}
};

struct Player {
  int hp = kStartingHp;
  int shield = 0;
  int crystals = kStartingCrystals;
  std::vector<int> deck;
  std::vector<int> hand;
  std::vector<int> board;
  std::vector<int> discard;
  Leader leader = Leader::kFoltestClear;
  bool leader_used = false;
};

struct Action {
  enum Kind { kPlayCard, kUseLeader, kSkip } kind;

  int card;
  Lane lane;
  int target;

  explicit Action(Kind action_kind = kSkip, int card_id = kNoCard,
                  Lane destination = Lane::kNone, int target_id = kNoCard)
      : kind(action_kind),
        card(card_id),
        lane(destination),
        target(target_id) {}
};

struct Event {
  enum Kind {
    kTransfer,
    kDestroy,
    kValue,
    kGuard,
    kHit,
    kRestore,
    kReturn,
    kRevive,
    kReveal,
    kSuppress,
    kBoost,
    kLeaderUsed,
    kRoundEnd,
    kRoundStart
  } kind;

  int owner;
  int card;
  int amount;

  Event(Kind event_kind, int event_owner, int card_id, int value)
      : kind(event_kind), owner(event_owner), card(card_id), amount(value) {}
};

const std::array<EffectPhase, kEffectPhaseCount>& EffectPhases();
const char* PhaseName(EffectPhase phase);
std::string ResolutionOrder();
const char* CardName(CardType type);
const char* LaneName(Lane lane);
const char* LaneSymbol(Lane lane);
const char* LeaderName(Leader leader);
const char* LeaderDescription(Leader leader);
const char* CardDescription(CardType type);
bool IsWeather(CardType type);
Lane WeatherLane(CardType type);
bool IsTemporary(CardType type);
bool IsUnit(CardType type);
CardType EffectType(EffectPhase phase);

}  // namespace tema1

#endif  // LAB_M1_TEMA1_GAME_TYPES_H_
