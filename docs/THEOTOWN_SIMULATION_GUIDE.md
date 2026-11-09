# Chime V2: Gameplay & Simulation Guide

This guide explains how the city works: what each building does, how demand, taxes and upkeep are calculated, how time and seasons work, and how the world is generated. All numbers come from the code (`game/simulation/EconomyData.hpp`, `Economy.hpp`, `HappinessSystem.hpp`, `GameClock.hpp`, `game/ui/Toolbar.hpp`).

---

## 1. Getting started

```powershell
.\build\citybuilder.exe            # random world
.\build\citybuilder.exe --seed 42  # a specific world
```

You start on **1 March, Year 1** with **$20,000**, next to an 8×8 patch of open grass that has water nearby.

A good opening:
1. Draw a road (`Space`, then drag).
2. Line it with **cottages** (`R`).
3. Add a few **shops** (`C`) and **workshops** (`I`) so residents have jobs.
4. Add a **school** (`1`) once you want medium-income residents.
5. Watch the demand bars in the top bar and the budget (`B`).

### Controls

| Input | Action |
|---|---|
| Left sidebar | Category icons open a flyout of build cards (thumbnail, price, hotkey) |
| Left click / drag | Build. Roads, water and demolish follow your drag. Buildings are placed where you release |
| Right click / `Esc` | Cancel the drag or tool and return to Inspect. `Esc` also closes the budget |
| `W A S D` / arrows | Pan (`Shift` = faster) |
| `Q` / `E`, `+` / `-` | Zoom in / out (0.4×-3.2×) |
| `R` `C` `I` `F` | Cottages, Corner Grocery, Scrap Workshop, Farm |
| `1` `2` `3` `H` | School, Church, Park, Harbour |
| `Space` `W` `X` | Road, Water (when no tool is selected), Demolish |
| `P`, `[`, `]` | Pause / resume, slower, faster |
| `B` | Budget panel (or click the money chip) |
| `O` | Map view: Normal → Fertility → Groundwater → Deposits |
| `N` | Found a new city after bankruptcy |

The window can be resized or maximized. A bigger window shows more map, and the interface stays sharp and keeps to the edges.

---

## 2. The interface

| Element | Where | What it shows |
|---|---|---|
| **Build menu** | Left | Six categories. A "Placing …" pill at the bottom-left shows the active tool. Its **x** cancels it |
| **Money chip** | Top bar | Treasury and yesterday's net (`+$28 / day`), or "Debt! N days left". Click it to open the budget |
| **Population chip** | Top bar | Citizens, plus **9 demand bars**: R / C / I groups, each split $ / $$ / $$$. Bars going up mean demand; red bars going down mean a surplus |
| **Happiness chip** | Top bar | City-wide happiness (green ≥ 75, yellow ≥ 50, red below) |
| **Date chip** | Top bar | Date, season and the speed buttons `‖ ▶ ▶▶ ▶▶▶` |
| **Map View** | Top right | Normal / Fertility / Groundwater / Deposits, with a colour legend |
| **Inspect card** | Right | The hovered building or tile: tier stars, road status, tax or upkeep per day, occupancy, happiness, and the land underneath |
| **Toasts** | Bottom | Build errors and debt warnings |

---

## 3. Buildings

All 2×2 buildings need a road on one of the **8 tiles that share a full edge** with their footprint. A road that only touches a corner (a diamond tip) does not count.

| Building | Hotkey | Cost | Income level | Capacity | Upkeep / day |
|---|---|---|---|---|---|
| Suburban Cottages | `R` | $100 | R $ | 20-29 residents | – |
| Brick Apartments | | $250 | R $$ | 50-69 | – |
| Luxury Highrise | | $600 | R $$$ | 120-149 | – |
| Corner Grocery | `C` | $120 | C $ | 15-24 jobs | – |
| Shopping Plaza | | $280 | C $$ | 40-59 | – |
| Corporate Tower | | $750 | C $$$ | 100-129 | – |
| Scrap Workshop | `I` | $150 | I $ | 15-24 jobs | – |
| Factory Plant | | $350 | I $$ | 40-59 | – |
| Clean-Fab Hub | | $800 | I $$$ | 90-119 | – |
| Farm (barn & silo) | `F` | $70 | I $ | 20 jobs | – |
| Windmill Farm | | $90 | I $ | 25 jobs | – |
| Elementary School | `1` | $200 | – | – | $25 |
| Community Church | `2` | $180 | – | – | $20 |
| Nature Park | `3` | $80 | – | – | $10 (always active once built) |
| Trade Harbour | `H` | $250 | C $$ | 30 port jobs | $40 |
| Dirt Road | `Space` | $10 / tile | – | – | $1 / tile |
| Water Body | `W` | $20 / tile | – | – | – |
| Demolish | `X` | $5 (forest $15, rock $60) | – | – | – |

