# TheoTown City Simulation Guide — Mechanics & Architecture

Welcome to the new city simulation system for **Chime V2**! This document provides a complete guide to all features, mechanics, economic loops, and technical designs implemented in the simulation.

---

## 1. Quick Start & Controls

Launch the game via:
```powershell
.\build\citybuilder.exe
```

### Controls Cheat-Sheet

| Control | Function | Description |
|---|---|---|
| **Mouse Hover** | Tile Selector | Snaps cursor diamond with collision detection to isometric grid. |
| **Left Click + Drag** | **Drag-to-Multibuild** | Drag across the grid to preview and batch-build zones, civics, or roads on mouse release. |
| **Right Click** | Cancel / Inspect | Cancels active drag gesture or switches active tool back to `INSPECT`. |
| **`W / A / S / D`** or **Arrow Keys** | Camera Pan | Smooth 600 px/sec camera panning across the 2D isometric world. |
| **Bottom TheoTown Dock** | Category Toolbar | Click top category tabs (`INSPECT`, `ZONING`, `CIVICS`, `INFRASTRUCTURE`) or individual tool buttons. |

### Hotkey Directory

- `Esc` : **Inspect Mode** (Deselects tool)
- `R` : **Residential Zone**
- `C` : **Commercial Zone**
- `I` : **Industrial Zone**
- `F` : **Farms** (Agricultural Zone)
- `1` : **Elementary School** (Civic)
- `2` : **Community Church** (Civic)
- `3` : **Nature Park** (Civic)
- `Space` : **Dirt Road**
- `H` : **Trade Harbour**
- `W` : **Water Body**
- `X` : **Demolish / Bulldoze**

---

## 2. TheoTown Bottom Dock (Toolbar)

The bottom dock is modeled directly after TheoTown's categorized building menu:

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│  [INSPECT (Esc)]   │   [ZONING]   │   [CIVICS / SERVICES]   │   [INFRASTRUCTURE]       │
├────────────────────────────────────────────────────────────────────────────────────────┤
│  Tool 1            │   Tool 2     │   Tool 3                │   Tool 4                 │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

1. **INSPECT Tab**: Puts the cursor in inspection mode without modifying the map.
2. **ZONING Tab**: Residential ($100), Commercial ($100), Industrial ($120), Farms ($60).
3. **CIVICS / SERVICES Tab**: Elementary School ($200), Community Church ($180), Nature Park ($80).
4. **INFRASTRUCTURE Tab**: Dirt Road ($10), Trade Harbour ($250), Water Body ($20), Demolish ($5).

---

## 3. Tiered Zoning & Building Progression

Every developable zone features dynamic growth, road connection validation, capacity management, and visual progression.

### 1. Residential Zones (`ResidentalZone`)
- **Cost**: $100 | **Tax Index**: `townTaxes[0]`
- **Connection**: Requires adjacent `DirtRoad` or another active `ResidentalZone`.
- **Tiers**:
  - **Tier 1 (Suburban Cottages)**: 10–30 population capacity. Standard sprite.
  - **Tier 2 (Brick Apartments)**: Triggered when zone happiness reaches $\ge 60\%$. Capacity $\times 2.0$, income $\times 2.2$. Visual green-brick tint.
  - **Tier 3 (Highrise Estate)**: Triggered when zone happiness reaches $\ge 80\%$, Education $> 50\%$, and Parks $> 50\%$. Capacity $\times 2.5$, income $\times 2.8$. Modern sky-blue luxury tint.

### 2. Commercial Zones (`CommericalZone`)
- **Cost**: $100 | **Tax Index**: `townTaxes[1]`
- **Connection**: Requires adjacent `DirtRoad` or another active `CommericalZone`.
- **Tiers**:
  - **Tier 1 (Corner Shops)**: 15–40 job capacity. Custom commercial storefront sprite.
  - **Tier 2 (Shopping Plazas)**: Auto-develops once town population surpasses 50 citizens. Capacity $\times 1.8$, income $\times 2.0$.
  - **Tier 3 (Corporate Towers)**: Auto-develops once town population surpasses 120 citizens. Capacity $\times 2.2$, income $\times 2.5$. Golden commerce tint.

