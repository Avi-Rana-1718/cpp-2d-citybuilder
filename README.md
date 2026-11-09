# Chime V2: 2D Isometric City Builder

A 2D isometric city builder in **C++17** with **SFML 2.6.1** and **CMake**, inspired by mobile city builders such as TheoTown.

Every game starts on a procedurally generated 64×64 island. Build roads, homes, shops, industry and farms. Fund schools, churches and parks. Keep the budget in the black through the seasons.

![A starter town with the build menu and inspect card open](docs/images/city.png)

---

## Features

- **Procedural worlds:** seeded terrain with ocean coastlines, lakes, rivers, beaches, forests with trails and rocky hills. Each tile also has soil fertility, groundwater and coal / iron / stone deposits. The same seed always gives the same map.
- **Demand by income level:** residential, commercial and industrial demand is tracked separately for low ($), medium ($$) and high ($$$) income. Buildings only fill with people while their income level has demand.
- **Economy:** nine tax sliders, one per sector and income level. Daily tax income, daily upkeep for roads and services, loans, and bankruptcy after 6 months in debt.
- **Time and seasons:** pause, 1x, 2x or 4x speed and a 360-day calendar. Seasons change the look of the map: autumn forests, winter snow. Farms also produce more or less by season.
- **Services and happiness:** schools, churches and parks cover nearby homes. Happiness depends on jobs, education, health, safety and taxes.
- **Interface:** sidebar build menu with building thumbnails and prices, status bar, budget panel, inspect card, map views for fertility, groundwater and deposits. The window can be resized freely.

![Budget panel in autumn](docs/images/budget-autumn.png)

---

## Build and run

**Requirements:** a MinGW / GCC compiler with C++17 support, and CMake 3.20 or newer. SFML 2.6.1 is used from the bundled `SFML/` folder or downloaded automatically.

```powershell
# Configure
cmake -S . -B build -G "MinGW Makefiles"

# Build the game and the test runner
cmake --build build

# Run the tests (59)
.\build\citybuilder_tests.exe
# or: ctest --test-dir build --output-on-failure

# Play
.\build\citybuilder.exe
```

The build copies the SFML DLLs, the font and all sprites into `build/`.

---

## How to play

1. Lay **roads** (`Space`, drag to draw). Buildings only work when a road runs along one of their edges.
2. Place **homes** (`R`), then **shops** (`C`) and **industry** (`I`) to give residents jobs. Watch the 9 demand bars in the top bar.
3. Add a **school** (`1`) so medium- and high-income residents will move in, and **parks** (`3`) to keep people healthy.
4. Open the **budget** (`B`) to check income against upkeep and adjust taxes. Stay out of debt for long, or the city goes bankrupt.

### Controls

| Input | Action |
|---|---|
| **Left sidebar** | Pick a category, then a building card |
| **Left click / drag** | Build; drag to draw roads or fill areas |
| **Right click** / `Esc` | Cancel the drag or tool; `Esc` also closes the budget |
| `W` `A` `S` `D` / arrows | Pan the camera (`Shift` = faster) |
| `Q` / `E` (or `+` / `-`) | Zoom in / out |
| `R` `C` `I` `F` | Residential, Commercial, Industrial, Farm |
| `1` `2` `3` `H` | School, Church, Park, Harbour |
| `Space` `W` `X` | Road, Water, Demolish |
| `P`, `[`, `]` | Pause, slower, faster (or use the top-bar buttons) |
| `B` | Budget (or click the money chip) |
| `O` | Map view: Normal, Fertility, Groundwater, Deposits |
| `N` | Found a new city after bankruptcy |

### Launch options

| Option | Purpose |
|---|---|
| `--seed N` | Replay a specific world (the seed is shown bottom-right) |
| `--size WxH` | Starting window size, e.g. `--size 1920x1080` |
| `--month N` | Start in month N, e.g. `--month 1` for winter |

Developer options (benchmarks, balance simulation, screenshots) are listed in [agents.md](agents.md).

---

## Project layout

```
cpp-2d-citybuilder/
├── main.cpp                 # Entry point and launch options
├── src/                     # Engine: window, game loop, assets, globals
├── game/
│   ├── scenes/              # Play_Scene: input, building, rendering, HUD
│   ├── entities/            # Zones (R/C/I, farm, civics, harbour), roads
│   ├── world/               # World data, generator, batched ground renderer
│   ├── simulation/          # Economy, game clock, happiness
│   └── ui/                  # UI kit, build menu
├── assets/sprites/          # Pixel-art sprites and UI icons
├── tools/                   # Python sprite / icon generators
├── tests/                   # Test framework and 59 tests
└── docs/                    # Gameplay and simulation guide
```

## Documentation

- [docs/THEOTOWN_SIMULATION_GUIDE.md](docs/THEOTOWN_SIMULATION_GUIDE.md): gameplay mechanics, buildings, economy formulas, seasons and world generation.
- [agents.md](agents.md): architecture, conventions and developer tooling for contributors and coding agents.
