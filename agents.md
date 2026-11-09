# Chime V2: Agent & Developer Guide

A 2D isometric city builder in **C++17** with **SFML 2.6.1** and **CMake**. The player builds roads and 2×2 zones on a procedurally generated 64×64 isometric map. They manage demand by income level, a daily budget (taxes, upkeep, loans), seasons and citizen happiness.

The whole game compiles as **one translation unit**: `main.cpp` includes headers that hold their own definitions. `src/HeadlessWindow.cpp` is the single extra source file, and only the game target uses it.

---

## Build, Test & Run

```bash
cmake -S . -B build -G "MinGW Makefiles"   # configure
cmake --build build                         # game + tests
.\build\citybuilder_tests.exe               # all 59 tests must pass
.\build\citybuilder.exe                     # play
```

The build copies the SFML DLLs and `assets/` into `build/`.

> [!IMPORTANT]
> **Git Commit & Push Policy**: Do NOT commit or push code unless explicitly commanded by the user. Keep all work local in the working tree.

### Developer launch options (`main.cpp`)

| Option | Purpose |
|---|---|
| `--seed N` | Reproducible world (0 or omitted = random; shown bottom-right) |
| `--size WxH` | Starting window size (fitted to the desktop, minimum 1024×680) |
| `--month N` | Start mid-way through month N (1-12) |
| `--bench N` | Run N s after a 1 s warm-up, print `BENCH avg_ms / fps / worst_ms`, exit. Runs **headless**: an off-screen window that never takes focus (`src/HeadlessWindow.cpp`). `--show` makes it visible; `--headless` works without `--bench` |
| `--zoom Z` | Starting camera zoom (0.4-3.2) |
| `--shot file.png` | Save a screenshot when `--bench` ends |
| `--bench-city` / `--bench-town` | Pre-build a ~650-building grid city / an ~18-building starter town |
| `--fast-forward N` | Simulate N days at start-up, printing money, population and demand every 30 days (balance testing) |
| `--menu N`, `--inspect X Y`, `--view N`, `--budget` | Screenshot helpers: open build category N (1-5), select a tile, set the map view, open the budget |

Typical checks:
- **Speed:** `citybuilder.exe --seed 42 --bench 5 --bench-city`
- **Balance:** `citybuilder.exe --seed 42 --bench-town --fast-forward 360 --bench 0.1`

---

## Controls

| Input | Action |
|---|---|
| **Left sidebar** | Category icons (Inspect, Residential, Commercial, Industry & Farms, Civic & Ports, Roads & Utilities) open a flyout of build cards |
| **Left click + drag** | Build; roads / water / demolish drag along a path or area with a live preview |
| **Right click** | Cancel drag, or return to Inspect |
| `W A S D` / arrows, `Q E` / `+ -` | Pan (`Shift` faster), zoom 0.4×-3.2× |
| Hotkeys | `Esc` Inspect (also closes the budget), `R` `C` `I` `F` Res/Com/Ind/Farm, `1` `2` `3` School/Church/Park, `H` Harbour, `Space` Road, `W` Water (only when no tool is active), `X` Demolish, `O` map view, `B` budget, `P` pause, `[` `]` slower/faster, `N` new city after bankruptcy |
| Top bar | Money chip (click = budget), population + 9 demand bars, happiness, date/season, speed buttons |
| Top right | Map View toggles (Normal / Fertility / Groundwater / Deposits) and the inspect card |

Keyboard and mouse are ignored while the window is unfocused, because `sf::Keyboard` / `sf::Mouse` read global device state.

---

## Isometric Coordinate System & Geometry

### 1. Screen projection
`tileScreenPos(x, y)` (`game/world/WorldData.hpp`) and every entity constructor use:
$$X = 32x \qquad Y = 32y + 16x$$

- $+x$ moves down-right $(+32, +16)$; $(x-1, y+1)$ moves down-left $(-32, +16)$. Both share a full diamond edge.
- $+y$ alone moves **straight down** $(0, +32)$: the two tiles only touch at a diamond tip.

