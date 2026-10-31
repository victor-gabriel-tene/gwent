// The application build also discovers this file; only the standalone test
// command defines TEMA1_RULE_TESTS and introduces this main function.
#ifdef TEMA1_RULE_TESTS
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

#include "lab_m1/Tema1/game_rules.h"

using tema1::Action;
using tema1::Card;
using tema1::CardType;
using tema1::CardZone;
using tema1::EffectPhase;
using tema1::EffectPhases;
using tema1::Event;
using tema1::Game;
using tema1::GameResult;
using tema1::IsUnit;
using tema1::Lane;
using tema1::Leader;
using tema1::PhaseName;
using tema1::Player;

namespace {
int checks = 0;
const char* section = "initialization";

void Expect(bool condition, const char* message) {
  ++checks;

  if (!condition) {
    throw std::runtime_error(std::string(section) + ": " + message);
  }
}

void ResolveTurn(Game& game) {
  for (EffectPhase phase : EffectPhases()) {
    game.Resolve(phase);
    Expect(game.HasValidState(), "invariant after resolution phase");
  }

  game.FinishTurn();
  Expect(game.HasValidState(), "invariant after completed turn");
}

void KeepPlaying(Game& game) {
  game.Add(0, CardType::kHeal, 1, CardZone::kHand, Lane::kRanged);
  game.Add(1, CardType::kHeal, 1, CardZone::kHand, Lane::kRanged);
}

void EnableLeader(Game& game, int owner, Leader leader) {
  game.players_[owner].leader = leader;
  game.players_[owner].leader_used = false;
}

void UseLeader(Game& game, int owner, Leader leader) {
  EnableLeader(game, owner, leader);
  Expect(game.Commit(owner, Action(Action::kUseLeader)),
         "leader action commits");
}

void OpeningAndLegality() {
  section = "opening and legal actions";
  Game game(42), again(42);
  Expect(game.cards_.size() == 60 && game.HasValidState(),
         "sixty unique cards");
  Expect(
      game.players_[0].hand.size() == 10 && game.players_[1].deck.size() == 20,
      "opening ten-card deal");
  Expect(game.players_[0].hand == again.players_[0].hand,
         "repeatable seeded deal");

  for (int owner = 0; owner < 2; ++owner) {
    int count[12] = {}, damage_values[4] = {}, damage_lanes[3] = {};

    for (const Card& card : game.cards_) {
      if (card.owner == owner) {
        ++count[static_cast<int>(card.type)];

        if (card.type == CardType::kDamage) {
          ++damage_values[card.printed_value];
          ++damage_lanes[static_cast<int>(card.printed_lane)];
        }

        if (IsUnit(card.type)) {
          Expect(card.printed_lane == Lane::kMelee ||
                     card.printed_lane == Lane::kRanged ||
                     card.printed_lane == Lane::kSiege,
                 "units have a printed lane");
        }

        if (card.type == CardType::kMedic) {
          Expect(card.printed_lane == Lane::kRanged, "medics are ranged");
        }
      }
    }

    Expect(count[static_cast<int>(CardType::kDamage)] == 12 &&
               count[static_cast<int>(CardType::kHeal)] == 4 &&
               count[static_cast<int>(CardType::kShield)] == 4 &&
               count[static_cast<int>(CardType::kMedic)] == 2,
           "permanent deck composition");

    for (CardType type : {CardType::kSpy, CardType::kFrost, CardType::kClear,
                          CardType::kFog, CardType::kRain, CardType::kHorn,
                          CardType::kScorch, CardType::kDecoy}) {
      Expect(count[static_cast<int>(type)] == 1, "one of every special card");
    }

    Expect(
        damage_values[1] == 4 && damage_values[2] == 4 && damage_values[3] == 4,
        "damage value distribution");
    Expect(damage_lanes[0] == 4 && damage_lanes[1] == 4 && damage_lanes[2] == 4,
           "damage lanes round-robin");
    Expect(
        game.players_[owner].crystals == 2 && !game.players_[owner].leader_used,
        "fresh crystals and leaders");
  }

  const EffectPhase expected[] = {EffectPhase::kLeaders, EffectPhase::kScorch,
                                  EffectPhase::kUtility, EffectPhase::kSpy,
                                  EffectPhase::kClear,   EffectPhase::kWeather,
                                  EffectPhase::kHorn,    EffectPhase::kShield,
                                  EffectPhase::kDamage,  EffectPhase::kHeal};
  const auto& phases = EffectPhases();
  Expect(phases.size() == sizeof(expected) / sizeof(expected[0]),
         "one complete phase list");

  for (std::size_t i = 0; i < phases.size(); ++i) {
    Expect(phases[i] == expected[i] && PhaseName(phases[i])[0],
           "prescribed phase ordering");
  }

  game.ResetEmpty();
  const int unit =
      game.Add(0, CardType::kDamage, 3, CardZone::kHand, Lane::kSiege);
  Expect(!game.Commit(0, Action(Action::kSkip)), "cannot skip a playable card");
  Expect(!game.CanPlay(1, unit, Lane::kSiege) &&
             !game.CanPlay(0, -1, Lane::kSiege),
         "owner and ID validation");
  Expect(!game.CanPlay(0, unit, Lane::kMelee) &&
             !game.CanPlay(0, unit, Lane::kAny) &&
             !game.CanAct(0, Action(Action::kPlayCard, unit, Lane::kNone)),
         "wrong and non-board lanes rejected");
  Expect(game.Play(0, unit, Lane::kSiege), "correct lane commits");
  const int extra =
      game.Add(0, CardType::kShield, 1, CardZone::kHand, Lane::kSiege);
  Expect(!game.Play(0, extra, Lane::kSiege) &&
             !game.Commit(0, Action(Action::kSkip)),
         "one primary action per turn");
  Expect(game.cards_[unit].lane == Lane::kSiege && game.AnyCommitted(),
         "lane and turn context");
  game.ResetEmpty();

  for (const auto& entry : {std::make_pair(CardType::kFrost, Lane::kMelee),
                            std::make_pair(CardType::kFog, Lane::kRanged),
                            std::make_pair(CardType::kRain, Lane::kSiege)}) {
    const int id = game.Add(0, entry.first, 2, CardZone::kHand);

    for (Lane lane : {Lane::kMelee, Lane::kRanged, Lane::kSiege}) {
      Expect(game.CanPlay(0, id, lane) == (lane == entry.second),
             "weather lane restriction");
    }
  }

  const int spy = game.Add(0, CardType::kSpy, 2, CardZone::kHand);

  for (Lane lane : {Lane::kMelee, Lane::kRanged, Lane::kSiege}) {
    Expect(game.CanPlay(0, spy, lane),
           "non-weather temporary accepts all friendly lanes");
  }
}

void CoreRecurrence() {
  section = "permanent effects and spies";
  Game game;
  game.ResetEmpty();
  const int damage =
      game.Add(0, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
  game.Resolve(EffectPhase::kDamage);
  game.Resolve(EffectPhase::kDamage);
  Expect(game.players_[1].hp == 24 && game.cards_[damage].value == 3,
         "permanent damage recurs");
  game.ResetEmpty();
  game.players_[0].hp = 10;
  const int shield =
      game.Add(0, CardType::kShield, 3, CardZone::kBoard, Lane::kSiege);
  game.Add(1, CardType::kDamage, 5, CardZone::kBoard, Lane::kMelee);
  game.Add(0, CardType::kHeal, 2, CardZone::kBoard, Lane::kRanged);
  game.Resolve(EffectPhase::kShield);
  game.Resolve(EffectPhase::kDamage);
  game.Resolve(EffectPhase::kHeal);
  Expect(game.players_[0].hp == 10 && game.players_[0].shield == 0,
         "shield absorbs before health and heal");
  game.Resolve(EffectPhase::kShield);
  game.Resolve(EffectPhase::kShield);
  std::vector<Event> events;
  game.Destroy(shield, events);
  Expect(game.players_[0].shield == 6, "shield survives source destruction");
  game.players_[0].hp = 29;
  game.Resolve(EffectPhase::kHeal);
  Expect(game.players_[0].hp == 30, "healing cap");
  game.ResetEmpty();
  const int spy0 =
      game.Add(0, CardType::kSpy, 2, CardZone::kBoard, Lane::kMelee);
  const int spy1 =
      game.Add(1, CardType::kSpy, 2, CardZone::kBoard, Lane::kMelee);
  const int hand0 =
      game.Add(0, CardType::kDamage, 1, CardZone::kHand, Lane::kMelee);
  const int hand1 =
      game.Add(1, CardType::kShield, 1, CardZone::kHand, Lane::kSiege);
  game.cards_[hand1].known = true;
  game.Resolve(EffectPhase::kSpy);
  Expect(game.players_[0].hand == std::vector<int>(1, hand1) &&
             game.players_[1].hand == std::vector<int>(1, hand0),
         "simultaneous spy swap");
  Expect(!game.cards_[hand0].known && game.cards_[spy0].value == 1 &&
             game.cards_[spy1].value == 1 && game.HasValidState(),
         "spy ownership, hidden information and decay");
  game.Resolve(EffectPhase::kSpy);
  Expect(game.cards_[spy0].zone == CardZone::kDiscard &&
             game.cards_[spy1].zone == CardZone::kDiscard,
         "spy expiration");
  game.ResetEmpty();
  const int no_target =
      game.Add(0, CardType::kSpy, 1, CardZone::kBoard, Lane::kRanged);
  game.Resolve(EffectPhase::kSpy);
  Expect(game.cards_[no_target].zone == CardZone::kDiscard,
         "no-target temporary decays");
  game.ResetEmpty();
  game.Add(0, CardType::kSpy, 2, CardZone::kBoard, Lane::kMelee);
  game.Add(0, CardType::kSpy, 2, CardZone::kBoard, Lane::kSiege);
  game.Add(1, CardType::kHeal, 1, CardZone::kHand, Lane::kRanged);
  game.Resolve(EffectPhase::kSpy);
  Expect(game.players_[0].hand.size() == 1 && game.players_[1].hand.empty() &&
             game.HasValidState(),
         "multiple spies choose distinct phase-start targets");
}

void WeatherAndClear() {
  section = "lane weather";
  Game game;
  game.ResetEmpty();
  const int frost =
      game.Add(0, CardType::kFrost, 2, CardZone::kBoard, Lane::kMelee);
  const int fog =
      game.Add(0, CardType::kFog, 2, CardZone::kBoard, Lane::kRanged);
  const int rain =
      game.Add(1, CardType::kRain, 2, CardZone::kBoard, Lane::kSiege);
  const int melee =
      game.Add(1, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
  const int ranged =
      game.Add(1, CardType::kHeal, 3, CardZone::kBoard, Lane::kRanged);
  const int siege =
      game.Add(1, CardType::kShield, 3, CardZone::kBoard, Lane::kSiege);
  const int friendly =
      game.Add(0, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
  const int own_siege =
      game.Add(0, CardType::kShield, 3, CardZone::kBoard, Lane::kSiege);
  game.Resolve(EffectPhase::kWeather);
  Expect(game.cards_[melee].value == 2 && game.cards_[ranged].value == 2 &&
             game.cards_[siege].value == 3 &&
             game.cards_[friendly].value == 3 &&
             game.cards_[own_siege].value == 2,
         "only opposing matching rows reduced");
  Expect(game.cards_[frost].value == 1 && game.cards_[fog].value == 1 &&
             game.cards_[rain].value == 1,
         "weather sources each lose one lifetime point");
  game.ResetEmpty();
  const int frost0 =
      game.Add(0, CardType::kFrost, 2, CardZone::kBoard, Lane::kMelee);
  const int frost1 =
      game.Add(1, CardType::kFrost, 2, CardZone::kBoard, Lane::kMelee);
  const int dead =
      game.Add(1, CardType::kDamage, 1, CardZone::kBoard, Lane::kMelee);
  const int survivor =
      game.Add(0, CardType::kDamage, 2, CardZone::kBoard, Lane::kMelee);
  game.Resolve(EffectPhase::kWeather);
  Expect(game.cards_[frost0].zone == CardZone::kDiscard &&
             game.cards_[frost1].zone == CardZone::kDiscard,
         "opposing expiring weather sources both activate");
  Expect(game.cards_[dead].zone == CardZone::kDiscard &&
             game.cards_[survivor].value == 1,
         "weather destruction");
  game.Resolve(EffectPhase::kDamage);
  Expect(game.players_[0].hp == 30 && game.players_[1].hp == 29,
         "destroyed sources cannot damage later");
  game.ResetEmpty();
  const int clear =
      game.Add(0, CardType::kClear, 1, CardZone::kBoard, Lane::kSiege);
  std::vector<int> weather;

  for (int owner = 0; owner < 2; ++owner) {
    weather.push_back(
        game.Add(owner, CardType::kFrost, 2, CardZone::kBoard, Lane::kMelee));
    weather.push_back(
        game.Add(owner, CardType::kFog, 2, CardZone::kBoard, Lane::kRanged));
    weather.push_back(
        game.Add(owner, CardType::kRain, 2, CardZone::kBoard, Lane::kSiege));
  }

  const int unchanged =
      game.Add(1, CardType::kHeal, 2, CardZone::kBoard, Lane::kRanged);
  game.Resolve(EffectPhase::kClear);
  game.Resolve(EffectPhase::kWeather);

  for (int id : weather) {
    Expect(game.cards_[id].zone == CardZone::kDiscard,
           "clear removes every weather on both sides");
  }

  Expect(game.cards_[clear].zone == CardZone::kDiscard &&
             game.cards_[unchanged].value == 2,
         "clear expires before weather");
  game.ResetEmpty();
  game.Add(0, CardType::kFog, 2, CardZone::kBoard, Lane::kRanged);
  const int reduced =
      game.Add(1, CardType::kHeal, 3, CardZone::kBoard, Lane::kRanged);
  game.Resolve(EffectPhase::kWeather);
  game.Add(1, CardType::kClear, 1, CardZone::kBoard, Lane::kMelee);
  game.Resolve(EffectPhase::kClear);
  Expect(game.cards_[reduced].value == 2,
         "clear does not restore previous reductions");
}

void ScorchAndHorn() {
  section = "scorch and horn";
  Game game;
  game.ResetEmpty();
  KeepPlaying(game);
  const int scorch =
      game.Add(0, CardType::kScorch, 2, CardZone::kBoard, Lane::kSiege);
  const int old =
      game.Add(1, CardType::kDamage, 5, CardZone::kBoard, Lane::kMelee);
  const int fresh =
      game.Add(1, CardType::kDamage, 2, CardZone::kHand, Lane::kMelee);
  Expect(game.Play(1, fresh, Lane::kMelee), "target placement");
  game.Resolve(EffectPhase::kScorch);
  Expect(game.cards_[fresh].suppressed && !game.cards_[old].suppressed &&
             game.cards_[fresh].zone == CardZone::kBoard,
         "scorch suppresses only the new opposing card, without destroying it");
  Expect(game.cards_[scorch].value == 1, "scorch decay");
  game.Resolve(EffectPhase::kDamage);
  Expect(game.players_[0].hp == 25,
         "old damage acts, suppressed new damage does not");
  game.FinishTurn();
  Expect(!game.cards_[fresh].suppressed,
         "suppression expires at turn boundary");
  game.Resolve(EffectPhase::kDamage);
  Expect(game.players_[0].hp == 18, "suppressed card resumes next turn");
  game.ResetEmpty();
  const int s0 = game.Add(0, CardType::kScorch, 2, CardZone::kHand),
            s1 = game.Add(1, CardType::kScorch, 2, CardZone::kHand);
  Expect(game.Play(0, s0, Lane::kMelee) && game.Play(1, s1, Lane::kSiege),
         "opposing scorch placement");
  game.Resolve(EffectPhase::kScorch);
  Expect(game.cards_[s0].suppressed && game.cards_[s1].suppressed &&
             game.cards_[s0].value == 1 && game.cards_[s1].value == 1,
         "opposing scorches evaluate simultaneously and both decay");
  game.ResetEmpty();
  KeepPlaying(game);
  game.Add(0, CardType::kScorch, 1, CardZone::kBoard, Lane::kMelee);
  const int spy = game.Add(1, CardType::kSpy, 2, CardZone::kHand);
  Expect(game.Play(1, spy, Lane::kRanged), "spy target placement");
  const std::size_t hand_size = game.players_[1].hand.size();
  game.Resolve(EffectPhase::kScorch);
  game.Resolve(EffectPhase::kSpy);
  Expect(
      game.cards_[spy].value == 1 && game.players_[1].hand.size() == hand_size,
      "suppressed spy cannot steal but decays");
  game.FinishTurn();
  game.Resolve(EffectPhase::kSpy);
  Expect(game.cards_[spy].zone == CardZone::kDiscard &&
             game.players_[1].hand.size() == hand_size + 1,
         "temporary resumes then expires next turn");
  game.ResetEmpty();
  KeepPlaying(game);
  game.Add(0, CardType::kScorch, 3, CardZone::kBoard, Lane::kMelee);
  const int small =
      game.Add(1, CardType::kDamage, 2, CardZone::kHand, Lane::kSiege);
  game.Play(1, small, Lane::kSiege);
  game.Resolve(EffectPhase::kScorch);
  Expect(!game.cards_[small].suppressed,
         "below-threshold target is unaffected");
  game.ResetEmpty();
  KeepPlaying(game);
  const int horn0 =
      game.Add(0, CardType::kHorn, 2, CardZone::kBoard, Lane::kMelee);
  const int horn1 =
      game.Add(0, CardType::kHorn, 2, CardZone::kBoard, Lane::kSiege);
  const int target =
      game.Add(0, CardType::kDamage, 3, CardZone::kHand, Lane::kMelee);
  game.Resolve(EffectPhase::kHorn);
  Expect(game.cards_[target].value == 3 && game.cards_[horn0].value == 1 &&
             game.cards_[horn1].value == 1,
         "horn arms future placement and decays without changing hand cards");
  game.FinishTurn();
  Expect(game.Play(0, target, Lane::kMelee) && game.cards_[target].value == 6,
         "multiple horns double next-turn card once");
  game.Add(1, CardType::kScorch, 5, CardZone::kBoard, Lane::kMelee);
  game.Resolve(EffectPhase::kScorch);
  Expect(game.cards_[target].suppressed,
         "horn applies before scorch threshold comparison");
  game.ResetEmpty();
  KeepPlaying(game);
  const int horn =
      game.Add(0, CardType::kHorn, 1, CardZone::kBoard, Lane::kMelee);
  game.Resolve(EffectPhase::kHorn);
  game.FinishTurn();
  Expect(game.cards_[horn].zone == CardZone::kDiscard,
         "expired horn retains pending effect");
  UseLeader(game, 0, Leader::kFoltestClear);
  game.Resolve(EffectPhase::kLeaders);
  game.FinishTurn();
  const int late =
      game.Add(0, CardType::kDamage, 3, CardZone::kHand, Lane::kMelee);
  Expect(game.Play(0, late, Lane::kMelee) && game.cards_[late].value == 3,
         "leader action lets horn opportunity expire");
}

void Recovery() {
  section = "medic and decoy";
  Game game;
  game.ResetEmpty();
  KeepPlaying(game);
  const int medic =
      game.Add(0, CardType::kMedic, 2, CardZone::kHand, Lane::kRanged);
  const int special = game.Add(0, CardType::kClear, 1, CardZone::kDiscard);
  game.cards_[special].value = 0;
  game.cards_[special].known = true;
  game.cards_[special].suppressed = true;
  Expect(game.Play(0, medic, Lane::kRanged), "medic placement");
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[special].zone == CardZone::kHand &&
             game.cards_[special].value == 1 &&
             !game.cards_[special].suppressed,
         "medic revives specials and restores printed state");
  const int second =
      game.Add(0, CardType::kShield, 2, CardZone::kDiscard, Lane::kSiege);
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[second].zone == CardZone::kDiscard,
         "medic activates at most once per turn");
  game.FinishTurn();
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[second].zone == CardZone::kDiscard,
         "medic does not recover again on later turns without replay");
  game.FinishTurn();
  const int decoy = game.Add(0, CardType::kDecoy, 1, CardZone::kHand);
  const int opposing =
      game.Add(1, CardType::kHeal, 2, CardZone::kBoard, Lane::kRanged);
  Expect(!game.CanPlay(0, decoy, Lane::kRanged, opposing) &&
             !game.CanPlay(0, decoy, Lane::kMelee, medic) &&
             !game.CanPlay(0, decoy, Lane::kRanged),
         "decoy requires matching friendly board target");
  const int weather =
      game.Add(0, CardType::kFog, 2, CardZone::kBoard, Lane::kRanged);
  Expect(game.CanPlay(0, decoy, Lane::kRanged, weather),
         "decoy can target a special");
  game.cards_[medic].value = 4;
  Expect(game.Play(0, decoy, Lane::kRanged, medic), "valid decoy placement");
  Expect(game.cards_[medic].zone == CardZone::kBoard,
         "decoy swap waits until after scorch");
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[medic].zone == CardZone::kHand &&
             game.cards_[medic].value == 2 &&
             game.cards_[decoy].zone == CardZone::kBoard &&
             game.cards_[decoy].lane == Lane::kRanged,
         "decoy returns reset unit and remains in its lane");
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[decoy].zone == CardZone::kBoard &&
             game.cards_[decoy].value == 1,
         "decoy is inert without recurring decay");
  game.FinishTurn();
  Expect(game.Play(0, medic, Lane::kRanged), "recalled medic replays");
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[second].zone == CardZone::kHand,
         "replayed medic triggers its ability again");
  game.ResetEmpty();
  KeepPlaying(game);
  const int empty_medic =
      game.Add(0, CardType::kMedic, 2, CardZone::kHand, Lane::kRanged);
  game.Play(0, empty_medic, Lane::kRanged);
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[empty_medic].zone == CardZone::kBoard &&
             game.players_[0].discard.empty(),
         "empty-discard medic is a safe no-op");
  game.ResetEmpty();
  KeepPlaying(game);
  game.Add(1, CardType::kScorch, 1, CardZone::kBoard, Lane::kMelee);
  const int suppressed_medic =
      game.Add(0, CardType::kMedic, 2, CardZone::kHand, Lane::kRanged);
  const int dead_unit =
      game.Add(0, CardType::kDamage, 3, CardZone::kDiscard, Lane::kMelee);
  game.Play(0, suppressed_medic, Lane::kRanged);
  game.Resolve(EffectPhase::kScorch);
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[dead_unit].zone == CardZone::kDiscard,
         "scorch cancels medic recovery");
  game.FinishTurn();
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[dead_unit].zone == CardZone::kDiscard,
         "suppressed on-play ability is not deferred");
  game.ResetEmpty();
  KeepPlaying(game);
  game.Add(1, CardType::kScorch, 1, CardZone::kBoard, Lane::kMelee);
  const int unit =
      game.Add(0, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
  const int canceled_decoy = game.Add(0, CardType::kDecoy, 1, CardZone::kHand);
  game.Play(0, canceled_decoy, Lane::kMelee, unit);
  game.Resolve(EffectPhase::kScorch);
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[unit].zone == CardZone::kBoard &&
             game.cards_[canceled_decoy].zone == CardZone::kBoard,
         "scorch cancels decoy without returning target early");
  game.ResetEmpty();
  const int lonely = game.Add(0, CardType::kDecoy, 1, CardZone::kHand);
  Expect(!game.CanPlay(0, lonely, Lane::kMelee) && !game.HasAction(0),
         "targetless decoy is not a playable action");
  game.ResetEmpty();
  KeepPlaying(game);
  const int enemy_medic =
      game.Add(1, CardType::kMedic, 2, CardZone::kHand, Lane::kRanged);
  const int enemy_weather =
      game.Add(1, CardType::kFrost, 2, CardZone::kDiscard);
  game.cards_[enemy_weather].value = 0;
  Expect(game.cards_[enemy_weather].known, "discarded cards are public");
  game.Play(1, enemy_medic, Lane::kRanged);
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[enemy_weather].zone == CardZone::kHand &&
             !game.cards_[enemy_weather].known &&
             game.cards_[enemy_weather].value == 2,
         "enemy revival restores V without leaking its new hand entry");
  game.FinishTurn();
  const int enemy_decoy = game.Add(1, CardType::kDecoy, 1, CardZone::kHand);
  Expect(game.Play(1, enemy_decoy, Lane::kRanged, enemy_medic),
         "enemy decoy placement");
  game.Resolve(EffectPhase::kUtility);
  Expect(game.cards_[enemy_medic].zone == CardZone::kHand &&
             !game.cards_[enemy_medic].known,
         "enemy decoy return clears prior public identity");

  // Every board type is a legal target; preserve its slot and make it
  // replayable.
  for (CardType type :
       {CardType::kDamage, CardType::kHeal, CardType::kShield, CardType::kSpy,
        CardType::kFrost, CardType::kClear, CardType::kFog, CardType::kRain,
        CardType::kHorn, CardType::kScorch, CardType::kMedic,
        CardType::kDecoy}) {
    for (int slot = 0; slot < 3; ++slot) {
      game.ResetEmpty();
      KeepPlaying(game);
      const Lane lane = IsWeather(type) ? WeatherLane(type) : Lane::kRanged;

      for (int i = 0; i < slot; ++i) {
        game.Add(0, CardType::kShield, 1, CardZone::kBoard, lane);
      }

      const int target = game.Add(0, type, 2, CardZone::kBoard, lane);

      for (int i = slot; i < 2; ++i) {
        game.Add(0, CardType::kShield, 1, CardZone::kBoard, lane);
      }

      const int replacement = game.Add(0, CardType::kDecoy, 1, CardZone::kHand);
      game.cards_[target].value = 4;
      Expect(game.Play(0, replacement, lane, target),
             "Decoy accepts every friendly board type");
      game.Resolve(EffectPhase::kUtility);
      Expect(game.players_[0].board[slot] == replacement &&
                 game.cards_[target].zone == CardZone::kHand &&
                 game.cards_[target].value == 2 &&
                 !game.cards_[target].suppressed && game.HasValidState(),
             "replacement preserves first/middle/last slot and restores "
             "returned card");
      game.FinishTurn();
      Expect(game.Play(0, target, lane,
                       type == CardType::kDecoy ? replacement : -1),
             "returned card can be replayed");
    }
  }

  game.ResetEmpty();
  KeepPlaying(game);
  game.Add(0, CardType::kMedic, 2, CardZone::kBoard, Lane::kRanged);
  game.Add(0, CardType::kMedic, 2, CardZone::kBoard, Lane::kRanged);
  game.Add(0, CardType::kRain, 2, CardZone::kDiscard);
  game.Add(0, CardType::kShield, 2, CardZone::kDiscard, Lane::kSiege);
  const std::size_t before_recovery = game.players_[0].hand.size();
  game.Resolve(EffectPhase::kUtility);
  Expect(game.players_[0].discard.size() == 2 &&
             game.players_[0].hand.size() == before_recovery &&
             game.HasValidState(),
         "surviving medics do not recycle discarded cards without replay");
}

