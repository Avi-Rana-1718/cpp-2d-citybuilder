# Chime V2 — City Builder Agent & Developer Guide

A 2D isometric city-builder written in **C++17** with **SFML 2.6.1** and **CMake**.
The player places zones and roads on an isometric tile grid, manages population growth across Residential, Commercial, and Industrial sectors, and balances municipal treasury with periodic tax revenue.

---

## Build, Test & Run

```bash
# Configure with CMake (MinGW Makefiles on Windows)
cmake -S . -B build -G "MinGW Makefiles"

# Build all targets (executable and tests)
cmake --build build

# Run unit and integration tests (all 52 tests must pass)
ctest --test-dir build --output-on-failure
# Or directly run the test binary:
.\build\citybuilder_tests.exe

# Run game
.\build\citybuilder.exe
```

The binary `citybuilder.exe`, test runner `citybuilder_tests.exe`, required SFML DLLs, and `assets/` directory are output to `build/` automatically via post-build commands.

> [!IMPORTANT]
> **Git Commit & Push Policy**: Do NOT commit or push code unless explicitly commanded by the user. Keep all work local in the working tree.

---

## Controls & Interaction

| Input | Action |
|---|---|
| **TheoTown Bottom Toolbar** | Click any category button (`INSPECT`, `ZONING`, `CIVICS`, `INFRASTRUCTURE`) to arm tools |
| **Mouse Hover** | Highlight and select tile under cursor (isometric diamond collision detection) |
| **Left Click + Drag** | **Drag-to-Multibuild** — drag an isometric diamond area or road path with live ghost preview, builds on release |
| **Right Click** | Cancel active drag or return to `INSPECT` mode |
| **`W / A / S / D`** or **Arrow Keys** | Pan camera smoothly across the world (`Shift` for fast pan) |
| **`Q / E`** or **`+ / -`** | Smooth camera zoom in (`Q` / `+`) and zoom out (`E` / `-`) between $0.4\times$ and $3.2\times$ |
| **Hotkeys** | `Esc` (Inspect), `R` (Residential), `C` (Commercial), `I` (Industrial), `F` (Farms), `1` (School), `2` (Church), `3` (Park), `Space` (Dirt Road), `H` (Harbour), `W` (Water), `X` (Demolish), `O` (cycle map view: Normal / Fertility / Groundwater / Deposits), `P` (pause / resume), `[` / `]` (slower / faster) |
| **Speed buttons** | Top bar `|| 1x 2x 4x` next to the date |
| **Map View panel** | Top-left HUD buttons switch the map view to show soil fertility, groundwater or coal/iron/stone deposits |
| **`--seed N`** | Launch argument: generate a reproducible world (`citybuilder.exe --seed 42`); omitted = random seed shown in the top bar |
| **`--month N`** | Start mid-way through month N (1-12), e.g. `--month 1` for winter, `--month 10` for autumn |
| **`--bench N`** | Launch argument: run N seconds after a 1 s warm-up, print `BENCH avg_ms / fps / worst_ms`, then exit. Combine with `--zoom Z` (0.4-3.2), `--shot file.png` (save a screenshot at the end) and `--bench-city` (fill the map with a ~650-building grid city first). Benchmarks run **headless**: an off-screen window that never takes focus or shows on the taskbar (`src/HeadlessWindow.cpp`); pass `--show` to watch, or `--headless` without `--bench` |
| **Focus** | Keyboard and mouse are ignored while the game window is not focused (`sf::Keyboard` / `sf::Mouse` read global device state) |

---

## Isometric Coordinate System & Geometry

### 1. Screen Projection Formula
In `tileScreenPos()` (`game/world/WorldData.hpp`) and every entity constructor, screen coordinates $(X, Y)$ are computed as:
$$\text{pos.x} = 32 \cdot x$$
$$\text{pos.y} = 32 \cdot y + 16 \cdot x$$

- **Axis 1 (East / Down-Right)**: $+x \longrightarrow (\Delta X, \Delta Y) = (+32, +16)$ (2:1 slope).
- **Axis 2 (South-West / Down-Left)**: $-x, +y \longrightarrow (\Delta X, \Delta Y) = (-32, +16)$ (flush edge-sharing).
- **Stepping $+y$ alone**: Moves **straight down vertically** by 32 pixels $(\Delta X = 0, \Delta Y = 32)$, touching $(x, y)$ only at its bottom tip.