### 2. True 2×2 footprint
Every 2×2 building occupies `FOOTPRINT_2X2 = {(0,0), (1,0), (-1,1), (0,1)}` relative to its root tile:

```
               Top (x, y)
                  /\
       Left      /  \      Right
   (x - 1, y + 1) \  / (x + 1, y)
                  \/
             Bottom (x, y + 1)
```

> [!WARNING]
> **Never use Cartesian `(x+dx, y+dy)` 2×2 offsets.** `(x+1, y+1)` is one column too far right and produces a sheared parallelogram. Use `FOOTPRINT_2X2` / `{{0,0},{1,0},{-1,1},{0,1}}`.

- **Sprites**: 2×2 building sprites are drawn at `(pos.x - 16, pos.y)`, which centres them on the diamond.
- **Road access**: a building is *active* only when a `DirtRoad` lies on one of the **8 edge-sharing border tiles** (`BORDER_2X2`, `hasAdjacentRoad()`). Tip-touching `(i, j±1)` neighbours do not count, and connection does not chain through other zones. Harbours also need `hasAdjacentWater()`. Placement (`canPlaceTile`) and activation (`checkValid`) share this rule.
- **Road autotiling**: `DirtRoad::check()` looks at left/right, `ne` (x+1, y-1) / `sw` (x-1, y+1) and up/down neighbours.

---

## Architecture

```
GameEngine (src/GameEngine.hpp)        window, loop, events (close / resize), assets, --bench
└── Main (game/main.hpp)
    └── Play_Scene (game/scenes/Play_Scene.hpp)
        ├── entities[64][64]            structures only; NULL = natural ground
        │   └── Zone (game/entities/_Zone.hpp)
        │       ├── ResidentalZone / CommericalZone / IndustrialZone   (tier 1-3)
        │       ├── FarmZone, HarbourZone
        │       ├── SchoolZone, ChurchZone, ParkZone                    (civic)
        │       ├── SubZone                                              (filler tiles of a 2x2)
        │       └── Road → DirtRoad
        ├── World (game/world/)
        │   ├── WorldData       world[64][64] terrain / fertility / groundwater / deposits
        │   ├── WorldGen        seeded generator
        │   └── GroundRenderer  natural ground in one vertex-array draw + seasonal look
        ├── Simulation (game/simulation/)
        │   ├── GameClock       calendar, seasons, speed (global gameClock)
        │   ├── EconomySystem   demand, occupancy, taxes, upkeep, loans, debt
        │   └── HappinessSystem service coverage maps + per-home happiness
        └── UI (game/ui/)
            ├── UiKit           palette, cached text / shapes, icons, bars, layout helpers
            └── Toolbar         sidebar + flyout build menu, "Placing" pill, toolCost()
```

### Frame and tick flow
1. **Every frame** (`Play_Scene::update`): input → `renderWorld` → `renderHUD` → `advanceTime(dt)`.
2. **`advanceTime`**: advances `gameClock` by real time × speed (nothing happens while paused or bankrupt).
   - Runs `runSimulationTick()` once per game second, at most 4 catch-up ticks per frame. A tick calls `Zone::simTick()` on every structure, then `HappinessSystem::updateSimulation()`.
   - Runs `runNewDay()` for every day boundary crossed, which calls `EconomySystem::runDay()`.
3. **Map entities have no per-frame logic.** `update()` is not called on them. Road access is re-checked only on change: `checkValid()` runs on placement and from `refreshBuildingsAround()`. Road autotiling likewise only runs from `refreshRoadsAround()`.

