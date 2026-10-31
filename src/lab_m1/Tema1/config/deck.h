#ifndef LAB_M1_TEMA1_CONFIG_DECK_H_
#define LAB_M1_TEMA1_CONFIG_DECK_H_

#include <array>

#include "lab_m1/Tema1/game_types.h"

namespace tema1 {

struct DeckGroup {
  CardType type;
  int first_value;
  int last_value;
  int copies_per_value;
  Lane lane;
};

// kNone assigns unit lanes in round-robin order; kAny leaves specials flexible.
constexpr std::array<DeckGroup, kCardTypeCount> kDeckRecipe = {{
    {CardType::kDamage, 1, 3, 4, Lane::kNone},
    {CardType::kHeal, 1, 2, 2, Lane::kNone},
    {CardType::kShield, 1, 2, 2, Lane::kNone},
    {CardType::kMedic, 2, 2, 2, Lane::kRanged},
    {CardType::kSpy, 2, 2, 1, Lane::kAny},
    {CardType::kFrost, 2, 2, 1, Lane::kAny},
    {CardType::kFog, 2, 2, 1, Lane::kAny},
    {CardType::kRain, 2, 2, 1, Lane::kAny},
    {CardType::kHorn, 2, 2, 1, Lane::kAny},
    {CardType::kScorch, 2, 2, 1, Lane::kAny},
    {CardType::kClear, 1, 1, 1, Lane::kAny},
    {CardType::kDecoy, 1, 1, 1, Lane::kAny},
}};

}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_DECK_H_
