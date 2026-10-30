# Tema1 — GWENT: The Card Table

A C++11/OpenGL implementation of the HP-based game from
[EGC Tema 1, 2026](https://ocw.cs.pub.ro/courses/egc/teme/2026/01), including
three battlefield lanes, all twelve card types, five selectable leaders, passing
and matches of up to three rounds. The walnut/brass table, cards, symbols and
leader shapes use procedural geometry; no downloaded artwork is required.

## Build and run

From the repository root, with the framework's existing dependencies:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j 6
./build/release/bin/Release/GFXFramework
```

The Debug configuration remains available with `cmake -S . -B build`,
`cmake --build build -j 6`, and `./build/bin/Debug/GFXFramework`.
Keep the generated `assets` link beside the executable intact.

Set `TEMA1_SEED` to an unsigned integer for a reproducible shuffled deal, opposing
leader and random choices. Repeat the same player actions to replay a match:

```sh
TEMA1_SEED=42 ./build/release/bin/Release/GFXFramework
```

## Controls

| Control | Action |
| --- | --- |
| Click a leader in the opening menu | Select it; its matching silhouette grows behind it |
| Click the selected leader again | Begin the match against a random opposing leader |
| Hold left mouse on a hand card | Pick up and drag, preserving the grab offset |
| Release on a highlighted friendly lane | Play the card if that lane is legal |
| Release a Decoy over a friendly unit | Return that unit to hand and put the Decoy in its place |
| Release outside a legal destination | Animate back to hand without spending the action |
| Backspace while dragging | Cancel the drag |
| Click your leader panel | Use its once-per-match power instead of playing a card |
| P, or Pass Round | Pass for the rest of the current round |
| Hover over a visible card or leader | Show its description; cards also receive an enlarged sidebar preview |
| H, or the Rules button during a match | Open/close the rules overlay and pause turn progression |
| Click the rules overlay | Close it |
| Continue, Enter or Space in Emhyr's inspection | Resume after reading the revealed cards |
| Next Round, or Enter after a round result | Clear the board, reset HP/shield and draw one card each |
| R, or Play Again after the match result | Return to leader selection for a new match |
| Escape | Exit, using the framework control |

Only the player hand and revealed enemy hand cards expose their faces and tooltips.
The opponent places ordinary cards face down, then flips them before resolution.

## Cards and lanes

Both players begin with 30 HP, zero shield, two crystals and a ten-card opening
hand from a shuffled 30-card deck. Each battlefield half has Melee, Ranged and
Siege lanes. A card's upper-right badge shows its permitted lane: **M**, **R**,
**S**, or **\*** for any lane. Units have fixed printed lanes; Medic is Ranged.
Frost, Fog and Rain occupy their corresponding lane. Other specials accept any
friendly lane, subject to the Decoy target requirement.

| Type | Cards per deck / printed V | Behavior |
| --- | --- | --- |
| Damage, red triangle | 12: four each at 1, 2, 3 | Deal V damage each turn |
| Heal, green diamond | 4: two each at 1, 2 | Restore V HP each turn, capped at 30 |
| Shield, blue circle | 4: two each at 1, 2 | Add V shield each turn |
| Medic, green cross | 2 at 2 | On play, recover one random discarded card to hand at printed V |
| Spy, black paired diamonds | 1 at 2 | Steal one random opposing hand card each turn |
| Frost, white snowflake | 1 at 2 | Reduce every opposing Melee board card by 1 each turn |
| Fog, gray horizontal lines | 1 at 2 | Reduce every opposing Ranged board card by 1 each turn |
| Rain, blue cloud and drops | 1 at 2 | Reduce every opposing Siege board card by 1 each turn |
| War Horn, orange horn | 1 at 2 | Arm a bonus that doubles the card played on the following turn |
| Scorch, purple flame | 1 at 2 | Suppress the opposing card played this turn if its effective V is at least Scorch's |
| Clear Sky, yellow sun | 1 at 1 | Remove all Frost, Fog and Rain from both boards |
| Decoy, brown dummy | 1 at 1 | On play, return a selected friendly unit to hand at printed V |

Damage, Heal, Shield and Medic count as units for Decoy targeting. The Medic
remains on the board after its one-time recovery. A Decoy remains as an inert
board card and cannot itself be a Decoy target. A Medic with no discard target
still uses the action. Returning or recovering a card restores printed V and
clears its previous board status.

Spy, weather, Clear Sky, War Horn and Scorch are temporary. Each loses one V
after its activation, including suppressed activations or activations with no
available target. Temporary V controls lifetime, except that Scorch also uses V
as its cancellation threshold. Any board card reduced to V <= 0 is discarded.
Square/diamond symbol frames distinguish permanent and temporary categories.
Current effective V appears as a numeral and individual geometric units. Hover
previews show the effective V and distinguish base/lifetime and printed values
when boosted. Boosted cards receive a green value/frame; suppressed cards receive
a crossed-out symbol.

## Leaders

A leader power costs the primary action and can be used **once per match**.
It remains spent in later rounds. All leader effects resolve before card effects.

| Leader / geometric shape | Power |
| --- | --- |
| Foltest: Clear Sky / circle | Remove all weather from both boards |
| Emhyr / diamond | Reveal up to three random enemy hand cards |
| Eredin / hexagon | Double effective values of friendly Melee cards for this turn |
| Francesca / triangle | Destroy one strongest opposing Melee card, choosing randomly among tied cards |
| Foltest: Siege Horn / star | Double a Siege card played on the next turn; the opportunity expires that turn |

Emhyr opens a paused inspection of only the revealed cards. Those cards stay
visible in the enemy hand while that knowledge remains valid. Unrevealed cards
share identical backs. Eredin modifies effective V without permanently changing
the cards' stored V.

## Turn, round and match rules

Each side takes one primary action per turn: play a card, use its unused leader,
or pass. The enemy randomly chooses among its legal actions, with card and lane
choices kept separate. A passed player takes no further primary actions in that
round; its surviving board cards continue to activate while the opponent acts.
A player with no usable card and no unused leader skips automatically. A Skip
can become playable again after Spy or recovery; an explicit Pass stays locked.

The shared phase order is:

**Leader -> Scorch -> Medic / Decoy -> Spy -> Clear Sky -> Weather -> War Horn -> Shield -> Damage -> Heal.**

- Recurring effects include surviving cards from earlier turns. Medic and Decoy
  resolve their utility only on the turn they are played.
- Same-phase effects use phase-start snapshots and resolve symmetrically. Spies
  choose distinct targets from the hands after both primary actions, without
  selecting cards just transferred by another Spy in that phase.
- All three weather types resolve together. They reduce every opposing board
  type in the matching lane. Each weather source also loses one lifetime V.
  Clear Sky removes weather before weather resolution and does not restore V
  lost on earlier turns.
- Scorch suppresses the newly played opposing card for this turn without
  destroying it. Suppressed temporary cards still decay. Scorch sources evaluate
  the same phase snapshot, so opposing Scorches can suppress one another.
- War Horn arms the following turn's played-card bonus on each activation.
  Multiple horns do not stack. The bonus expires on that following turn even
  if its owner chooses another action or plays outside Foltest's Siege condition.
- Shield has no gameplay cap, persists between turns and survives destruction of
  its source. Damage consumes shield before HP. Damage is simultaneous; check
  lethal before Heal, so healing cannot revive a defeated player.
- Zero HP loses the round; simultaneous lethal draws it. Once both sides have
  passed or have no available primary action, compare HP. Equal HP draws the
  round; shield is not a tiebreaker. No extra effect cycle runs solely because
  both sides passed or skipped.

Losing a round costs one crystal. A drawn round costs both sides one crystal.
At zero crystals a player loses the match; both reaching zero together draws it.
A match therefore ends within three rounds.

At each new round, all board cards enter their owners' discard piles, both sides
return to 30 HP and zero shield, Pass and pending turn bonuses clear, and each
player draws one card if its deck is not empty. Remaining hands, deck order,
discard piles and spent leader powers carry over. No ordinary card draw occurs
between turns.

## Implementation map

- `GameRules.h`: graphics-independent cards, lanes, legal actions, deck creation,
  effect phases, leaders, round/match outcomes and ownership invariants.
- `Tema1.h` / `Tema1.cpp`: scene state machine, shared hit rectangles, dragging,
  stable animated layouts, reveal/inspection flow and round transitions.
- `Tema1Presentation.inl`: procedural cards and leaders, six lane rows, drop
  highlights, status/crystals, menus and overlays.
- `BatchRenderer.h` / `.cpp`: painter-ordered geometry/text batch, immutable font
  atlas, rotating upload buffers, cached text and retained frames.
- `tests/rules_tests.cpp`: deterministic rule scenarios and 1,000 seeded matches.
- `tests/SceneChecks.inl`: opt-in tests through the actual scene handlers and GPU.
- `tests/PerformanceChecks.h`: opt-in wall-clock frame/render benchmark.

The scene uses a uniformly scaled 1600x900 logical canvas with letterboxing and
Retina-aware inverse pointer mapping. Each lane centers and scales its own cards.
Stable card IDs preserve animation continuity through hand transfers and recovery.
The same lane geometry drives rendering, legal drop highlighting and hit tests.

All geometry and text uses one ordered draw call per frame. Idle frames reuse
uploaded geometry. Changed frames use rotating buffers with replacement stores;
there is no per-character buffer overwrite or per-shape uniform lookup. Cached
card faces include every visible lane, V, boost and suppression distinction.
Animations use elapsed wall time. Rendering remains local to Tema1.

## Verification

Standalone rules checks require only a C++11 compiler:

```sh
clang++ -std=c++11 -Wall -Wextra -pedantic -fsanitize=address,undefined \
  -DTEMA1_RULE_TESTS src/lab_m1/Tema1/tests/rules_tests.cpp \
  -o /tmp/tema1-rules-tests
/tmp/tema1-rules-tests
```

They cover deck composition, lane legality, one-action enforcement, recurrence,
weather snapshots, cancellation, Horn expiry, recovery, all five leaders, hidden
information, passing, crystal loss, round persistence and 1,000 complete seeded
matches. Ownership/value invariants are checked after resolution phases.

For application-level checks, build the game and create an output directory:

```sh
mkdir -p /tmp/tema1-ui-checks
TEMA1_VERIFY_DIR=/tmp/tema1-ui-checks ./build/bin/Debug/GFXFramework
```

The opt-in scene driver exercises input handlers, lane and target legality,
advanced cards, leader selection/use, hidden information, recovery, resizing,
round transitions and complete-match restart. It saves 20+ GPU captures as PPM
images, prints a PASS message and exits. It does not run during ordinary play.

To measure an idle, fully dealt game:

```sh
TEMA1_BENCHMARK_FILE=/tmp/tema1-performance.json ./build/bin/Debug/GFXFramework
```

Benchmark mode automatically selects Foltest: Clear Sky and completes the
opening deal without menu input. It then uses one second of warmup and six
seconds of idle sampling before exiting. To force rebuilding/uploading on every
frame:

```sh
TEMA1_BENCHMARK_REBUILD=1 TEMA1_BENCHMARK_FILE=/tmp/tema1-rebuild.json \
  ./build/bin/Debug/GFXFramework
```

JSON reports FPS, mean/p95 frame interval, rendering CPU time and framebuffer
size. Use the same window, build configuration and power settings for comparisons
and run one game instance at a time. Frame intervals include presentation/vsync;
CPU submission time does not measure the full asynchronous GPU frame time.

Advanced-scene measurements on the development machine, at a 2560x1440
framebuffer with `TEMA1_SEED=42`:

| Configuration | Rendered FPS | Mean rendering CPU time |
| --- | ---: | ---: |
| Debug, stationary table | 60.00 | 0.095 ms |
| Debug, forced rebuild/upload every frame | 60.00 | 2.690 ms |
| Release, stationary table | 60.00 | 0.128 ms |

These are short wall-clock samples, not a fixed FPS guarantee. Display timing
and scheduling affect frame intervals. The rebuild run verifies that performance
does not depend only on the retained idle frame.

## Submission notes

Select `m1::Tema1` in `src/main.cpp`. Existing CMake source discovery and the scene
include support Tema1 without additional libraries. Keep source and required
resources, exclude executables/build clutter, and keep the coursework archive
below 50 MB. The in-person presentation should explain transforms, pointer
mapping, batched rendering, legal actions, phase snapshots, cancellation/lifetime,
card ownership and state carried between rounds.
