# Chime V2: Agent & Developer Guide

A 2D isometric city builder in **C++17** with **SFML 2.6.1** and **CMake**. The player builds roads (three tiers, bridges, roundabouts) and 2×2 zones on a procedurally generated 64×64 isometric map. They manage demand by income level, a daily budget (taxes, upkeep, loans), traffic, seasons and citizen happiness.

The whole game compiles as **one translation unit**: `main.cpp` includes headers that hold their own definitions. `src/HeadlessWindow.cpp` is the single extra source file, and only the game target uses it.

---

## Build, Test & Run

```bash
cmake -S . -B build -G "MinGW Makefiles"   # configure
cmake --build build                         # game + tests
.\build\citybuilder_tests.exe               # all 100 tests must pass
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
| `--bench-city` / `--bench-town` | Pre-build a ~650-building grid city / an ~18-building starter town (its two streets are joined and run out to the nearest map edge) |
| `--starter` | Showcase town: the starter town plus asphalt main street, avenue link, roundabout on a spur junction and the full utility chain (coal plant, transformer, pump, treatment, tower, plus an off-road wind farm on a power line and a well on a pipe), medium-density R and low-density C zones along a NE spur that fill during the warm-up, with a $75k sandbox treasury and 60 days pre-simulated. Prints each utility's tile for `--inspect` |
| `--load file.sav` / `--save file.sav` | Open a saved city at start-up / save the city when `--bench` ends |
| `--fast-forward N` | Simulate N days at start-up, printing money, population, demand and traffic congestion every 30 days (balance testing) |
| `--menu N`, `--inspect X Y`, `--view N`, `--budget`, `--messages` | Screenshot helpers: open build category N (1-7), select a tile, set the map view (0-7: 4 Traffic, 5 Power, 6 Water, 7 Land value; power / water tools switch to their view automatically), open the budget / the message log |

Typical checks:
- **Speed:** `citybuilder.exe --seed 42 --bench 5 --bench-city`
- **Balance:** `citybuilder.exe --seed 42 --bench-town --fast-forward 360 --bench 0.1`

---

## Controls

| Input | Action |
|---|---|
| **Left sidebar** | Category icons (Inspect, Residential, Commercial, Industry & Farms, Civic & Ports, Roads, Power & Water) open a flyout of build cards |
| **Left click + drag** | Build; roads / water / demolish drag along a path or area with a live preview. Road drags show build (green), upgrade (yellow), bridge (blue), skipped (grey) and blocked (red) tiles, and the pill shows the drag's total cost |
| **Right click** | Cancel drag, or return to Inspect |
| `W A S D` / arrows, `Q E` / `+ -` | Pan (`Shift` faster), zoom 0.4×-3.2× |
| Hotkeys | `Esc` Inspect (also closes the budget), `R` `C` `I` low-density R/C/I zone, `F` Farm, `1` `2` `3` School/Church/Park, `H` Harbour, `Space` Road, `W` Water (only when no tool is active), `X` Demolish, `U` Power & Water, `O` map view, `B` budget, `F5` save (`city.sav`), `F9` load (`city.sav`, else `autosave.sav`), `Z` undo the last build, `L` message log, `P` pause, `[` `]` slower/faster, `N` new city after bankruptcy |
| Top bar | Money chip (click = budget), population + 9 demand bars, happiness, date/season, speed buttons |
| Top right | Map View toggles (Normal / Fertility / Groundwater / Deposits / Traffic / Power / Water) and the inspect card |

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
- **Road access**: a building is *active* only when a land road of any tier (`DirtRoad` / `AsphaltRoad` / `Avenue`, never a `Bridge` deck) lies on one of the **8 edge-sharing border tiles** (`BORDER_2X2`, `hasAdjacentRoad()`). Tip-touching `(i, j±1)` neighbours do not count, and connection does not chain through other zones. Harbours also need `hasAdjacentWater()`. Placement (`canPlaceTile`) and activation (`checkValid`) share this rule. Compare road names with `isRoadName()` / `roadTierOfName()`, never `== "DirtRoad"`.
- **Road autotiling**: roads link only across edges. `RoadTile::linkMask()` sets NE `(x+1, y-1)` = 1, SE `(x+1, y)` = 2, SW `(x-1, y+1)` = 4, NW `(x-1, y)` = 8 (`ROAD_DIR`), and the mask is the frame in the tier's 16-frame sheet. A bridge links only along its axis (0 = NE-SW, 1 = SE-NW).
- **Road drags**: `roadDragPath(start, end)` (`game/world/RoadNetwork.hpp`) is an L of two edge axes, `a = dx + dy` steps along `(±1, 0)` then `-dy` steps along `±(1, -1)`. Never step along y alone.

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
        │       └── Road → RoadTile (tier, bridge axis, roundabout, mask)
        │                  └── DirtRoad / AsphaltRoad / Avenue          (tier shorthands)
        ├── World (game/world/)
        │   ├── WorldData       world[64][64] terrain / fertility / groundwater / deposits
        │   ├── WorldGen        seeded generator
        │   ├── GroundRenderer  natural ground in one vertex-array draw + seasonal look
        │   ├── RoadNetwork     roadDragPath, road components + outside connection (global roadNet)
        │   └── CarRenderer     visual cars, drawn in the depth pass
        ├── Simulation (game/simulation/)
        │   ├── GameClock       calendar, seasons, speed (global gameClock)
        │   ├── EconomySystem   demand, occupancy, taxes, upkeep, loans, debt
        │   ├── TrafficSystem   daily commute routing, load / congestion maps (global traffic)
        │   ├── UtilitySystem   power & water grids, groundwater drain / recharge (global utilities)
        │   ├── ServiceSystem   police / fire / clinic / bus coverage, garbage, fires, mines, land value (global services)
        │   └── HappinessSystem service coverage maps + per-home happiness
        └── UI (game/ui/)
            ├── UiKit           palette, cached text / shapes, icons, bars, layout helpers
            └── Toolbar         sidebar + flyout build menu, "Placing" pill, toolCost()
```

