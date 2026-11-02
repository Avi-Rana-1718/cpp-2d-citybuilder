# Chime V2 — City Builder Agent Guide

A 2D isometric city-builder written in C++17 with SFML 2.6.1 and CMake.
The player places zones and roads on an isometric tile grid, manages population growth across Residential, Commercial, and Industrial sectors, and balances municipal treasury with periodic tax revenue.

---

## Build, Test & Run

```bash
# Configure with CMake (MinGW Makefiles on Windows)
cmake -S . -B build -G "MinGW Makefiles"

# Build all targets (executable and tests)
cmake --build build

# Run unit and integration tests
ctest --test-dir build --output-on-failure
# Or directly run the test binary:
.\build\citybuilder_tests.exe

# Run game
.\build\citybuilder.exe
```

The binary `citybuilder.exe`, test runner `citybuilder_tests.exe`, required SFML DLLs, and `assets/` directory are output to `build/` automatically via post-build commands.

---

## Controls & Interaction

| Input | Action |
|---|---|
| **TheoTown Bottom Toolbar** | Click any category button (`INSPECT`, `ZONING`, `CIVICS`, `INFRASTRUCTURE`) to arm tools |
| **Mouse Hover** | Highlight and select tile under cursor (isometric diamond collision) |
| **Left Click + Drag** | **Drag-to-Multibuild** — drag a rectangle or road path with live ghost preview, builds on release |
| **Right Click** | Cancel active drag or return to `Inspect` mode |
| **`W / A / S / D`** or **Arrow Keys** | Pan camera smoothly across the world |
| **Hotkeys** | `Esc` (Inspect), `R` (Residential), `C` (Commercial), `I` (Industrial), `F` (Farms), `1` (School), `2` (Church), `3` (Park), `Space` (Dirt Road), `H` (Harbour), `W` (Water), `X` (Demolish) |

---

## Architecture