### 3. Industrial Zones (`IndustrialZone`)
- **Cost**: $120 | **Tax Index**: `townTaxes[2]`
- **Connection**: Requires road adjacency.
- **Pollution**: Emits heavy smog in a 3-tile radius, reducing health and happiness of nearby residential zones.
- **Tiers**:
  - **Tier 1 (Workshops)**: 15–40 industrial labor jobs.
  - **Tier 2 (Manufacturing Plants)**: Upgrades when industrial demand $> 200$. Capacity $\times 1.8$.
  - **Tier 3 (Heavy Industry)**: Upgrades when industrial demand $> 400$. Capacity $\times 2.2$.

### 4. Agricultural Farms (`FarmZone`)
- **Cost**: $60 | **Tax Index**: `townTaxes[2]`
- **Characteristics**: Low-density food and agrarian production. Generates 10 jobs with zero pollution emission. Ideal buffer zones between residential districts and heavy industry.

---

## 4. Public Services & Infrastructure

Public services project radial coverage across the grid that citizens require to be happy and for residential zones to evolve.

```
       [School: R=5]              [Church: R=4]              [Park: R=4]
    * * * * * * * * *          * * * * * * * * *          * * * * * * * * *
    *   . . . . .   *          *   . . . . .   *          *   . . . . .   *
    * . . . S . . . *          * . . . C . . . *          * . . . P . . . *
    *   . . . . .   *          *   . . . . .   *          *   . . . . .   *
    * * * * * * * * *          * * * * * * * * *          * * * * * * * * *
```

- **Elementary School (`SchoolZone`)**
  - **Footprint**: **2×2 Multi-Tile** | **Sprite**: 48×64 Pixel-Art (`school.png`)
  - **Cost**: $200 | **Coverage Radius**: 5 tiles
  - Features a multi-wing brick hall, bell tower with golden bell, courtyard, and flag pole.
  - Occupies 4 tiles `(x, y)` through `(x+1, y+1)`. Road adjacency along its 12-tile perimeter activates the entire complex.
  - Provides $+100$ Education score to all residences within range. Prerequisite for Tier 3 Highrise estates.
- **Community Church (`ChurchZone`)**
  - **Footprint**: **2×2 Multi-Tile** | **Sprite**: 48×70 Pixel-Art (`church.png`)
  - **Cost**: $180 | **Coverage Radius**: 4 tiles
  - Features stone buttresses, steep slate roof, large arched stained-glass rose window, and gothic belfry spire with a golden crucifix reaching into the skyline.
  - Occupies 4 tiles with unified road connectivity, hover preview, and demolition propagation.
  - Fosters civic order, moral cohesion, and provides $+50$ safety boost.
- **Nature Park (`ParkZone`)**
  - **Cost**: $80 | **Coverage Radius**: 4 tiles
  - Accessible naturally without roads. Absorbs industrial pollution, boosts Health ($+70$) and Recreation ($+100$).
- **Trade Harbour (`HarbourZone`)**
  - **Cost**: $250 | **Placement**: Placed on shoreline adjacent to `WaterTile` or `DirtRoad`.
  - Generates $+30$ maritime trade jobs and strong periodic tax dividends ($+\$100\text{--}\$150$).
- **Water Bodies (`WaterTile`)**
  - **Cost**: $20 | Natural terrain foundation and maritime canal network.

---

## 5. Citizen Happiness System (`HappinessSystem`)

The simulation runs a 9-factor citizen satisfaction model (`src/Scene.hpp` & `game/simulation/HappinessSystem.hpp`):