### 2. True 2×2 Symmetrical Isometric Diamond Footprint
Any 2×2 building complex (Residential, Commercial, Industrial, Farm, Civics, Harbour) **MUST** strictly occupy the 4 tiles forming a symmetrical 2:1 isometric rhombus ($128 \times 64$ pixels):
- **Top / Root Tile**: `(x, y)`
- **Right / East Tile**: `(x + 1, y)`
- **Left / West Tile**: `(x - 1, y + 1)`
- **Bottom / South Tile**: `(x, y + 1)`

```
               Top (x, y)
                  /\
       Left      /  \      Right
   (x - 1, y + 1) \  / (x + 1, y)
                  \/
             Bottom (x, y + 1)
```

> [!WARNING]
> **Never use Cartesian `(x + dx, y + dy)` for 2×2 footprints!**
> Cartesian offsets allocate `(x, y)`, `(x+1, y)`, `(x, y+1)`, and `(x+1, y+1)`. Because $+y$ steps vertically down, `(x+1, y+1)` is shifted one column too far to the right, causing a sheared parallelogram with vertical sides (`0110` on row 1, `0011` on row 2). Always use `const sf::Vector2i dOffsets[4] = {{0, 0}, {1, 0}, {-1, 1}, {0, 1}};`.

### 3. Sprite Centering Rule
- 2×2 building sprites (unscaled $48\text{px}$, scaled $2\times$ to $96\text{px}$) are centered over the $128\text{px}$ diamond by setting:
  $$\text{sprite.setPosition}(\text{pos.x} - 16.0\text{f}, \text{pos.y})$$
- This aligns the building's vertical axis of symmetry with the true diamond center $(32 \cdot x + 32)$ with equal $16\text{px}$ margins on left and right.

### 4. Road Connectivity Rules
- **Horizontal Avenues (Axis 1)**: Run along constant $y$ (e.g. `setRoad(x, 4)`). Every tile $(x+1, y)$ shares a $32 \times 16$ edge with $(x, y)$.
- **Diagonal Cross-Boulevards (Axis 2)**: Run along constant sum $x + y = S$ (e.g. `setRoad(S - y, y)`). Consecutive road tiles $(x - 1, y + 1)$ share a complete $32 \times 16$ edge with $(x, y)$ with **zero diamond-tip seams**.
- **Autotiling**: Roads check neighbors along both Axis 1 (`left`, `right`) and Axis 2 (`ne`: $x+1, y-1$, `sw`: $x-1, y+1$) to render straight roads, 180° rotations, and 4-way intersections (`road.png`, `road-mid.png`).

---

## Architecture & Class Hierarchy

```
GameEngine
└── Main (game/main.hpp)
    └── Scene (src/Scene.hpp)
        └── Play_Scene (game/scenes/Play_Scene.hpp)
            ├── Entity Grid (entities[64][64]: structures only, NULL = natural ground)
            │   └── Zone
            │       ├── ResidentalZone (T1-T3)
            │       ├── CommericalZone (T1-T3)
            │       ├── IndustrialZone (T1-T3)
            │       ├── FarmZone
            │       ├── HarbourZone
            │       ├── SchoolZone (Civic)
            │       ├── ChurchZone (Civic)
            │       ├── ParkZone (Civic)
            │       └── Road
            │           └── DirtRoad
            ├── World Layer (game/world/)
            │   ├── WorldData (world[64][64]: terrain, fertility, groundwater, deposits)
            │   ├── WorldGen (seeded procedural generator)
            │   └── GroundRenderer (all natural ground in one vertex-array draw)
            ├── World Cursor (Player.hpp)
            ├── Simulation Layer
            │   └── HappinessSystem (game/simulation/HappinessSystem.hpp)
            └── UI Layer
                ├── Toolbar Dock (game/ui/Toolbar.hpp)
                └── Window (game/ui/Window.hpp - HUD panels: Stats, Treasury, Date, Debug)
```