### Core components
- **GameEngine**: creates the window, sized to fit the desktop (or the headless window). Forwards `sf::Event::Resized` (clamped to 1024×680) to `Scene::onResize()`. Loads textures, including `ui_<name>` icons. Implements `--bench` / `--shot`.
- **Play_Scene**: input, placement (`canPlaceTile` / `placeTile`), rendering passes, HUD (top bar, Map View, inspect card, toast, budget, game-over), `startNewCity(seed)`.
- **Zone**: the virtual interface for buildings:
  - `checkValid()`: road access.
  - `simTick()`: slow per-second logic.
  - `sector()` / `wealth()`: the (sector, wealth) bucket; wealth = tier − 1.
  - `dailyTax()` = `population × INCOME_PER_CAPITA[s][w] × taxRates[s][w] / 100 × incomeMultiplier()`.
  - `dailyUpkeep()`.
  - Farms override `incomeMultiplier()` (soil × season). Harbours are C$$ with 30 port jobs.
- **EconomySystem** (`Economy.hpp`, constants in `EconomyData.hpp`): `runDay()` = scan → `sampleServices()` → `computeDemand()` → `updateOccupancy()` (includes `changeTier()`) → scan → `settle()`.
  - **Demand:** 9 values from −100 to 100. Residents follow open job capacity, shops follow customers vs filled jobs, industry follows workers vs filled jobs plus harbour exports. Adjusted for tax vs 7% neutral, city happiness, education (R$$ / R$$$ / I$$$) and pollution near homes.
  - **Upgrades** need a full building, happiness, and demand for the next income level.
  - **Bankruptcy:** 180 consecutive days below $0. Loans: $10k at 10% over 360 days, max 3.
- **HappinessSystem**: coverage maps (education R5, church R4, park R4, pollution R3, commerce R3) and per-home happiness. Weights: jobs 0.25, education 0.20, health 0.20, safety 0.15, tax 0.20. It does **not** change tiers; the economy does.
- **GroundRenderer**: builds a texture atlas at runtime and one vertex array for every ground tile. It is rebuilt only after `markDirty()` or a `SeasonLook` change. Draws a snow layer, and forest / rock sprites in the depth-sorted pass.
- **Toolbar**: the build menu. `toolCost()` is the single price table and `toolThumbnailKey()` maps each tool to its card / ghost sprite.
- **UiKit**: `roundedRect`, `panel`, `text` / `textMid` / `textRight`, `fitText`, `icon`, `thumbnail`, `bar`, `formatMoney`. Text and shape geometry are cached by content (`cachedText`, shape map), which keeps the HUD cheap.

### Conventions
- **Natural ground is data, not entities.** Use `tileKind(x, y)`: it returns the structure's name, or `Grass` / `Sand` / `Water` / `Forest` / `Rock` / `ForestPath` for an empty slot. After changing `world[][].terrain`, call `ground.markDirty()`.
- **Adaptive layout.** Lay out HUD elements with `uikit::SW()` / `uikit::SH()` (the live `screenW` / `screenH`), never `WINDOW_W` / `WINDOW_H`, which are only the preferred start size. Map mouse positions with `uikit::hudFromPixel()`. The HUD view is 1:1 with the window, and the world view is window size × zoom, so nothing stretches.
- **Determinism.** World generation uses its own `std::mt19937`. Economy "random" upgrades use a hash of position and day, never `std::rand`, so tests stay stable.
- **Tests construct `Play_Scene` without `init()`.** Keep logic reachable without a window or textures, and reset globals in `clearTestGrid()` (world, clock, tax rates).

---

## Global State (`src/_global.hpp` and module globals)

| Global | Type | Purpose |
|---|---|---|
| `_window`, `view` | `sf::RenderWindow`, `sf::View` | Window and world camera |
| `screenW`, `screenH` | `int` | Live window size (HUD layout) |
| `WINDOW_W`, `WINDOW_H`, `preferredW/H` | `int` | Preferred start size (`--size`) |
| `entities[64][64]` | `Entity*` | Structures; `NULL` = natural ground |
| `world[64][64]` | `TileData` | Terrain, fertility, groundwater, deposit (`WorldData.hpp`) |
| `worldSeed`, `worldSpawnX/Y` | | Current seed and the guaranteed buildable start area |
| `money` | `int` | Treasury (starts at `STARTING_MONEY` = $20,000; may go negative) |
| `taxRates[3][3]` | `int` | Tax % per (sector, wealth), 0-20, default 9 (`EconomyData.hpp`) |
| `townPopulation[3]`, `townCapacity[3]`, `townDemand[3]` | `int` | Per-sector totals refreshed by the economy scan (`townDemand` = average of the 3 income levels) |
| `townStats[9]` | `int` | Happiness system averages (taxes, population, education, safety, health, recreation, …, jobs) |
| `gameClock`, `yr` / `mon` / `day` | `GameClock`, `int` | Calendar; the ints mirror the clock |
| `tileX`, `tileY` | `int` | Hovered / inspected tile |
| `benchSeconds`, `benchCity`, `benchTown`, `fastForwardDays`, `headless`, … | | Launch-option flags |

