#include "lab_m1/Tema1/game_types.h"

namespace tema1 {
namespace {
struct PhaseLabel {
  const char* heading;
  const char* summary;
};

const std::array<PhaseLabel, kEffectPhaseCount> kPhaseLabels = {{
    {"LEADERS", "Leader"},
    {"SCORCH", "Scorch"},
    {"MEDIC / DECOY", "Medic / Decoy"},
    {"SPY", "Spy"},
    {"CLEAR SKY", "Clear"},
    {"WEATHER", "Weather"},
    {"WAR HORN", "Horn"},
    {"SHIELD", "Shield"},
    {"DAMAGE", "Damage"},
    {"HEAL", "Heal"},
}};
}  // namespace

const std::array<EffectPhase, kEffectPhaseCount>& EffectPhases() {
  static const std::array<EffectPhase, kEffectPhaseCount> phases = {
      {EffectPhase::kLeaders, EffectPhase::kScorch, EffectPhase::kUtility,
       EffectPhase::kSpy, EffectPhase::kClear, EffectPhase::kWeather,
       EffectPhase::kHorn, EffectPhase::kShield, EffectPhase::kDamage,
       EffectPhase::kHeal}};

  return phases;
}

const char* PhaseName(EffectPhase phase) {
  return kPhaseLabels[static_cast<int>(phase)].heading;
}

std::string ResolutionOrder() {
  std::string summary;

  for (EffectPhase phase : EffectPhases()) {
    if (!summary.empty()) {
      summary += " > ";
    }

    summary += kPhaseLabels[static_cast<int>(phase)].summary;
  }

  return summary;
}

const char* CardName(CardType type) {
  static const char* names[] = {"DAMAGE",   "HEAL",      "SHIELD", "SPY",
                                "FROST",    "CLEAR SKY", "FOG",    "RAIN",
                                "WAR HORN", "SCORCH",    "MEDIC",  "DECOY"};

  return names[static_cast<int>(type)];
}

const char* LaneName(Lane lane) {
  if (lane == Lane::kMelee) {
    return "MELEE";
  }

  if (lane == Lane::kRanged) {
    return "RANGED";
  }

  if (lane == Lane::kSiege) {
    return "SIEGE";
  }

  return lane == Lane::kAny ? "ANY LANE" : "NO LANE";
}

const char* LaneSymbol(Lane lane) {
  switch (lane) {
    case Lane::kMelee:
      return "M";
    case Lane::kRanged:
      return "R";
    case Lane::kSiege:
      return "S";
    default:
      return "*";
  }
}

const char* LeaderName(Leader leader) {
  static const char* names[] = {"FOLTEST: CLEAR SKY", "EMHYR", "EREDIN",
                                "FRANCESCA", "FOLTEST: SIEGE HORN"};
  const int index = static_cast<int>(leader);

  return index >= 0 && index < static_cast<int>(Leader::kCount) ? names[index]
                                                                : "NO LEADER";
}

const char* LeaderDescription(Leader leader) {
  static const char* descriptions[] = {
      "Remove every weather card on both sides.",
      "Reveal up to three random cards in the opposing hand.",
      "Double your melee cards' effective values for this turn.",
      "Destroy one strongest opposing melee card; break ties randomly.",
      "Double your next turn's siege card. The opportunity expires that turn."};
  const int index = static_cast<int>(leader);

  return index >= 0 && index < static_cast<int>(Leader::kCount)
             ? descriptions[index]
             : "";
}

const char* CardDescription(CardType type) {
  static const std::string descriptions[] = {
      "Deal V damage each turn, after shields.",
      "Restore V HP each turn, up to " + std::to_string(kStartingHp) +
          ". Lethal damage cannot be healed.",
      "Add V shield each turn. Shield carries between turns.",
      "Steal one random opposing hand card each turn; lose one V.",
      "Reduce opposing melee cards by one each turn; lose one V.",
      "Remove all weather on both sides; lose one V each turn.",
      "Reduce opposing ranged cards by one each turn; lose one V.",
      "Reduce opposing siege cards by one each turn; lose one V.",
      "Double your next turn's played card; lose one V. Horns do not stack.",
      "Cancel the opposing card played this turn if its V is at least this V; "
      "lose one V.",
      "On play, return a random discarded card to your hand at printed V.",
      "Play over a friendly board card to return it at printed V. Decoy stays "
      "inert."};

  return descriptions[static_cast<int>(type)].c_str();
}

bool IsWeather(CardType type) {
  return type == CardType::kFrost || type == CardType::kFog ||
         type == CardType::kRain;
}

Lane WeatherLane(CardType type) {
  return type == CardType::kFrost ? Lane::kMelee
         : type == CardType::kFog ? Lane::kRanged
                                  : Lane::kSiege;
}

bool IsTemporary(CardType type) {
  return type == CardType::kSpy || IsWeather(type) ||
         type == CardType::kClear || type == CardType::kHorn ||
         type == CardType::kScorch;
}

bool IsUnit(CardType type) {
  return type == CardType::kDamage || type == CardType::kHeal ||
         type == CardType::kShield || type == CardType::kMedic;
}

CardType EffectType(EffectPhase phase) {
  if (phase == EffectPhase::kScorch) {
    return CardType::kScorch;
  }

  if (phase == EffectPhase::kSpy) {
    return CardType::kSpy;
  }

  if (phase == EffectPhase::kClear) {
    return CardType::kClear;
  }

  if (phase == EffectPhase::kWeather) {
    return CardType::kFrost;
  }

  if (phase == EffectPhase::kHorn) {
    return CardType::kHorn;
  }

  if (phase == EffectPhase::kShield) {
    return CardType::kShield;
  }

  if (phase == EffectPhase::kHeal) {
    return CardType::kHeal;
  }

  return CardType::kDamage;
}

}  // namespace tema1
