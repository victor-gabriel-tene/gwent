#ifndef LAB_M1_TEMA1_GAME_CONFIG_H_
#define LAB_M1_TEMA1_GAME_CONFIG_H_

namespace tema1 {

// Gameplay choices shared by the rules, scene, and indicators.
constexpr int kPlayerCount = 2;
constexpr int kHumanPlayer = 0;
constexpr int kEnemyPlayer = 1;
constexpr int kStartingHp = 30;
constexpr int kStartingCrystals = 2;
constexpr int kMaxRounds = kPlayerCount * (kStartingCrystals - 1) + 1;
constexpr int kOpeningHandSize = 10;
constexpr int kCardsPerRound = 1;
constexpr int kLeaderRevealCount = 3;
constexpr int kValueMultiplier = 2;
constexpr int kNoCard = -1;
constexpr unsigned kDefaultSeed = 1;
constexpr int kTemporaryDecay = 1;
constexpr int kWeatherReduction = 1;
constexpr int kCrystalsPerLoss = 1;
constexpr int kFirstRound = 1;
constexpr int kFirstTurn = 1;
constexpr int kNoTurn = -1;
constexpr int kNoOwner = -1;

constexpr int OpponentOf(int owner) {
  return kHumanPlayer + kEnemyPlayer - owner;
}

}  // namespace tema1

#endif  // LAB_M1_TEMA1_GAME_CONFIG_H_