### Frame and tick flow
1. **Every frame** (`Play_Scene::update`): input → `renderWorld` → `renderHUD` → `advanceTime(dt)`.
2. **`advanceTime`**: advances `gameClock` by real time × speed (nothing happens while paused or bankrupt).
   - Runs `runSimulationTick()` once per game second, at most 4 catch-up ticks per frame. A tick calls `Zone::simTick()` on every structure, then `HappinessSystem::updateSimulation()`.
   - Runs `runNewDay()` for every day boundary crossed, which calls `traffic.update()` and then `EconomySystem::runDay()` (which first rebuilds `roadNet`).
   - Moves the cars by the scaled game time (they stop while paused).
3. **Map entities have no per-frame logic.** `update()` is not called on them. Road access is re-checked only on change: `checkValid()` runs on placement and from `refreshBuildingsAround()`. Every road change goes through `Play_Scene::onRoadsChanged(x, y)`, which re-tiles (`refreshRoadsAround()`), re-checks nearby buildings and rebuilds `roadNet`.

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
  - **Outside connection:** an active building whose border roads never reach the map edge (`roadNet.buildingOutside()`) fills at ¼ rate, only to half capacity, and never upgrades.
- **Zones** (`world[][].zone`, helpers in `WorldData.hpp`): 9 zone tools (R/C/I × Low/Medium/High density, $5/tile, rectangle drag) plus Dezone. `zone = 1 + sector*3 + density`, `zoneMaxTier = density + 1`. `Play_Scene::growZones()` runs daily before the economy: per sector with low-income demand it grows up to `1 + demand/30` (max 3) tier-1 buildings on lots whose 4 footprint tiles share one zone, are empty clear ground without power lines and touch a road (hash-spread, deterministic). Grown buildings get `zoned = true`, `maxTier`; buildings placed from the cards get `fixedTier = true` and `changeTier` skips them (they still fill with demand). Demolishing keeps the zone; bulldozing bare zoned ground or Dezone removes it. Empty zones are drawn in PASS 1b2.
- **Roads** (`RoadTile`, constants in `EconomyData.hpp`): Dirt / Asphalt / Avenue cost $10 / $30 / $80, upkeep $1 / $3 / $6, capacity 40 / 120 / 300 trips a day, speed 1 / 1.5 / 2. A road tool on a lower tier upgrades in place for the price difference (`getPlacementCost`). `planRoadPath()` classifies every tile of a drag (build / upgrade / bridge / skip / fail) and is shared by the preview and `buildRoadPath()`. Bridges: straight, land to land, at most `BRIDGE_MAX_SPAN` = 8 water tiles, 5× cost, 3× upkeep, named `"Bridge"`, `world[][].terrain` stays `Water`. Roundabouts: $150, +$2/day, only on a land tile with 3+ links.
- **TrafficSystem**: once a day each occupied home sends 60% of residents through a Dijkstra search over its road component and fills the nearest jobs (job buildings take up to their population); the rest drive to the nearest map-edge road. Trips add load per tile; congestion = load / capacity, ×0.7 at plain 3+ way junctions, ×1.5 at roundabouts. Homes over 100% lose up to 20 happiness, city jams over 100% cut commercial demand by up to 25. Feeds the Traffic map view and `CarRenderer`.
- **UtilitySystem** (`UtilitySystem.hpp`, buildings in `UtilityZone.hpp`, constants in `EconomyData.hpp`): utilities travel along *conductors*: every road carries both, power lines (`world[][].lines & LINE_POWER`, overground, $8/tile) carry power and pipes (`LINE_PIPE`, underground, also under roads and buildings, $6/tile) carry water; lines cost $1/day per 10 tiles. Edge-sharing conductors form networks; a building joins every network touching its footprint or border, so one building touching two networks merges them. Power and water have separate grids (`powerGrids` / `waterGrids`). Utility buildings need no road (`isActive` is always true); `UtilityZone::linked` / `noTransformer` drive the warning icon, inspect status and placement toast.
  - **Power:** coal (400 MW, ×1.25 on a coal deposit, pollution r5), wind (60 MW), solar (120 MW × season: summer 1.2, autumn 0.8, winter 0.5). Consumers only get power through **transformers** on their grid: a grid delivers min(plant output, 150 MW × transformers) to any building on it, with no distance limit. A plant without a transformer powers nothing.
  - **Water:** wells (80 clean × groundwater around them, radius 2), underground pumps (400 raw × groundwater, radius 4), treatment plants (clean 400 raw), towers (store 300 spare clean water, cover later shortfalls). Untreated raw water is still delivered but costs homes 20 health.
  - **Groundwater:** `world[][].groundwater` is the live reserve and `groundwaterMax` the natural level. Each source drains 0.05 points per unit delivered, spread over its radius; tiles recharge 0.15 / day towards the max (×2 spring, ×0.5 summer, ×2 within 3 tiles of water).
  - **Demand:** occupants (at least 10% of capacity) × per-capita use (power R 0.25 / C 0.5 / I 0.8, water R 0.2 / C 0.1 / I 0.3) × tier factor (1 / 1.3 / 1.7) × city factor (1 + population / 3000). Farms and civic buildings need nothing.
  - **Allocation:** scan order, each building all-or-nothing; clean water first, then towers, then untreated. `runDay()` (from `EconomySystem::runDay`) also fills towers and drains / recharges groundwater; `refreshService()` runs after any build (via `Play_Scene::utilitiesDirty`) and changes nothing.
  - **Penalties:** below 150 citizens everyone counts as served. Above it, an unserved building gets no move-ins or upgrades and shrinks by cap/20 a day to cap/4; homes lose 12 happiness per missing utility (`happinessIndex[7]`, `townStats[7]`); `limitingFactor` says "No power" / "No water" / "Dirty water".