### Core Components
- **GameEngine (`src/GameEngine.hpp`)** — Owns the main window (`_window`), camera (`view`), asset manager, delta-time clock, frame counter, and the active `Scene`. Drives the primary game loop: polls SFML window close events, computes real-time FPS, restarts clocks, and delegates per-frame updates to `currentScene->update(dt)`.
- **Scene (`src/Scene.hpp`)** — Abstract scene base class. Defines `init()` and `update(deltaTime)`. Clears UI and background entity lists on initialization.
- **Play_Scene (`game/scenes/Play_Scene.hpp`)** — Simulation coordinator scene. Delegates to `handleCameraMovement()`, `handleToolHotkeys()`, `handleTileHover()`, `handleGestures()`, `renderWorld()`, and `renderHUD()`. Houses periodic throttled simulation accumulator for `HappinessSystem`.
- **Toolbar (`game/ui/Toolbar.hpp`)** — Self-contained TheoTown dock UI component. Organizes 4 categories (`INSPECT`, `ZONING`, `CIVICS`, `INFRASTRUCTURE`) and 11 distinct tools with active state detection and shortcut indicators.
- **HappinessSystem (`game/simulation/HappinessSystem.hpp`)** — High-performance 9-domain satisfaction engine. Computes spatial service coverage (Education, Safety, Health, Recreation, Pollution), weights economic satisfaction (taxes, jobs), and drives autonomous tier upgrades (T1 $\to$ T2 $\to$ T3).
- **Entity (`src/Entity.hpp`)** — Base game object. Owns transform `position`, ID, naming, `isActive` status, and optional rendering primitives (`sprite`, `shape`, `text`). Safely handles rendering dispatch in `draw()` with null guards, and deallocates heap-allocated SFML objects in `~Entity()`.
- **Zone (`game/entities/_Zone.hpp`)** — Extends `Entity`. Base class for developable and natural ground tiles. Tracks `capacity`, `population`, `zoneIncome`, `buildCost`, and tax payment state.
- **SubZone (`game/entities/SubZone.hpp`)** — Child tile entity for multi-tile structures. Shares ground base texture (`zone_residental`, `zone_commercial`, `zone_industrial`, `zone_farm`, or `grass`) with master zone. Does not draw duplicate building sprites.
- **ResidentalZone, CommericalZone, IndustrialZone** — 2×2 complexes with 3 happiness-dependent tiers and variants. Auto-upgrade and downgrade based on citizen satisfaction.
- **Civics (`SchoolZone`, `ChurchZone`, `ParkZone`)** — 2×2 civic services with radial coverage: Elementary School ($R=5$), Gothic Cathedral ($R=4$), and Nature Park ($R=4$).
- **HarbourZone (`game/entities/HarbourZone.hpp`)** — 2×2 maritime trade port placed on shorelines adjacent to water.
- **Road access (`checkValid`)** — Every 2×2 building is active only when a `DirtRoad` sits on one of the 8 tiles sharing a full edge with its footprint (`BORDER_2X2`, `hasAdjacentRoad()` in `game/world/WorldData.hpp`; harbours also need `hasAdjacentWater()`). `checkValid()` is a virtual on `Zone`, called when the building is placed and from `Play_Scene::refreshBuildingsAround()` after a road, water or bulldoze change — never per frame.
- **HUD (`game/ui/`)** — `UiKit.hpp` holds the palette and drawing helpers (rounded panels, cached text, icons, bars, thumbnails, `fitText`). `Toolbar.hpp` is the build menu: a left sidebar of category icons, a flyout of build cards (sprite thumbnail, price from `toolCost()`, hotkey), and a bottom-left "Placing" pill. `Play_Scene` draws the top status bar (money/day, population + R/C/I demand, happiness, date/season, speed), the Map View icon toggles, the inspect card and the error toast. Icons are 16×16 PNGs in `assets/sprites/ui/` from `tools/gen_ui_icons.py`, registered as `ui_<name>`. Screenshot flags: `--menu N`, `--inspect X Y`, `--view N`.
- **Game clock (`game/simulation/GameClock.hpp`)** — Global `gameClock`. 1 day = 4 s at 1x; speeds Pause / 1x / 2x / 4x; 30-day months, 12 months, start 1 Mar Year 1. Seasons: Spring Mar-May, Summer Jun-Aug, Autumn Sep-Nov, Winter Dec-Feb. `Play_Scene::advanceTime()` advances it each frame; every new day runs `runNewDay()` (daily taxes via `Zone::dailyTax()` with fractional carry, population growth). Globals `yr`/`mon`/`day` mirror the clock.
- **Seasons** — `GroundRenderer` applies a `SeasonLook` (grass/water tint, autumn/snowy tree sprites, winter snow layer over land, lots and roads), blending into the next season over its last 20%. Farms earn ×1.0 spring, ×1.3 summer, ×1.5 autumn, ×0.3 winter.
- **Simulation tick** — Map entities have no per-frame logic and `update()` is not called on them. Once per game second (so 4×/s at 4x, never while paused) `Play_Scene::runSimulationTick()` calls `Zone::simTick()` on every structure (tier changes, `SubZone` active-state sync) and then `HappinessSystem::updateSimulation()`.
- **DirtRoad (`game/entities/roads/DirtRoad.hpp`)** — 1×1 autotiling road network across 4-way and diagonal intersections.