---

## World Generation (`game/world/`)

`startNewCity(seed)` calls `generateWorld(seed)`, which is pure data and writes only `world[][]`, then `materializeWorld()`, which clears `entities[][]`.

**Pipeline:** height fBm with coastal falloff → ocean (lowest ~18-26%) → inland lakes → rivers into water → sand beaches → rocky hills (top ~8% of land) → forests (wettest ~22% of grassland) → forest paths → spawn area (8×8 grass, water within 8 tiles) → fertility → groundwater → deposits.

**Placement rules:**
- Forest and rock block building until bulldozed ($15 / $60), which turns them into grass.
- Sand and forest paths are buildable.
- Demolishing a structure empties its slot, so the original terrain shows again.

**Sprites:**
- `tools/gen_terrain_sprites.py`: sand, path, forest (summer / autumn / winter) and rock.
- `tools/gen_ui_icons.py`: the 16×16 HUD icons.

---

## Tests (`tests/test_main.cpp`, 59)

A small custom framework (`tests/test_framework.hpp`). Areas covered:

- **Geometry & input**: projection, diamond picking, drag paths, visible-tile range vs brute force, HUD adapting to window size.
- **Build menu**: layout, flyout click flow, tool names, `toolCost()` consistency.
- **Placement & roads**: road / water / forest / rock blocking, edge-sharing road access, on-change re-checks, autotiling, demolition restoring terrain.
- **Buildings**: capacity accounting, tiers and variants, civic radii, farms (fertility, seasons), harbour water rules.
- **World**: determinism, terrain mix, forest paths, spawn guarantee, data invariants, ground renderer quads and seasonal look.
- **Time**: calendar and seasons, speed / pause, time flow through the scene.
- **Economy**: tax from occupants per bucket, fractional carry, demand drivers (jobs, customers, tax, education, pollution, happiness), occupancy, upgrades needing demand, upkeep report, loans, bankruptcy and restart, budget sliders and buttons.
- **Happiness**: coverage maps and scoring.

---

## Directory Structure

```
cpp-2d-citybuilder/
├── CMakeLists.txt              # Game + test targets, asset / DLL copy
├── main.cpp                    # Entry point, launch options
├── agents.md                   # This guide
├── README.md                   # Player / repository overview
├── docs/
│   ├── THEOTOWN_SIMULATION_GUIDE.md   # Gameplay & simulation reference
│   └── images/                 # README screenshots
├── assets/
│   ├── Hack.ttf
│   └── sprites/                # Buildings, terrain, roads; ui/ = HUD icons
├── tools/                      # gen_terrain_sprites.py, gen_ui_icons.py, older sprite tools
├── tests/                      # test_framework.hpp, test_main.cpp
├── src/                        # _global.hpp, GameEngine.hpp, Scene.hpp, Entity.hpp,
│                               # AssetManager.hpp, HeadlessWindow.cpp
└── game/
    ├── main.hpp
    ├── scenes/                 # Play_Scene.hpp (Menu_Scene stub)
    ├── entities/               # _Zone, R/C/I, Farm, Harbour, School, Church, Park, SubZone, Player, roads/
    ├── world/                  # WorldData, WorldGen, GroundRenderer
    ├── simulation/             # Economy, EconomyData, GameClock, HappinessSystem
    └── ui/                     # UiKit, Toolbar, Window (legacy)
```