- **ServiceSystem** (`ServiceSystem.hpp`, buildings in `ServiceZone.hpp`, constants in `EconomyData.hpp`): police, fire station, clinic (coverage r7: 100 inside ⅔ of the radius, fading out), bus depot (r8: 30% of commuters there leave the car, read by `TrafficSystem`), garbage disposal (each R/C/I occupant makes 0.1 a day; trucks take it by road to the facilities on the building's road network: recycling centre 120/day clean, then incinerator 200/day which pollutes r5 and adds 0.5 power per unit to its power grid, then landfill which accepts 300/day and *stores* it until 15,000 are used up — `ServiceZone::stored`, filled in `runDay()`, a full landfill takes nothing and raises a message; under 150 citizens nobody minds) and mine (30 low-wealth industrial jobs on a deposit, income × deposit richness, uses 0.02 deposit points per miner a day, each working mine adds up to +8 low/mid industrial demand). `runDay()` handles fires (only with ≥ 300 residents; 1 in 4000 per building per day, spreading 1 in 6 a day to buildings within 2 tiles, destroying after 4 days; a fire station in range puts them out and stops new ones; parks, farms, fire stations and utilities don't burn; lots keep their zone). `computeLandValue()` (daily) = 30 + parks 0.2 + education 0.15 + shops 0.1 + police 0.1 + clinic 0.1 + 12 near water − pollution 0.5 − up to 15 for jammed roads. Upgrades need land value 40 (tier 2) / 60 (tier 3), and zones grow the richest wanted tier the density and land value allow. Happiness: safety = 55 + faith/3 + police/2 (−15 crime from 500 residents without police), health also −15 without garbage pickup, + clinic/4.
- **Sewage** (in `UtilitySystem`, water grids): each building's waste water (= its water use) needs a sewage outflow (cheap, must touch water, pollution r4) or treatment plant (r2) on its water grid (roads + pipes), 300 each. Unsewered homes lose 15 health; towns under 400 people are exempt (`sewageGrace`).
- **Buses**: `CarRenderer` keeps `BUSES_PER_DEPOT` yellow buses (model `BUS`) driving out from each active depot.
- **Undo** also covers tax slider changes (`UndoStep::taxOnly`).
- **Save / load** (`game/scenes/SaveGame.hpp`): plain text, `CHIMESAVE 1`: seed / money / spawn, clock, taxes, economy, loans, every tile's `TileData`, roads (tier, bridge axis, roundabout) and buildings (name, tier, variant, population, capacity, fixedTier, zoned, maxTier, happiness, kind, stored). Everything derived is rebuilt on load. `readCity(in, keepTime)` with `keepTime` is how **undo** works: each drag action snapshots the city first (`executeMultibuild`, last 10), and undo restores the map but keeps the calendar and refunds what the action cost. Autosave to `autosave.sav` monthly and on close (not in benchmarks / headless). `loadError` says why a load failed.
- **Messages**: `notify()` = toast + entry in the message log (`L` or the bottom-right chip): shortages, debt, fires, saves.
- **HappinessSystem**: coverage maps (education R5, church R4, park R4, pollution R3, commerce R3) and per-home happiness. Weights: jobs 0.25, education 0.20, health 0.20, safety 0.15, tax 0.20, minus the traffic penalty. It does **not** change tiers; the economy does.
- **GroundRenderer**: builds a texture atlas at runtime and one vertex array for every ground tile. It is rebuilt only after `markDirty()` or a `SeasonLook` change. Draws a snow layer, and forest / rock sprites in the depth-sorted pass.
- **Toolbar**: the build menu. `toolCost()` is the single price table and `toolThumbnailKey()` maps each tool to its card / ghost sprite.
- **UiKit**: `roundedRect`, `panel`, `text` / `textMid` / `textRight`, `fitText`, `icon`, `thumbnail`, `bar`, `formatMoney`. Text and shape geometry are cached by content (`cachedText`, shape map), which keeps the HUD cheap.