---

## Global Simulation State (`src/_global.hpp`)

| Global | Type | Purpose |
|---|---|---|
| `_window` | `sf::RenderWindow` | SFML window instance (1500 × 900) |
| `view` | `sf::View` | Primary 2D world camera viewport |
| `entities[64][64]` | `Entity*[][]` | 64×64 isometric tile grid (4,096 tiles, 10x size) |
| `ui` | `std::vector<Entity*>` | UI entities rendered in fixed screen space |
| `bg` | `std::vector<Entity*>` | Background layer entities |
| `money` | `int` | Player municipal funds (starts at $50,000 in Play_Scene) |
| `townPopulation[3]` | `int[3]` | Population by zone (0: Residential, 1: Commercial, 2: Industrial) |
| `townCapacity[3]` | `int[3]` | Total available housing/job capacity |
| `townDemand[3]` | `int[3]` | Current zone demand |
| `townTaxes[3]` | `int[3]` | Tax rate percentage per sector (default: 5%) |
| `townStats[9]` | `int[9]` | Aggregated city statistics (Happiness, Education, Health, etc.) |
| `world[64][64]` | `TileData[][]` | Natural terrain + soil fertility + hidden groundwater/deposits per tile (`game/world/WorldData.hpp`) |
| `worldSeed` | `uint32_t` | Seed of the current world (0 = random at init) |
| `worldSpawnX`, `worldSpawnY` | `int` | Centre of the guaranteed buildable start area; camera starts here |
| `tileX`, `tileY` | `int` | Grid coordinates of the selected tile |
| `gameClock` | `GameClock` | Calendar, seasons and speed (`game/simulation/GameClock.hpp`) |
| `yr`, `mon`, `day` | `int` | Current date, mirrored from `gameClock` each frame |
| `calculatedFPS` | `int` | Measured frames per second |

---

## Automated Test Suite (30 Tests)

All 30 automated tests in `tests/test_main.cpp` must remain green before and after any changes:

```
[ RUN      ] Test_IsometricProjection_Formula ................... [ OK ]
[ RUN      ] Test_IsometricRaycasting_DiamondCenter ............. [ OK ]
[ RUN      ] Test_IsometricRaycasting_DiamondTipsAndExclusion .... [ OK ]
[ RUN      ] Test_Toolbar_LayoutAndDefaults ..................... [ OK ]
[ RUN      ] Test_Toolbar_ToolSelectionAndToggle ................ [ OK ]
[ RUN      ] Test_DragComputation_RectangularBox ................ [ OK ]
[ RUN      ] Test_DragComputation_RoadLPath ...................... [ OK ]
[ RUN      ] Test_Zone_CapacityAccounting ....................... [ OK ]
[ RUN      ] Test_Zone_AdjacencyActivationRules ................. [ OK ]
[ RUN      ] Test_Zone_BoundaryEdgeSafety ....................... [ OK ]
[ RUN      ] Test_Economy_TaxationCycle ......................... [ OK ]
[ RUN      ] Test_Economy_PopulationGrowthAndDemand ............. [ OK ]
[ RUN      ] Test_Roads_AutotilingLogic ......................... [ OK ]
[ RUN      ] Test_IndustrialZone_LifecycleAndTiers .............. [ OK ]
[ RUN      ] Test_FarmZone_RoadActivationAndTaxes ............... [ OK ]
[ RUN      ] Test_HarbourZone_WaterAdjacency .................... [ OK ]
[ RUN      ] Test_CivicServices_RadiusesAndTypes ................ [ OK ]
[ RUN      ] Test_HappinessSystem_CoverageMaps .................. [ OK ]
[ RUN      ] Test_HappinessSystem_ScoringAndTierProgression ...... [ OK ]
[ RUN      ] Test_Toolbar_NewCategoriesAndToolNames ............. [ OK ]
[ RUN      ] Test_CivicServices_SchoolAndChurchProperties ....... [ OK ]
[ RUN      ] Test_Zone_TiersVariantsAndHappinessProgression ..... [ OK ]
[ RUN      ] Test_Zone_SameBaseTextureAcrossSubZones ............ [ OK ]
[ RUN      ] Test_Demolition_RestoresGrassTile .................. [ OK ]
[ RUN      ] Test_World_FullBuildingCoverage .................... [ OK ]
[ RUN      ] Test_TheoTown_CannotBuildOnRoad .................... [ OK ]
[ RUN      ] Test_TheoTown_CannotBuildOnWater .................. [ OK ]
[ RUN      ] Test_TheoTown_RequiresRoadConnection ............... [ OK ]
[ RUN      ] Test_TheoTown_HarbourShorelineRules ................ [ OK ]
[ RUN      ] Test_TheoTown_InsufficientFundsAndBulldoze ......... [ OK ]
```