- **Industry pollutes** within 3 tiles, which hurts the health of nearby homes and lowers residential demand.
- **Farms** don't pollute. Their income depends on soil (×0.4 on barren land up to ×1.6 on rich soil) and on the season (see §6).
- **Harbours** must touch water and a road. Each active harbour adds export demand for low- and medium-income industry.
- **Demolishing** a building returns its tiles to the original ground: grass, sand or forest path.

---

## 4. Economy

### Income levels
A building's tier sets its income level: Tier 1 = **$** (low), Tier 2 = **$$** (medium), Tier 3 = **$$$** (high). Every residential, commercial and industrial building belongs to one of **9 buckets** (sector × income level).

### Demand
Demand is computed once per game day for each bucket, from −100 to +100. With *residents* = people living in homes of that income level, and *workers* = 0.6 × residents:

| Sector | Base ($ / $$ / $$$) | Grows with |
|---|---|---|
| Residential | 35 / 15 / 5 | open job capacity (shop + industry buildings at that level) compared with workers |
| Commercial | 10 / 5 / 0 | customers (0.25 × residents) compared with shop jobs already filled |
| Industrial | 20 / 8 / 0 | available labour (0.5 × workers) compared with industry jobs already filled, **+10 per active harbour** (max +30, $ and $$ only) |

Each comparison is `100 × (want − have) / max(floor, want + have)`. Then these modifiers apply:

| Modifier | Effect ($ / $$ / $$$) |
|---|---|
| Tax above 7% | −2.5 / −3.5 / −5 per percentage point, using that bucket's own slider. Below 7% raises demand |
| Happiness (vs 55) | Residential ±0.4 / ±0.7 / ±1.0 per point; commercial and industrial get half |
| Pollution near homes | Residential −0.2 / −0.5 / −1.0 × average pollution |
| Education (school coverage over homes, 0-100) | R $$ −0.8 × (40 − edu); R $$$ −1.0 × (70 − edu); I $$$ −0.8 × (60 − edu). Applies only when coverage is below those thresholds |

Hover a row in the budget panel to see the main reason a bucket's demand is held back, e.g. "Needs more schools" or "Taxes too high".

### People moving in and out (daily)
- **Road-connected building, positive demand:** gains `capacity × (4% + 12% × demand/100)` occupants (at least 1), up to capacity.
- **Demand below −15:** loses `capacity × 8% × |demand|/100` occupants.
- **No road:** loses a fifth of its capacity per day.

### Upgrades and downgrades
- A building moves **up** a tier when all of these are true:
  - it is at least 90% full;
  - the next income level has demand above +25;
  - it is liked enough: residential needs the home's happiness ≥ 60 (→ Tier 2) or ≥ 75 (→ Tier 3); commercial and industrial use city happiness;
  - it wins a 1-in-5 daily roll.
- It moves **down** when its lower income level has demand below −40, it is less than 40% full, and it wins the same roll.
- Farms and harbours don't change tier.

### Taxes
Tax per building per day:

$$\text{occupants} \times \text{income per occupant} \times \text{tax rate}$$

| Income per occupant per day | $ | $$ | $$$ |
|---|---|---|---|
| Residents | $3 | $6 | $12 |
| Shop / office workers | $4 | $8 | $16 |
| Industrial workers | $3.50 | $7 | $14 |

There are nine tax sliders (0-20%, default **9%**), one per bucket. Farms multiply their tax by soil × season. Fractions of a dollar carry over to the next day.

### Upkeep, loans and debt
- **Upkeep** is paid daily; see the table in §3. Homes, shops, industry and farms cost nothing to run.
- **Loans:** $10,000 each, at most 3 at a time. You repay 110% in daily instalments over 360 days.
- **Debt:** money can go below $0, but you can't build anything you can't pay for. Warnings appear on the 1st, 90th and 150th day in debt. **After 180 days in a row in debt the city goes bankrupt**: the game stops and you can found a new city (`N`).