void EnemyResponses() {
  section = "random enemy card response";
  Game game;

  for (unsigned seed = 0; seed < 100; ++seed) {
    game.ResetEmpty(seed);
    EnableLeader(game, 1, Leader::kEmhyr);
    const int decoy = game.Add(1, CardType::kDecoy, 1, CardZone::kHand);
    Expect(game.ChooseAction(1).kind == Action::kUseLeader,
           "targetless hand falls back to unused leader");
    game.players_[1].leader_used = true;
    Expect(game.ChooseAction(1).kind == Action::kSkip,
           "no card or leader safely skips");
    game.players_[1].leader_used = false;
    const int target = game.Add(1, CardType::kFog, 2, CardZone::kBoard);
    const Action swap = game.ChooseAction(1);
    Expect(swap.kind == Action::kPlayCard && swap.card == decoy &&
               swap.target == target && swap.lane == Lane::kRanged &&
               game.CanAct(1, swap),
           "enemy decoy selects a friendly special");
    game.Add(1, CardType::kDamage, 3, CardZone::kHand, Lane::kMelee);
    game.Add(1, CardType::kShield, 2, CardZone::kHand, Lane::kSiege);

    for (int n = 0; n < 10; ++n) {
      const Action action = game.ChooseAction(1);
      Expect(action.kind == Action::kPlayCard && game.CanAct(1, action),
             "playable hand always gets a legal card response");
    }
  }
}