1. **Taxes** ($20\%$ weight): Citizens dislike high taxes. Satisfaction $= \max(0, 100 - (\text{Tax Rate} \times 4))$.
2. **Jobs & Employment** ($25\%$ weight): Ratio of available jobs (Commercial $+$ Industrial $+$ Harbour $+$ Farms) to working citizens. If jobs $\ge$ workers, score is $100\%$.
3. **Education** ($20\%$ weight): Elementary school radial coverage ($R=5$).
4. **Health** ($20\%$ weight): Base health ($70\%$) $+$ Park coverage (up to $+30\%$) $-$ Industrial pollution smog.
5. **Safety** ($15\%$ weight): Civic stability $+$ Church influence ($R=4$).
6. **Recreation**: Parks $+$ Commercial shopping access.
7. **Traffic**: Road connectivity and proximity.
8. **Ease of Access**: Transit availability.
9. **Total Population**: City scale multiplier.

$$\text{Zone Happiness} = (0.25 \times \text{Jobs}) + (0.20 \times \text{Edu}) + (0.20 \times \text{Health}) + (0.15 \times \text{Safety}) + (0.20 \times \text{TaxSat})$$

- When Happiness $\ge 60\%$, cottages upgrade into Tier 2 brick apartment buildings.
- When Happiness $\ge 80\%$ with education and park coverage, apartments upgrade into Tier 3 highrises.
- If Happiness drops below $40\%$, citizens will begin emigrating and population demand plummets.

---

## 6. Performance & Architecture Highlights

- **Throttled Simulation Loop**: While SFML renders at full 60 FPS, the spatial coverage propagation and economy ticks run on an accumulator clock (`simAccumulator >= 1.0f`). This guarantees zero lag even on large maps.
- **Display-List Ghosting**: The drag-to-multibuild system uses zero-allocation drawing passes for ghost sprites and colored indicator diamonds.
- **Leak-Free Memory Model**: All tile replacements properly invoke virtual entity destructors to clean up textures, shapes, and decrement municipal capacity before instantiating new entities.

---

## 7. Automated Test Suite (20 Tests)

All systems are covered by automated unit and integration tests in `tests/test_main.cpp`:

```powershell
.\build\citybuilder_tests.exe
```

Test coverage includes:
1. `Test_IsometricProjection_Formula`: Mathematical verification of 2:1 isometric diamond mapping.
2. `Test_IsometricRaycasting_DiamondCenter & DiamondTips`: Diamond collision math and bounding-box exclusion.
3. `Test_Toolbar_LayoutAndDefaults & ToolSelection`: Dock coordinate geometry and selection state.
4. `Test_DragComputation_RectangularBox & RoadLPath`: Rectangular and L-path multibuild computations.
5. `Test_Zone_CapacityAccounting`: Verification of `townCapacity` additions and subtractions.
6. `Test_Zone_AdjacencyActivationRules`: Road connection propagation.
7. `Test_Zone_BoundaryEdgeSafety`: Grid edge $[0, 19]$ bounds safety.
8. `Test_Economy_TaxationCycle & PopulationGrowth`: Calendar tax payment and immigration conservation.
9. `Test_Roads_AutotilingLogic`: Autotiling neighbor checks (`road` vs `road-mid`).
10. `Test_IndustrialZone_LifecycleAndTiers`: Industrial tier upgrades and capacity cleanup.
11. `Test_FarmZone_RoadActivationAndTaxes`: Farm road dependency and tax generation.
12. `Test_HarbourZone_WaterAdjacency`: Shoreline water tile adjacency detection.
13. `Test_CivicServices_RadiusesAndTypes`: School, Church, and Park service radiuses.
14. `Test_HappinessSystem_CoverageMaps`: Spatial radial coverage propagation.
15. `Test_HappinessSystem_ScoringAndTierProgression`: Autonomous Tier 1 $\to$ Tier 2 upgrade verification.
16. `Test_Toolbar_NewCategoriesAndToolNames`: Category switching and tool label resolution.