---

## World Generation (`game/world/`)

Every game starts on a procedurally generated map. `Play_Scene::init()` calls `generateWorld(worldSeed)` (pure data, writes only `world[][]`) and then `materializeWorld()` (clears `entities[][]`).

- **Natural ground is data, not entities**: `entities[x][y]` holds only structures (buildings, roads). A `NULL` slot is natural ground described by `world[x][y].terrain`. `GroundRenderer` draws every ground tile from `world[][]` as one `sf::VertexArray` over a runtime texture atlas (rebuilt only after `markDirty()`), and draws forest trees / rock boulders in the depth-sorted building pass. Use `tileKind(x, y)` for placement logic: it returns the structure name, or `Grass` / `Sand` / `Water` / `Forest` / `Rock` / `ForestPath` for empty slots.

- **Pipeline**: height fBm with coastal falloff on 1-2 map edges → ocean (lowest ~18-26%) → inland lakes → rivers (steepest descent into water) → sand beaches on the ocean shore → rocky hills (top ~8% of land) → forests (wettest ~22% of grassland) → forest paths → spawn area → fertility → groundwater → coal/iron/stone deposits.
- **Determinism**: uses its own `std::mt19937`, never `std::rand`. Same seed means same world.
- **Spawn guarantee**: an 8×8 all-grass area near the centre with water within 8 tiles.
- **Placement**: forest and rock block building until bulldozed ($15 / $60), which turns them into grass. Sand and forest paths are buildable. Demolishing a structure empties its slot, so the original terrain shows again. Changing terrain must call `ground.markDirty()`.
- **Map View**: all resources are always known. The top-left Map View panel (or `O`) tints the map by fertility, groundwater, or deposits (coal black, iron orange, stone white).
- **Farms**: income × `fertilityMultiplier` (0.4× barren → 1.0× at 50 → 1.6× rich soil).
- **Sprites**: `tools/gen_terrain_sprites.py` generates `sand`, `forest_path`, `forest`, `rock` without touching other sprites.

---

## Directory Structure

```
cpp-2d-citybuilder/
├── CMakeLists.txt              # CMake build configuration with test target and DLL copy
├── main.cpp                    # Application entry point
├── AGENTS.md                   # Canonical agent guide and architecture reference
├── README.md                   # Repository overview and quick start
├── docs/                       # In-depth system documentation
│   └── THEOTOWN_SIMULATION_GUIDE.md # Comprehensive guide for gameplay, simulation, & civics
├── tests/                      # Automated test suite (52 tests)
│   ├── test_framework.hpp      # Lightweight assertion and runner framework
│   └── test_main.cpp           # Unit & integration test cases
├── assets/                     # Game assets (sprites and fonts)
│   ├── Hack.ttf                # Monospace UI font
│   └── sprites/                # 2:1 isometric pixel art sprites
├── src/                        # Engine framework
│   ├── _global.hpp             # Shared simulation globals
│   ├── AssetManager.hpp        # Texture, font, and sound manager
│   ├── Entity.hpp              # Base entity with safe drawing & destruction
│   ├── GameEngine.hpp          # Window management, loop, event pump
│   └── Scene.hpp               # Scene lifecycle interface
└── game/                       # Game simulation code (all lowercase subdirectories)
    ├── main.hpp                # Main game class derived from GameEngine
    ├── entities/               # Game objects (zones, roads, civics, player cursor)
    ├── simulation/             # HappinessSystem 9-domain citizen satisfaction engine
    ├── world/                  # WorldData (terrain/resource layer) & WorldGen (procedural generator)
    ├── scenes/                 # Play_Scene (simulation scene) & Menu_Scene
    └── ui/                     # Toolbar (TheoTown dock) & Window (HUD panels)
```