### Conventions
- **Natural ground is data, not entities.** Use `tileKind(x, y)`: it returns the structure's name, or `Grass` / `Sand` / `Water` / `Forest` / `Rock` / `ForestPath` for an empty slot. After changing `world[][].terrain`, call `ground.markDirty()`.
- **Adaptive layout.** Lay out HUD elements with `uikit::SW()` / `uikit::SH()` (the live `screenW` / `screenH`), never `WINDOW_W` / `WINDOW_H`, which are only the preferred start size. Map mouse positions with `uikit::hudFromPixel()`. The HUD view is 1:1 with the window, and the world view is window size × zoom, so nothing stretches.
- **Determinism.** World generation uses its own `std::mt19937`. Economy "random" upgrades use a hash of position and day, never `std::rand`, so tests stay stable.
- **Tests construct `Play_Scene` without `init()`.** Keep logic reachable without a window or textures, and reset globals in `clearTestGrid()` (world, clock, tax rates, `roadNet`, `traffic`). Tests that need growth put a road to the map edge with `connectToOutside()`.

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
- `tools/gen_utility_sprites.py`: the eight `util_*` power & water building sprites (48×H, 2×2 lots).
- `tools/gen_road_sprites.py`: `road_dirt` / `road_asphalt` / `road_avenue` / `roundabout` (512×16, 16 mask frames each), `road_bridge` (6 frames of 32×24: tier × 2 + axis) and the build-card `*_icon` thumbnails. Off-road pixels are transparent, so the ground underneath shows.

---

## Tests (`tests/test_main.cpp`, 100)

A small custom framework (`tests/test_framework.hpp`). Areas covered:

- **Geometry & input**: projection, diamond picking, drag paths, visible-tile range vs brute force, HUD adapting to window size.
- **Build menu**: layout, flyout click flow, tool names, `toolCost()` consistency.
- **Placement & roads**: road / water / forest / rock blocking, edge-sharing road access, on-change re-checks, all 16 autotile masks, edge-connected drag paths, tier upgrades, bridge rules, roundabouts, outside connection, demolition restoring terrain.
- **Traffic**: commute routing and loads, capacity by tier, the happiness penalty, cars staying on roads.
- **Waste / sewage / buses**: disposal order and landfill filling up, incinerator power, sewage grace / plants / outflow placement / health, tax undo, buses leaving depots.
- **Save / undo / services**: save round trip (and bad files change nothing), undo refunds, coverage and garbage by road network, fires burning down / put out, mines (deposit rule, depletion, tax) and buses (fewer trips), land value formula and its upgrade / growth gates.
- **Zones**: painting / dezoning, growth with demand and road, no growth on bad lots, density caps upgrades, placed buildings keep their tier, demolished lots regrow.
- **Utilities**: lines connecting off-road buildings, line placement / bulldoze rules, line upkeep, two-column flyout on small windows, grace period, transformers (needed, pooled capacity, no range), grids following roads and merging, output by season / deposit / groundwater, pumping drain and recharge, treatment and wells, tower buffer, demand scaling, shrinking unserved buildings, happiness and health penalties, upkeep and tools.
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