### Budget panel (`B`)
- **Left:** per bucket, a demand bar with its value, the tax slider (drag it), the rate, and yesterday's tax income.
- **Right:** daily expenses (roads with tile count, schools, churches, parks, harbours, loan repayments), the summary (tax income, expenses, net per day), loans (active, still owed, "Take $10,000 loan"), and a red warning strip while in debt.

### Balance reference
From `--fast-forward`, seed 42:
- An 18-building starter town (homes, 3 shops, 3 workshops, a farm, a school and a park) settles at about **+$28/day**.
- A dense 650-building grid with a school every sixth lot loses about $1,400/day and goes bankrupt within the first year.

---

## 5. Happiness and services

Coverage is a circle around each service (radius in tiles):

| Service | Radius | Effect on homes inside |
|---|---|---|
| School | 5 | Education 100 |
| Church | 4 | Safety = 60 + faith/2 |
| Park | 4 | Health +; recreation |
| Industry | 3 | Pollution 60 (lowers health and demand) |
| Commerce | 3 | Recreation (shopping) |

Each home's happiness is

$$0.25\,\text{Jobs} + 0.20\,\text{Education} + 0.20\,\text{Health} + 0.15\,\text{Safety} + 0.20\,\text{TaxSatisfaction}$$

where:
- **Jobs** = min(100, 100 × job capacity / residents).
- **Health** = 70 + parks − pollution.
- **TaxSatisfaction** = 100 − 6 × (average residential tax − 5).

City happiness is the average over all connected homes. It feeds demand (§4) and residential upgrades.

---

## 6. Time and seasons

- **Calendar:** 1 day = 4 s at 1x. A month is 30 days and a year is 12 months (a year lasts 24 minutes at 1x).
- **Speed:** pause, 1x, 2x or 4x. Simulation ticks run once per game second, so 4 per real second at 4x, and stop while paused.
- **Each new day:** the economy runs: demand, people moving in and out, upgrades, taxes, upkeep, loans and the debt clock.

| Season | Months | Look | Farm yield |
|---|---|---|---|
| Spring | Mar-May | Fresh green | ×1.0 |
| Summer | Jun-Aug | Full colour | ×1.3 |
| Autumn | Sep-Nov | Orange and gold forests, drier grass | ×1.5 (harvest) |
| Winter | Dec-Feb | Snow on land, lots and roads; frosted trees; darker water | ×0.3 |

Each season blends into the next over its last ~18 days.

---

## 7. World and natural resources

Every city is a new 64×64 world generated from a seed. Launch with `--seed N` to replay one; the seed is shown bottom-right.

| Feature | Gameplay effect |
|---|---|
| Ocean, lakes, rivers | Can't be built on; harbours need a shoreline |
| Sand beaches | Buildable ground along the ocean coast |
| Forests | Block building; bulldoze for $15 |
| Forest paths | Buildable trails; roads can be laid on them |
| Rocky hills | Block building; bulldoze for $60. Stone and iron are found in and around them |
| Soil fertility | Farm output ×0.4-×1.6; best near rivers and lakes |
| Groundwater | Underground water table (reserved for future water pumps) |
| Coal / iron / stone | Ore clusters (reserved for future mines) |

**Map View** (top right or `O`) tints the map:
- **Fertility:** red = barren, green = rich.
- **Groundwater:** deeper blue = more water.
- **Deposits:** coal black, iron orange, stone white.

**Generation order:** heightmap with a coastline on 1-2 map edges → ocean → lakes → rivers → beaches → hills → forests → forest paths → start area → fertility → groundwater → deposits. Generation is deterministic: the same seed always gives the same world.

---

## 8. Performance notes

- **Ground:** all natural ground is drawn in **one** call, from a vertex array over a runtime texture atlas. It is rebuilt only when terrain or the season look changes.
- **No per-frame logic for map objects:** simulation runs per game second and per game day. Road access and road shapes are re-checked only when something nearby changes.
- **Off-screen tiles are skipped:** draw passes visit only the tiles inside the camera's view.
- **HUD caching:** text and panel shapes are cached, so the interface costs about the same as the old text-only HUD.
- **Benchmark:** `--bench 5 --bench-city` runs off-screen without taking focus. A 650-building city takes about 4-5 ms per frame in a Debug build.

Test coverage is summarized in [agents.md](../agents.md#tests-teststest_maincpp-59).