```
GameEngine
└── Main (game/main.hpp)
    └── Scene (src/Scene.hpp)
        └── Play_Scene (game/scenes/Play_Scene.hpp)
            ├── Entity Grid (entities[20][20])
            │   └── Zone
            │       ├── GrassTile (GlassTile.hpp)
            │       ├── ResidentalZone (T1-T3)
            │       ├── CommericalZone (T1-T3)
            │       ├── IndustrialZone (T1-T3)
            │       ├── FarmZone
            │       ├── HarbourZone
            │       ├── WaterTile
            │       ├── SchoolZone (Civic)
            │       ├── ChurchZone (Civic)
            │       ├── ParkZone (Civic)
            │       └── Road
            │           └── DirtRoad
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
- **Play_Scene (`game/scenes/Play_Scene.hpp`)** — The active simulation coordinator scene. Delegates to `handleCameraMovement()`, `handleToolHotkeys()`, `handleTileHover()`, `handleGestures()`, `renderWorld()`, and `renderHUD()`. Houses periodic throttled simulation accumulator for `HappinessSystem`.
- **Toolbar (`game/ui/Toolbar.hpp`)** — Self-contained TheoTown dock UI component. Organizes 4 categories (`INSPECT`, `ZONING`, `CIVICS`, `INFRASTRUCTURE`) and 11 distinct tools with active state detection and shortcut indicators.
- **HappinessSystem (`game/simulation/HappinessSystem.hpp`)** — High-performance 9-domain satisfaction engine. Computes spatial service coverage (Education, Safety, Health, Recreation, Pollution), weights economic satisfaction (taxes, jobs), and drives autonomous tier upgrades (T1 $\to$ T2 $\to$ T3).
- **Entity (`src/Entity.hpp`)** — Base game object. Owns transform `position`, ID, naming, `isActive` status, and optional rendering primitives (`sprite`, `shape`, `text`). Safely handles rendering dispatch in `draw()` with null guards, and deallocates heap-allocated SFML objects in `~Entity()`.
- **Zone (`game/entities/_Zone.hpp`)** — Extends `Entity`. Base class for developable and natural ground tiles. Tracks `capacity`, `population`, `zoneIncome`, `buildCost`, and tax payment state. Subclasses override `update()`, `checkValid()`, and `payTax()`.
- **ResidentalZone (`game/entities/ResidentalZone.hpp`)** — 2x2 housing zone complex with identical `zone_residental` ground base and 3 happiness-dependent tiers (Tier 1 Cottages/Bungalow, Tier 2 Brick Apartments/Condominiums, Tier 3 Highrise Luxury Towers/Twin Plazas). Auto-upgrades and downgrades based on citizen satisfaction.
- **CommericalZone (`game/entities/CommericalZone.hpp`)** — 2x2 commercial zone complex with identical `zone_commercial` ground base and 3 happiness-dependent tiers (Tier 1 Corner Grocery/Diner, Tier 2 Shopping Plaza/Department Store, Tier 3 Corporate Banking Highrise/Financial Center).
- **IndustrialZone (`game/entities/IndustrialZone.hpp`)** — 2x2 manufacturing complex with identical `zone_industrial` ground base and 3 tiers (Tier 1 Scrap Workshop/Warehouse, Tier 2 Manufacturing Plant/Assembly, Tier 3 High-Tech Clean-Fab/Robotics Hub).
- **FarmZone (`game/entities/FarmZone.hpp`)** — 2x2 agricultural zone complex with `zone_farm` base and variants (Red Barn & Silo, Windmill Homestead).
- **HarbourZone (`game/entities/HarbourZone.hpp`)** — 2x2 maritime trade port placed adjacent to water with cargo cranes, docks, and shipping containers.
- **Civics (`SchoolZone.hpp`, `ChurchZone.hpp`, `ParkZone.hpp`, `SubZone.hpp`)** — 2x2 civic services with radial coverage: Elementary School (R:5), Gothic Cathedral (R:4), and Grand Nature Park with central fountain and shaded grove (R:4).
- **Road & DirtRoad (`game/entities/roads/`)** — 1x1 transportation network autotiling across 4-way and diagonal intersections.
- **Player (`game/entities/Player.hpp`)** — Selector cursor entity. Snaps its position in world space to the currently hovered isometric tile.
- **Window (`game/ui/Window.hpp`)** — HUD component. Automatically sizes its background rectangle to tightly enclose its label text (`resize()`) without allocating leaked heap objects.
- **AssetManager (`src/AssetManager.hpp`)** — Global asset registry holding `m_textures`, `m_fonts`, and `m_sounds`. Searches both local `assets/` and parent `../assets/`.

---

## Global Simulation State (`src/_global.hpp`)

Shared mutable state lives in `_global.hpp`.

| Global | Type | Purpose |
|---|---|---|
| `_window` | `sf::RenderWindow` | SFML window instance (1500 × 900) |
| `view` | `sf::View` | Primary 2D world camera viewport |
| `entities[20][20]` | `Entity*[][]` | 20×20 isometric tile grid |
| `ui` | `std::vector<Entity*>` | UI entities rendered in fixed screen space |
| `bg` | `std::vector<Entity*>` | Background layer entities |
| `money` | `int` | Player municipal funds (starts at $5000 in Play_Scene) |
| `townPopulation[3]` | `int[3]` | Population by zone (0: Residential, 1: Commercial, 2: Industrial) |
| `townCapacity[3]` | `int[3]` | Total available housing/job capacity |
| `townDemand[3]` | `int[3]` | Current zone demand |
| `townTaxes[3]` | `int[3]` | Tax rate percentage per sector (default: 5%) |
| `townStats[9]` | `int[9]` | Aggregated city statistics (Happiness, Education, Health, etc.) |
| `tileX`, `tileY` | `int` | Grid coordinates of the selected tile |
| `gameTime` | `sf::Clock` | Game timer driving the calendar |
| `calculatedFPS` | `int` | Measured frames per second |

---

## Directory Structure

```
cpp-2d-citybuilder/
├── CMakeLists.txt              # CMake build configuration with test target and DLL copy
├── main.cpp                    # Application entry point
├── agents.md                   # Agent guide and architecture reference
├── AGENTS.md                   # Canonical copy of the agent guide
├── README.md                   # Repository overview
├── docs/                       # In-depth system documentation
│   └── THEOTOWN_SIMULATION_GUIDE.md # Comprehensive guide for gameplay, simulation, & civics
├── tests/                      # Automated test suite (20 tests)
│   ├── test_framework.hpp      # Lightweight assertion and runner framework
│   └── test_main.cpp           # Unit & integration test cases
├── assets/                     # Game assets (sprites and fonts)
│   ├── Hack.ttf                # Monospace UI font
│   └── sprites/
│       ├── grass.png           # Ground tile
│       ├── residental.png      # Residential building sprite
│       ├── commercial.png      # Commercial store sprite
│       ├── industrial.png      # Industrial factory sprite
│       ├── farm.png            # Agricultural farm sprite
│       ├── school.png          # Elementary school sprite
│       ├── church.png          # Community church sprite
│       ├── park.png            # Nature park sprite
│       ├── harbour.png         # Trade harbour sprite
│       ├── water.png           # Water tile sprite
│       ├── road.png            # Straight / cap road sprite
│       ├── road-mid.png        # Intersection / straight road mid sprite
│       └── player.png          # Selector cursor sprite
├── src/                        # Engine framework
│   ├── _global.hpp             # Shared simulation globals
│   ├── AssetManager.hpp        # Texture, font, and sound manager
│   ├── Entity.hpp              # Base entity with safe drawing & destruction
│   ├── GameEngine.hpp          # Window management, loop, event pump
│   └── Scene.hpp               # Scene lifecycle interface
└── game/                       # Game simulation code (all lowercase subdirectories)
    ├── main.hpp                # Main game class derived from GameEngine
    ├── entities/
    │   ├── _Zone.hpp           # Base zone entity
    │   ├── GlassTile.hpp       # Grass ground tile (GrassTile class)
    │   ├── ResidentalZone.hpp  # Tiered residential zone logic & tax collection
    │   ├── CommericalZone.hpp  # Tiered commercial zone logic & tax collection
    │   ├── IndustrialZone.hpp  # Tiered industrial zone & pollution emission
    │   ├── FarmZone.hpp        # Agricultural farm zone
    │   ├── HarbourZone.hpp     # Shoreline maritime trade port
    │   ├── SchoolZone.hpp      # Civic education service (R:5)
    │   ├── ChurchZone.hpp      # Civic church / faith service (R:4)
    │   ├── ParkZone.hpp        # Civic nature park / health service (R:4)
    │   ├── WaterTile.hpp       # Water body foundation
    │   ├── Player.hpp          # World cursor entity snapping to hovered tile
    │   └── roads/
    │       ├── _Road.hpp       # Base road entity
    │       └── DirtRoad.hpp    # Autotiling dirt road
    ├── simulation/
    │   └── HappinessSystem.hpp # 9-domain citizen satisfaction engine
    ├── scenes/
    │   ├── Play_Scene.hpp      # Main city builder simulation scene
    │   └── Menu_Scene.hpp      # Main menu scene stub
    └── ui/
        ├── Toolbar.hpp         # TheoTown categorized dock toolbar component
        └── Window.hpp          # Auto-resizing HUD panel
```
