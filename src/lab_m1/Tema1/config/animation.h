#ifndef LAB_M1_TEMA1_CONFIG_ANIMATION_H_
#define LAB_M1_TEMA1_CONFIG_ANIMATION_H_

namespace tema1 {
namespace ui {

// Elapsed-time durations, in seconds. The flip uses the same duration as its
// state.
constexpr float kMoveDuration = 0.22f;
constexpr float kDefaultAnimationDuration = 0.25f;
constexpr float kVanishDuration = 0.28f;
constexpr float kInvalidDropDuration = 0.26f;
constexpr float kRevealDuration = 0.35f;
constexpr float kInitialDealDelay = 0.18f;
constexpr float kRoundDealDelay = 0.35f;
constexpr float kDrawDuration = 0.30f;
constexpr float kLastDealDelay = 0.40f;
constexpr float kDealInterval = 0.14f;
constexpr float kBeforeResolveDelay = 0.15f;
constexpr float kAfterRevealDelay = 0.20f;
constexpr float kEmptyPhaseDelay = 0.08f;
constexpr float kPhaseDelay = 0.60f;
constexpr float kPlayerPlaceDelay = 0.30f;
constexpr float kEnemyPlaceDelay = 0.34f;
constexpr float kSkipDelay = 0.40f;
constexpr float kHitFlashDuration = 0.60f;
constexpr float kMinimumAnimationDuration = 0.001f;
constexpr float kMinimumFlipScale = 0.015f;

}  // namespace ui
}  // namespace tema1

#endif  // LAB_M1_TEMA1_CONFIG_ANIMATION_H_