void Leaders() {
  section = "leader abilities";
  Game game;
  game.ResetEmpty();
  KeepPlaying(game);
  const int weather0 =
      game.Add(0, CardType::kFrost, 2, CardZone::kBoard, Lane::kMelee);
  const int weather1 =
      game.Add(1, CardType::kRain, 2, CardZone::kBoard, Lane::kSiege);
  UseLeader(game, 0, Leader::kFoltestClear);
  Expect(game.players_[0].leader_used && !game.CanUseLeader(0),
         "leader consumed at commitment");
  game.Resolve(EffectPhase::kLeaders);
  Expect(game.cards_[weather0].zone == CardZone::kDiscard &&
             game.cards_[weather1].zone == CardZone::kDiscard,
         "leader clears both weather sides");
  game.FinishTurn();
  Expect(!game.CanUseLeader(0), "leader cannot be reused");
  game.ResetEmpty();

  for (int i = 0; i < 5; ++i) {
    game.Add(1, CardType::kDamage, i + 1, CardZone::kHand, Lane::kMelee);
  }

  UseLeader(game, 0, Leader::kEmhyr);
  const auto reveals = game.Resolve(EffectPhase::kLeaders);
  std::set<int> seen;

  for (const Event& event : reveals) {
    if (event.kind == Event::kReveal) {
      Expect(event.owner == 0, "reveal event identifies viewer");
      seen.insert(event.card);
    }
  }

  int known = 0;

  for (int id : game.players_[1].hand) {
    if (game.cards_[id].known) {
      ++known;
    }
  }

  Expect(seen.size() == 3 && known == 3,
         "Emhyr reveals three distinct hand cards");
  const int revealed = *seen.begin();
  Expect(game.Play(1, revealed, Lane::kMelee) &&
             game.cards_[revealed].zone == CardZone::kBoard,
         "revealed card remains legally playable");
  game.Move(revealed, 1, CardZone::kHand);
  Expect(!game.cards_[revealed].known,
         "revealed identity resets on a new hand entry");
  game.ResetEmpty();
  const int only =
      game.Add(1, CardType::kShield, 1, CardZone::kHand, Lane::kSiege);
  UseLeader(game, 0, Leader::kEmhyr);
  game.Resolve(EffectPhase::kLeaders);
  Expect(game.cards_[only].known, "Emhyr handles fewer than three cards");
  game.ResetEmpty();
  UseLeader(game, 0, Leader::kEmhyr);
  game.Resolve(EffectPhase::kLeaders);
  Expect(game.players_[0].leader_used && game.HasValidState(),
         "targetless leader safely consumed");
  game.ResetEmpty();
  KeepPlaying(game);
  const int melee =
      game.Add(0, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
  const int other =
      game.Add(0, CardType::kShield, 2, CardZone::kBoard, Lane::kSiege);
  const int temporary =
      game.Add(0, CardType::kSpy, 2, CardZone::kBoard, Lane::kMelee);
  const int enemy =
      game.Add(1, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
  UseLeader(game, 0, Leader::kEredin);
  game.Resolve(EffectPhase::kLeaders);
  Expect(game.EffectiveValue(melee) == 6 &&
             game.EffectiveValue(temporary) == 4 &&
             game.EffectiveValue(other) == 2 && game.EffectiveValue(enemy) == 3,
         "Eredin doubles only own melee values");
  game.Resolve(EffectPhase::kSpy);
  Expect(
      game.cards_[temporary].value == 1 && game.EffectiveValue(temporary) == 2,
      "Eredin never extends temporary lifetime");
  game.Resolve(EffectPhase::kDamage);
  Expect(game.players_[1].hp == 24, "Eredin affects damage magnitude");
  game.FinishTurn();
  Expect(game.cards_[melee].value == 3 && game.EffectiveValue(melee) == 3,
         "Eredin ends without compounding base value");
  game.ResetEmpty(17);
  KeepPlaying(game);
  const int top0 =
      game.Add(1, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
  const int top1 =
      game.Add(1, CardType::kHeal, 3, CardZone::kBoard, Lane::kMelee);
  const int low =
      game.Add(1, CardType::kShield, 2, CardZone::kBoard, Lane::kMelee);
  const int ranged =
      game.Add(1, CardType::kDamage, 8, CardZone::kBoard, Lane::kRanged);
  UseLeader(game, 0, Leader::kFrancesca);
  game.Resolve(EffectPhase::kLeaders);
  Expect((game.cards_[top0].zone == CardZone::kDiscard) !=
             (game.cards_[top1].zone == CardZone::kDiscard),
         "Francesca destroys exactly one tied maximum");
  Expect(game.cards_[low].zone == CardZone::kBoard &&
             game.cards_[ranged].zone == CardZone::kBoard,
         "Francesca ignores other rows and weaker units");
  game.ResetEmpty();
  UseLeader(game, 0, Leader::kFrancesca);
  game.Resolve(EffectPhase::kLeaders);
  Expect(game.players_[0].leader_used && game.HasValidState(),
         "Francesca handles empty melee");

  for (Lane lane : {Lane::kMelee, Lane::kSiege}) {
    game.ResetEmpty();
    KeepPlaying(game);
    UseLeader(game, 0, Leader::kFoltestSiege);
    game.Resolve(EffectPhase::kLeaders);
    game.FinishTurn();
    const int target = game.Add(0, CardType::kDamage, 3, CardZone::kHand, lane);
    Expect(game.Play(0, target, lane), "siege horn next-turn placement");
    Expect(game.cards_[target].value == (lane == Lane::kSiege ? 6 : 3),
           "siege horn lane restriction");
    game.FinishTurn();
    const int late =
        game.Add(0, CardType::kShield, 2, CardZone::kHand, Lane::kSiege);
    Expect(game.Play(0, late, Lane::kSiege) && game.cards_[late].value == 2,
           "siege horn expires after next turn");
  }

  game.ResetEmpty();
  KeepPlaying(game);
  const int doomed = game.Add(1, CardType::kScorch, 3, CardZone::kHand);
  game.Play(1, doomed, Lane::kMelee);
  UseLeader(game, 0, Leader::kFrancesca);
  game.Resolve(EffectPhase::kLeaders);
  game.Resolve(EffectPhase::kScorch);
  Expect(game.cards_[doomed].zone == CardZone::kDiscard,
         "leader destruction precedes scorch source capture");
}

void RoundsAndExhaustion() {
  section = "rounds and exhaustion";
  Game game;
  game.ResetEmpty();
  game.players_[0].hp = game.players_[1].hp = 1;
  game.Add(0, CardType::kDamage, 1, CardZone::kBoard, Lane::kMelee);
  game.Add(1, CardType::kDamage, 1, CardZone::kBoard, Lane::kMelee);
  game.Add(0, CardType::kHeal, 3, CardZone::kBoard, Lane::kRanged);
  game.Resolve(EffectPhase::kDamage);
  Expect(game.round_result_ == GameResult::kDraw &&
             game.result_ == GameResult::kPlaying &&
             game.players_[0].crystals == 2 && game.players_[1].crystals == 2,
         "lethal records round outcome while crystal scoring waits for turn "
         "completion");
  game.CheckLethal();
  game.Resolve(EffectPhase::kHeal);
  Expect(game.players_[0].hp == 0 && game.players_[0].crystals == 2,
         "lethal blocks healing without scoring early");
  game.FinishTurn();
  Expect(game.round_result_ == GameResult::kDraw &&
             game.result_ == GameResult::kPlaying && game.players_[0].hp == 0 &&
             game.players_[0].crystals == 1 && game.players_[1].crystals == 1,
         "simultaneous lethal precedes heal and costs both crystals");
  game.FinishTurn();
  Expect(game.players_[0].crystals == 1 && game.players_[1].crystals == 1,
         "round award is idempotent");
  Expect(game.NextRound() && game.round_ == 2, "next round begins");
  game.FinishTurn();
  Expect(game.result_ == GameResult::kDraw && game.players_[0].crystals == 0 &&
             game.players_[1].crystals == 0 && !game.NextRound(),
         "simultaneous last-crystal loss draws match");
  game.ResetEmpty();
  game.players_[1].hp = 29;
  game.players_[1].shield = 100;
  game.FinishTurn();
  Expect(game.round_result_ == GameResult::kPlayerWin &&
             game.result_ == GameResult::kPlaying,
         "exhaustion uses HP only and ends round, not match");
  game.NextRound();
  game.players_[1].hp = 0;
  game.CheckLethal();
  game.FinishTurn();
  Expect(game.result_ == GameResult::kPlayerWin && game.round_ == 2,
         "two straight wins end match");
  game.ResetEmpty();
  EnableLeader(game, 0, Leader::kEmhyr);
  EnableLeader(game, 1, Leader::kFoltestClear);
  game.players_[1].hp = 29;
  game.FinishTurn();
  Expect(game.round_result_ == GameResult::kPlayerWin &&
             !game.players_[0].leader_used && !game.players_[1].leader_used,
         "empty hands end round even with both leaders unused");
  game.ResetEmpty();
  EnableLeader(game, 0, Leader::kEmhyr);
  EnableLeader(game, 1, Leader::kFoltestClear);
  game.FinishTurn();
  Expect(game.round_result_ == GameResult::kDraw,
         "equal HP exhaustion draws with unused leaders");
  // Reproduce the reported loss: lose the first round, then draw at 30 HP.
  game.ResetEmpty();
  game.players_[0].hp = 0;
  game.players_[1].hp = 5;
  game.FinishTurn();
  Expect(game.round_result_ == GameResult::kEnemyWin &&
             game.players_[0].crystals == 1 && game.players_[1].crystals == 2,
         "first loss leaves crystals at one versus two");
  Expect(game.NextRound() && game.players_[0].hp == 30 &&
             game.players_[1].hp == 30,
         "next round restores both HP equally");
  game.FinishTurn();
  Expect(game.round_result_ == GameResult::kDraw &&
             game.result_ == GameResult::kEnemyWin &&
             game.players_[0].crystals == 0 && game.players_[1].crystals == 1,
         "equal HP draws the round; prior crystal deficit loses the match");
  game.FinishTurn();
  Expect(game.players_[0].hp == 30 && game.players_[1].hp == 30 &&
             game.players_[0].crystals == 0 && game.players_[1].crystals == 1,
         "finished match preserves final HP and scores each round once");
  game.ResetEmpty();
  const int last_medic =
      game.Add(0, CardType::kMedic, 2, CardZone::kHand, Lane::kRanged);
  const int recovered =
      game.Add(0, CardType::kDamage, 1, CardZone::kDiscard, Lane::kMelee);
  game.Play(0, last_medic, Lane::kRanged);
  ResolveTurn(game);
  Expect(game.round_result_ == GameResult::kPlaying &&
             game.cards_[recovered].zone == CardZone::kHand,
         "final effect cycle recovery prevents premature hand exhaustion");
  game.ResetEmpty();
  game.players_[0].hp = 0;
  game.CheckLethal();
  game.FinishTurn();
  game.NextRound();
  game.players_[1].hp = 0;
  game.CheckLethal();
  game.FinishTurn();
  Expect(game.result_ == GameResult::kPlaying &&
             game.players_[0].crystals == 1 && game.players_[1].crystals == 1,
         "split rounds require third");
  game.NextRound();
  game.players_[0].hp = 0;
  game.CheckLethal();
  game.FinishTurn();
  Expect(game.result_ == GameResult::kEnemyWin && game.round_ == 3,
         "third-round winner wins match");
  game.ResetEmpty();
  KeepPlaying(game);
  const int board =
      game.Add(0, CardType::kDamage, 3, CardZone::kBoard, Lane::kMelee);
  const int top0 =
      game.Add(0, CardType::kShield, 2, CardZone::kDeck, Lane::kSiege);
  const int top1 =
      game.Add(1, CardType::kHeal, 2, CardZone::kDeck, Lane::kRanged);
  const int discard =
      game.Add(0, CardType::kFog, 2, CardZone::kDiscard, Lane::kRanged);
  game.players_[0].shield = 7;
  game.players_[0].leader = Leader::kEmhyr;
  game.players_[0].leader_used = true;
  const std::size_t hand0 = game.players_[0].hand.size(),
                    hand1 = game.players_[1].hand.size();
  game.players_[1].hp = 0;
  game.CheckLethal();
  game.FinishTurn();
  Expect(game.NextRound(), "round transition accepted");
  Expect(game.players_[0].hp == 30 && game.players_[1].hp == 30 &&
             game.players_[0].shield == 0 && game.players_[0].board.empty() &&
             game.cards_[board].zone == CardZone::kDiscard &&
             game.cards_[discard].zone == CardZone::kDiscard,
         "transition clears board and shield while preserving discard");
  Expect(game.players_[0].hand.size() == hand0 + 1 &&
             game.players_[1].hand.size() == hand1 + 1 &&
             game.cards_[top0].zone == CardZone::kHand &&
             game.cards_[top1].zone == CardZone::kHand,
         "round two draws exactly one per preserved hand");
  Expect(game.players_[0].leader_used &&
             game.players_[0].leader == Leader::kEmhyr && !game.NextRound(),
         "leader use persists and active round cannot advance");
  game.ResetEmpty();
  EnableLeader(game, 0, Leader::kEmhyr);
  Expect(!game.Commit(0, Action(Action::kSkip)),
         "cannot skip an available leader");
  game.ResetEmpty();
  game.Add(0, CardType::kDamage, 2, CardZone::kBoard, Lane::kMelee);
  game.players_[1].hp = 29;
  Expect(game.Commit(0, Action(Action::kSkip)) &&
             game.Commit(1, Action(Action::kSkip)),
         "both unavailable sides skip");
  Expect(!game.AnyCommitted(), "two skips do not trigger an effect cycle");
  game.FinishTurn();
  Expect(
      game.round_result_ == GameResult::kPlayerWin && game.players_[1].hp == 29,
      "exhaustion settles HP without extra activation");
  game.ResetEmpty();
  game.Add(0, CardType::kSpy, 2, CardZone::kBoard, Lane::kMelee);
  game.Add(0, CardType::kDamage, 2, CardZone::kBoard, Lane::kMelee);
  const int enemy_play =
      game.Add(1, CardType::kDamage, 1, CardZone::kHand, Lane::kMelee);
  const int stolen =
      game.Add(1, CardType::kHeal, 1, CardZone::kHand, Lane::kRanged);
  Expect(game.Commit(0, Action(Action::kSkip)),
         "unavailable side automatically skips");
  Expect(game.Play(1, enemy_play, Lane::kMelee),
         "opponent acts while side skips");
  ResolveTurn(game);
  Expect(
      game.cards_[stolen].owner == 0 && game.CanPlay(0, stolen, Lane::kRanged),
      "spy recovery enables skipped side");
  Expect(game.players_[1].hp == 28 && game.players_[0].hp == 29,
         "both boards resolve when only one side acts");
  game.ResetEmpty();
  game.Add(0, CardType::kDecoy, 1, CardZone::kHand);
  game.Add(1, CardType::kDecoy, 1, CardZone::kHand);
  game.FinishTurn();
  Expect(game.round_result_ == GameResult::kDraw,
         "two unusable decoys cannot stall exhaustion");
  game.Reset(7);
  Expect(game.turn_ == 1 && game.round_ == 1 &&
             game.result_ == GameResult::kPlaying &&
             game.round_result_ == GameResult::kPlaying &&
             game.players_[0].hp == 30 && game.players_[0].shield == 0 &&
             game.players_[0].crystals == 2 && !game.players_[0].leader_used &&
             game.players_[0].discard.empty() && game.HasValidState(),
         "restart clears all prior match state");
}

void SeededMatches() {
  section = "seeded full matches";

  for (unsigned seed = 0; seed < 1000; ++seed) {
    Game game(seed);
    int decisions = 0;

    while (game.result_ == GameResult::kPlaying) {
      if (game.round_result_ != GameResult::kPlaying) {
        Expect(game.NextRound(), "seeded next round");
        Expect(game.round_ <= 3, "at most three rounds");
      }

      for (int owner = 0; owner < 2; ++owner) {
        const Action action = game.ChooseAction(owner);
        Expect(game.CanAct(owner, action) && game.Commit(owner, action),
               "random action legal and committed");
        Expect(game.HasValidState(), "invariant after random action");
      }

      if (game.AnyCommitted()) {
        ResolveTurn(game);
      } else {
        game.FinishTurn();
      }

      for (const Player& player : game.players_) {
        Expect(player.hp >= 0 && player.hp <= Game::kMaxHp &&
                   player.shield >= 0 && player.crystals >= 0 &&
                   player.crystals <= 2,
               "bounded HP, crystals and shield");
      }

      if (++decisions > 2000) {
        std::ostringstream message;
        message << "seed " << seed << " exceeded test watchdog at round "
                << game.round_ << ", turn " << game.turn_;
        throw std::runtime_error(message.str());
      }
    }

    Expect(game.HasValidState() && (game.players_[0].crystals == 0 ||
                                    game.players_[1].crystals == 0),
           "completed match invariant and final crystal");
  }
}
}  // namespace

int main() {
  try {
    OpeningAndLegality();
    CoreRecurrence();
    WeatherAndClear();
    ScorchAndHorn();
    Recovery();
    EnemyResponses();
    Leaders();
    RoundsAndExhaustion();
    SeededMatches();
    std::cout << "PASS: " << checks
              << " checks, including all advanced rules and 1000 complete "
                 "seeded matches\n";
  } catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n';

    return 1;
  }
}

#endif
