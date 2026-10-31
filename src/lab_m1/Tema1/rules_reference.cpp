#include "lab_m1/Tema1/rules_reference.h"

#include "lab_m1/Tema1/game_config.h"

namespace tema1 {
namespace text {

const std::vector<RuleSection>& RuleSections() {
  static const std::vector<RuleSection> sections = {
      {"CARD VALUES",
       {"Squares show the current value.",
        "Circle: permanent. Triangle: temporary.",
        "Temporary cards lose 1 V per activation.",
        "At 0 V, a card is removed."}},
      {"ROUND & MATCH",
       {"0 HP loses a round and one crystal.",
        "Both hands empty, or no actions: compare HP.",
        "Ties cost both players one crystal.",
        "Lose both crystals to lose the match."}},
      {"NEXT ROUND",
       {"Reset to " + std::to_string(kStartingHp) + " HP and no shield.",
        "Keep your hand; draw one card.", "Leader powers work once per match.",
        "Used leaders turn grey."}},
  };

  return sections;
}

}  // namespace text
}  // namespace tema1
