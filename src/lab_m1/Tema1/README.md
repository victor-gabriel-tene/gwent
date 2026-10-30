# Tema1 — GWENT: The Card Table

A C++11/OpenGL implementation of all **basic** requirements from
[EGC Tema 1, 2026](https://ocw.cs.pub.ro/courses/egc/teme/2026/01).
The procedural walnut/brass tabletop is inspired by the Witcher 3 GWENT UI;
the rules follow the homework's simplified HP-based game.

## Build and run

From the repository root, using the framework's existing dependencies. Release is
recommended for normal play and is built separately from the existing Debug tree:

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j 6
./build/release/bin/Release/GFXFramework
```

The existing Debug build remains available with `cmake -S . -B build`,
`cmake --build build -j 6`, and `./build/bin/Debug/GFXFramework`.
Run with the generated `assets` link beside the executable intact.
No additional libraries or downloaded artwork are required.

## Controls

| Control | Action |
| --- | --- |
| Hold left mouse on a hand card | Pick up and drag |
| Release inside your lower battlefield half | Play one card |
| Release elsewhere | Animate back to the original hand slot; no turn spent |
| Backspace while dragging | Cancel the drag |
| Hover over a visible card | Show its description |
| H, or the Rules button | Open/close the rules overlay; pause turn progression |
| Click the rules overlay | Close it |
| R, or Play Again after a result | Begin a fresh game |
| Escape | Exit (framework control) |

## Cards and match rules

Both sides start at 30/30 HP, with no shield and ten randomly dealt cards.
There are no ordinary deck draws after the opening deal.

Each player's shuffled 30-card deck contains:

| Type | Count / starting V | Effect each activation |
| --- | --- | --- |
| Damage, red triangle | 12: four each at V 1, 2, 3 | Deal V damage |
| Heal, green diamond | 6: three each at V 1, 2 | Restore V HP, capped at 30 |
| Shield, blue circle | 6: three each at V 1, 2 | Add V shield |
| Spy, black paired diamonds | 2 at V 2 | Steal one random opponent hand card |
| Frost, white snowflake | 2 at V 2 | Reduce every opponent board card's V by 1 |
| Clear Sky, yellow sun | 2 at V 1 | Remove all active Frost on both sides |

The three temporary types are Spy, Frost and Clear Sky. The homework permits
three of the five listed types for basic credit. War Horn and Scorch are deferred.
Square and diamond symbol frames distinguish permanent and temporary categories.
Current V is displayed both numerically and through geometric units along the
card's left side. HP and shield also have individual geometric unit indicators.

The player places one card; the enemy randomly chooses and places one face down,
then flips it face up. **All surviving board cards** resolve every turn, including
those played earlier:

**Spy → Clear → Frost → Shield → Damage → Heal.**

This is the homework's prescribed ordering with the unimplemented Scorch and
War Horn phases skipped. Every temporary card loses one V after its activation;
any board card at V ≤ 0 is discarded. Temporary V controls lifetime, not effect
magnitude: a Spy steals one card and a Frost applies one reduction per activation.

### Defaults for unspecified interactions

- Same-type effects use a phase-start snapshot and resolve symmetrically.
- Multiple Spies choose distinct targets from hands after both placements.
  Transfers cannot select a card that another Spy has just transferred that phase.
- A temporary activation without a target still consumes one V.
- Clear removes both players' Frost cards, including freshly played cards.
  It does not restore V already lost to earlier Frost activations.
- Frost affects every opposing board type. All Frost sources alive at its phase
  start activate, including ones destroyed by opposing Frost in that phase.
- Cards destroyed in an earlier phase cannot activate later in that turn.
- Shield accumulates without a gameplay cap, persists between turns, and remains
  after the Shield card that created it is destroyed. It absorbs damage first.
- Both sides' damage is simultaneous. Check lethal before Heal; no resurrection.
- One empty-handed side automatically passes; surviving cards still activate.
  Spy can refill an empty hand. Both-empty exhaustion is checked after resolution.
- Zero HP loses. Simultaneous lethal is a draw. If both hands are empty, compare
  HP only; equal HP means a draw. Shield is not a tiebreaker.

## Implementation map

- `GameRules.h`: graphics-independent state, deck generation, legal placement,
  effect phases, outcomes and ownership invariant.
- `Tema1.h` / `Tema1.cpp`: turn controller, event presentation, reusable geometric
  shapes, logical layout, dragging and animations.
- `BatchRenderer.h` / `.cpp`: one painter-ordered draw for procedural geometry
  and atlas text, streamed rotating buffers, cached text layout and retained frames.
- `tests/rules_tests.cpp`: standalone deterministic and randomized rule checks.
- `tests/SceneChecks.inl`: opt-in application-level interaction/rendering checks.
- `tests/PerformanceChecks.h`: opt-in wall-clock frame/render benchmark.

All positions use a 1600×900 logical canvas, with uniform scaling and letterboxing.
Pointer conversion queries actual GLFW window/framebuffer sizes, including Retina
scaling. Geometry and text use the same viewport. Hands retain order in one centered
row; board groups may use a second row. Every layout change interpolates from the
current position. The enemy reveal follows a vertical-axis angle from 0 to π,
using its orthographic projected width and swapping faces at π/2.

### Rendering performance

All geometry and text is submitted in **one ordered draw call per frame**. Glyphs
share a single immutable font atlas. Changed frames use rotating buffers and a
replacement buffer store; there is no per-character `glBufferSubData` overwrite.
Board decoration, card faces, status/deck panels and text layouts are cached.
Stationary frames reuse the uploaded GPU geometry without rebuilding or uploading
it. There are no per-shape uniform queries or frequent GL error queries in the
rendering path. Animation uses elapsed wall time rather than a 100 ms timestep cap.

The renderer is local to Tema1; the framework and laboratory text/mesh code are
unchanged. The viewport retains the same uniform logical scaling illustrated by
lab 3, including Retina-aware inverse pointer conversion.

Measured on the development M1 Mac at a 2560×1440 framebuffer:

| Configuration / workload | Rendered FPS | Mean rendering CPU time |
| --- | ---: | ---: |
| Original Debug renderer | 5.49 | 180.08 ms |
| Batched Debug, stationary table | 88.70 | 0.065 ms |
| Batched Debug, forced rebuild/upload every frame | 120.07 | 1.750 ms |
| Batched Release, stationary table | 101.72 | 0.066 ms |

These are short wall-clock measurements, not a fixed FPS guarantee; presentation,
display synchronization and system scheduling affect frame intervals. The forced
rebuild test checks that performance does not depend only on the idle-frame cache.

No source changes are required outside Tema1 except selecting `m1::Tema1` in
`src/main.cpp`. The existing CMake source glob and scene include already support it.
The user's other laboratory files remain untouched.

## Verification

Standalone tests need only a C++11 compiler. On macOS with Clang:

```sh
clang++ -std=c++11 -Wall -Wextra -pedantic -fsanitize=address,undefined \
  -DTEMA1_RULE_TESTS src/lab_m1/Tema1/tests/rules_tests.cpp \
  -o /tmp/tema1-rules-tests
/tmp/tema1-rules-tests
```

Checks cover recurrence, shield/heal boundaries, temporary decay, Spy transfers,
weather timing, destruction, simultaneous lethal, exhaustion, restart and 1,000
complete seeded matches. Ownership/value invariants are checked after every phase.

For application-level checks, first build the game and create an output directory:

```sh
mkdir -p /tmp/tema1-ui-checks
TEMA1_VERIFY_DIR=/tmp/tema1-ui-checks ./build/bin/Debug/GFXFramework
```

This opt-in driver exercises the scene's actual mouse handlers, valid/invalid drops,
enemy response and effect progression, help, window resizing, complete-match flow,
and restart. It saves seven GPU frame captures as PPM images, prints a PASS message,
and exits. It does not run during ordinary play.

To measure an idle, fully dealt game (one second of warmup, then six seconds):

```sh
TEMA1_BENCHMARK_FILE=/tmp/tema1-performance.json ./build/bin/Debug/GFXFramework
```

The JSON reports rendered FPS, mean/p95 frame intervals, render CPU time and
framebuffer dimensions. To stress rebuilding and uploading every frame, including
when the table is stationary:

```sh
TEMA1_BENCHMARK_REBUILD=1 TEMA1_BENCHMARK_FILE=/tmp/tema1-rebuild.json \
  ./build/bin/Debug/GFXFramework
```

Use the same window size, configuration and power settings for comparisons; run
one game instance at a time. Frame intervals include presentation/vsync, while
render CPU time measures building/submitting the frame. GPU work is asynchronous,
so CPU submission time is not a standalone measure of GPU frame time.

## Basic rubric coverage

| Area | Points | Coverage |
| --- | ---: | --- |
| Board | 20 | Two halves, both hands, colored card faces, type/category symbols, side V units, both deck stacks |
| Drag and drop | 35 | Rectangle selection/release checks, grab offset, cursor following, topmost drag, valid placement, invalid animated return |
| Drawing | 15 | Both opening hands animate from their respective decks |
| Organization | 20 | Both hands/board groups center and translate; stable hand ordering, crowded layouts |
| Permanent types | 15 | Damage, Heal, Shield |
| Temporary types | 15 | Spy, Frost, Clear |
| Effect resolution | 15 | Recurrence, specified relative order, decay and destruction |
| Enemy | 5 | Random hand choice, placement, rotating reveal before effects |
| Life UI | 5 | One rectangle per HP and separate shield units |
| Flow | 5 | Turns, forced empty-hand pass, outcomes, restart |
| **Total** | **150** | All basic requirements |

Advanced lanes, additional cards, leaders and best-of-three match rounds are deferred.

## Submission notes

Include source and required resources, with no executables/build clutter; keep the
archive below 50 MB. The coursework also requires an in-person presentation. Be
prepared to explain the transformations, pointer mapping, phase snapshots, temporary
decay, card ownership and animation/controller separation.
