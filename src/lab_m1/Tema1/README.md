# Tema1 — GWENT: The Card Table

A C++11/OpenGL implementation of the HP-based game from
[EGC Tema 1, 2026](https://ocw.cs.pub.ro/courses/egc/teme/2026/01), including
three battlefield lanes, all twelve card types, five selectable leaders
and matches of up to three rounds. The flat board, cards, symbols and leader
shapes use simple meshes and the framework's supplied rendering helpers.

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
| Release a Decoy over a friendly board card | Return that card to hand and put the Decoy in its place |
| Release outside a legal destination | Animate back to hand without spending the action |
| Backspace while dragging | Cancel the drag |
| Click your leader panel | Use its once-per-match power instead of playing a card |
| Hover over a visible card or leader | Show its description in the right margin; cards also receive an enlarged preview |
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
Siege lanes. A card's upper-right letter shows its permitted lane: **M**, **R**,
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
| Decoy, brown dummy | 1 at 1 | On play, return a selected friendly board card to hand at printed V |

Decoy can replace any friendly board card, including weather, other specials and
an existing Decoy. It takes the target's lane and position. A Decoy remains inert
after its one-time swap. Medic recovers once when played and remains on the board;
returning it with Decoy and replaying it triggers another recovery. An empty
discard pile makes Medic a safe no-op. Returning or recovering a card restores
printed V and clears its previous board status.

Spy, weather, Clear Sky, War Horn and Scorch are temporary. Each loses one V
after its activation, including suppressed activations or activations with no
available target. Temporary V controls lifetime, except that Scorch also uses V
as its cancellation threshold. Any board card reduced to V <= 0 is discarded.
Small circle/triangle markers distinguish permanent and temporary categories.
Current effective V appears as individual geometric units down the left edge. Hover
previews show the effective V and distinguish base/lifetime and printed values
when boosted. Boosted cards receive green value units; suppressed cards receive
a crossed-out symbol.

## Leaders

A leader power costs the primary action and can be used **once per match**.
It remains spent in later rounds. All leader effects resolve before card effects.
Spent leaders are shown in neutral grey.

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

Each side takes one primary action per turn: play a card or use its unused leader.
The enemy always chooses a random playable card when one is available,
then chooses a legal lane and any Decoy target separately. With no playable card,
it uses its unused leader or skips. A player with no usable card and no unused
leader skips automatically; its board still activates when the opponent acts.
Spy or recovery can make that player able to act again. Voluntary passing is
not part of this implementation.

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
- Zero HP loses the round; simultaneous lethal draws it. After the effect cycle,
  both hands being empty ends the round even if leaders remain unused. Recovery
  or stealing during that cycle can prevent exhaustion. Both sides having no
  available action also ends the round. Compare HP; equal HP draws and
  shield is not a tiebreaker. Two skips trigger no extra effects.

Losing a round costs one crystal. A drawn round costs both sides one crystal.
At zero crystals a player loses the match; both reaching zero together draws it.
A match therefore ends within three rounds.
Equal HP draws the current round, while the match still depends on crystals.
For example, a prior loss leaves crystals at 1:2; a drawn round then makes it
0:1 and loses the match, even if both players finish that round at 30 HP.

At each new round, all board cards enter their owners' discard piles, both sides
return to 30 HP and zero shield, pending turn bonuses clear, and each
player draws one card if its deck is not empty. Remaining hands, deck order,
discard piles and spent leader powers carry over. No ordinary card draw occurs
between turns.

## Implementation map

| Files | Responsibility |
| --- | --- |
| `game_config.h` | Starting HP/crystals, hand sizes, round limits and shared rule values |
| `game_types.h` / `.cpp` | Cards, players, actions, events, names, descriptions and phase order |
| `game_rules.h` / `.cpp` | Match state, deck recipe, legal actions, ownership, turns and rounds |
| `game_effects.cpp` | Leader powers and individual card resolution phases |
| `Tema1.h` / `Tema1.cpp` | Framework scene, initialization and turn/round state machine |
| `input.cpp` | Mouse/keyboard callbacks, drag cancellation and legal drops |
| `layout.cpp` | Shared rectangles, pointer mapping, centered card layouts and animation |
| `drawing.cpp` | Basic meshes, shape transforms and text drawing helpers |
| `cards.cpp` | Card faces, geometric symbols and hover previews |
| `board.cpp` | Battlefield, legal drop highlights and status indicators |
| `leaders.cpp` | Leader selection, panels and hover descriptions |
| `symbols.cpp` | Small drawing functions for each card and leader symbol |
| `meshes.cpp` | Basic unit meshes and triangle indices |
| `rendering.cpp` | Scene drawing order and card reveal/destruction rendering |
| `geometry.h` | Shared rectangles, centers, expansion and text placements |
| `rules_reference.h` / `.cpp` | Rules reference sections as content data |
| `overlays.cpp` | Rules, results and revealed-card popups |
| `config/` and `palette.cpp` | Canvas/layout dimensions, animation timing, assets and palette |
| `text_renderer.h` / `.cpp` | Existing glyph drawing buffer adapter |
| `tests/rules_tests.cpp` | Deterministic rule scenarios and 1,000 seeded matches |
| `tests/scene_checks.cpp` | Opt-in checks through the actual input handlers and GPU |

Implementations compile as ordinary `.cpp` files; no source fragments are
included by another source file. Rules depend only on the C++11 standard
library. The scene keeps the same simple callbacks, meshes and transforms used
in labs 1–4.

Runtime code reads named values from focused configuration files:

- `config/cards.h`: face markings, value indicators, selection frame and preview.
- `config/symbols.h`: named proportions for every card and leader symbol.
- `config/board.h`, `config/leaders.h`, `config/overlays.h`: screen-specific layout.
- `config/scene.h`: base canvas, card dimensions, shared margins and derived positions.
- `config/animation.h`: animation durations and reveal timing.
- `config/palette.h` and `palette.cpp`: shared colors and type/leader palettes.
- `config/drawing.h`: mesh topology, glyph buffer layout and drawing settings.
- `config/deck.h`: card groups, values, copies and allowed lanes.
- `config/text.h` and `rules_reference.cpp`: UI labels and rules content.

Dependent values are calculated from their source: card aspect ratios, hand and
board spacing, enemy hand position, centered health/shield segments, leader menu
spacing and preview sizes. The rules reference iterates its sections and lines;
there is no fixed three-by-four rendering loop.

Numeric literals belong in these data definitions. Rendering and game-flow
functions use names and derived geometry. Ordinary mathematical identities,
indices, empty/zero states and the coefficients of the named cubic smoothstep
formula remain directly expressed. Test fixtures retain explicit expected values
so they independently check the configured defaults.

The local `.clang-format` uses Google's C++ style with two-space indentation,
80-column lines and braces around control-flow bodies. Blank lines separate
guard clauses, setup, drawing stages, and state changes; the formatter preserves
this logical spacing. Variables use
`snake_case`, class members end in `_`, and constants/enumerators use `kNames`.
`Tema1.h` and `Tema1.cpp` keep the framework's existing scene name; the other
filenames use lowercase words. Format only this homework from the repository root:

```sh
clang-format -i src/lab_m1/Tema1/*.h src/lab_m1/Tema1/*.cpp \
  src/lab_m1/Tema1/config/*.h src/lab_m1/Tema1/tests/*.cpp
```

The scene uses a uniformly scaled 1600x900 logical canvas with letterboxing and
Retina-aware inverse pointer mapping. Both hands and every lane center on x=800.
Lane labels sit inside each lane's left margin. Equal space on the right keeps
card groups centered and prevents crowded rows from covering the labels.
A thin frame encloses the central table
with clear space around both hands. Compact matching information rows show
leaders, crystals, HP and shield. Shield has thirty empty segments even at zero,
filling blue as it grows; values over thirty add rows without capping gameplay.
Leader hover uses a warm circular backdrop and slight shape scaling; used leaders
remain grey. The Rules button has a subtle fill, thin brass border and gold hover
highlight. Its overlay separates the card reference, effect order and three short
groups of general rules, with controls in a single footer line.
Round, match and revealed-card popups share a warm panel, thin brass frame,
gold heading, quiet divider and one action button. Result rows place each stat
label between the player's and opponent's values on a shared baseline.
Round results show HP; match results show the remaining crystals instead of HP.
When a drawn round ends the match, the popup identifies the final round as drawn.
Deck rectangles are hidden, with the shared side positions retained
as dealing animation origins. Hover previews use
the empty right margin without covering the battlefield. Each lane scales its own cards.
Stable card IDs preserve animation continuity through hand transfers and recovery.
The same lane geometry drives rendering, legal drop highlighting and hit tests.

### Laboratory concepts used

- **Lab 1:** `SimpleScene` callbacks, elapsed-time updates and mouse/keyboard input.
- **Lab 2:** rectangle, triangle and circle meshes constructed from vertices and
  triangle indices with `Mesh::InitFromData`. Each is created once in `InitMeshes` in `meshes.cpp`.
- **Lab 3:** the completed `transform2D` translation, rotation and scale matrices;
  an orthographic camera; model matrices composed around card/leader centers.
- **Labs 3/4:** viewport coordinates and uniform scaling. The inverse mapping
  converts window mouse coordinates through Retina pixels to logical board space.

`DrawShape` calls the supplied `RenderMesh2D(mesh, modelMatrix, color)` overload for
each element. `DrawBox`, `DrawFrame` and `DrawLine` compose these basic shapes. Draw order,
with depth testing disabled, puts the dragged card and modal screens on top.
Every frame draws the scene directly; there are no draw-list caches. Board and
card geometry use solid RGB colors and the supplied shaders.

Labels use the Tema1-local `text_renderer.h` / `text_renderer.cpp` adapter and existing `Hack-Bold.ttf`.
It inherits the supplied `gfxc::TextRenderer` font loader, glyph textures and
shader, preserving the font and layout. The adapter owns a small drawing buffer
because the framework's buffers are private. Each glyph replaces its complete
buffer data with `glBufferData(..., GL_STREAM_DRAW)` instead of updating the same
busy storage with `glBufferSubData`. This uses the buffer API from Lab 2 and
avoids per-character upload stalls observed on the M1 Mac. No shared framework
files, shaders or font assets are changed. `DrawLabel` converts the bottom-left
logical origin to the helper's top-left origin, while `MeasureTextWidth` uses its
character advances for centering. There is no custom font implementation.

Before/after measurements on the development Mac used the same Debug build
settings, 2560x1440 framebuffer and VSync, with two seconds of warmup and five
seconds of sampling per fixed scene:

| Scene | Original FPS | Updated FPS |
| --- | ---: | ---: |
| Leader selection | 6.0 | 120.0 |
| Table with cards | 4.1 | 103.6 |
| Hover preview | 3.6 | 60.0 |
| Rules overlay | 1.5 | 60.0 |

The four before/after captures were pixel-identical. The existing scene checks
also passed with the updated helper.

Animations interpolate the cards' rectangles using elapsed time. The reveal
animation scales width by `abs(cos(angle))` and switches from back to front at
halfway, the orthographic silhouette of a rotating card. Destruction shrinks the
card; the selected leader is drawn once larger behind its normal shape.

Medic timing, leader action cost and tied-round crystal loss
are documented choices where the assignment leaves details open. Medic is
deliberately once per play, so it cannot endlessly recycle temporary cards each
turn.

## Verification

Standalone rules checks have their own CMake target and require no graphics
libraries:

```sh
cmake -S src/lab_m1/Tema1/tests -B /tmp/tema1-rules-build
cmake --build /tmp/tema1-rules-build
ctest --test-dir /tmp/tema1-rules-build --output-on-failure
```

To run the same suite with address and undefined-behavior sanitizers:

```sh
clang++ -std=c++11 -Wall -Wextra -pedantic -fsanitize=address,undefined \
  -DTEMA1_RULE_TESTS -Isrc \
  src/lab_m1/Tema1/tests/rules_tests.cpp \
  src/lab_m1/Tema1/game_types.cpp src/lab_m1/Tema1/game_rules.cpp \
  src/lab_m1/Tema1/game_effects.cpp -o /tmp/tema1-rules-tests
/tmp/tema1-rules-tests
```

They cover deck composition, lane legality, one-action enforcement, recurrence,
weather snapshots, cancellation, Horn expiry, recovery, all five leaders, hidden
information, forced skipping/exhaustion, crystal loss, round persistence and 1,000 complete seeded
matches. Ownership/value invariants are checked after resolution phases.
Regression scenarios also check every Decoy target type and replacement position,
Medic replay, card-first enemy responses and empty hands with unused leaders.

For application-level checks, build the game and create an output directory:

```sh
mkdir -p /tmp/tema1-ui-checks
TEMA1_VERIFY_DIR=/tmp/tema1-ui-checks ./build/bin/Debug/GFXFramework
```

The opt-in scene driver exercises input handlers, lane and target legality,
advanced cards, leader selection/use, hidden information, recovery, resizing,
round transitions and complete-match restart. It saves 20+ GPU captures as PPM
images, prints a PASS message and exits. It does not run during ordinary play.

## Submission notes

Select `m1::Tema1` in `src/main.cpp`. Existing CMake source discovery and the scene
include support Tema1 without additional libraries. Keep source and required
resources, exclude executables/build clutter, and keep the coursework archive
below 50 MB. The in-person presentation should explain transforms, pointer
mapping, mesh drawing, legal actions, phase snapshots, cancellation/lifetime,
card ownership and state carried between rounds.
