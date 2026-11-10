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
| **Map View** | Top right | Normal / Fertility / Groundwater / Deposits / Traffic / Power / Water, with a colour legend |
| **Inspect card** | Right | The hovered building or tile: tier stars, road and outside-connection status, tax or upkeep per day, occupancy, happiness, and the land underneath. On a road: tier, capacity, today's trips and congestion |
| **Toasts** | Bottom | Build errors and debt warnings |

---

## 3. Buildings

All 2×2 buildings need a land road (any tier, not a bridge deck) on one of the **8 tiles that share a full edge** with their footprint. A road that only touches a corner (a diamond tip) does not count.

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
| Dirt Road | `Space` | $10 / tile | – | 40 trips / day | $1 / tile |
| Asphalt Road | | $30 / tile | – | 120 trips / day | $2 / tile |
| Avenue | | $80 / tile | – | 300 trips / day | $5 / tile |
| Roundabout | | $150 | – | junction ×1.5 | +$2 |
| Water Body | `W` | $20 / tile | – | – | – |
| Demolish | `X` | $5 (forest $15, rock $60) | – | – | – |

- **Industry pollutes** within 3 tiles, which hurts the health of nearby homes and lowers residential demand.
- **Farms** don't pollute. Their income depends on soil (×0.4 on barren land up to ×1.6 on rich soil) and on the season (see §6).
- **Harbours** must touch water and a road. Each active harbour adds export demand for low- and medium-income industry.
- **Demolishing** a building returns its tiles to the original ground: grass, sand or forest path.

### Roads
- **Shape:** roads join only across a shared diamond edge. Each tile picks one of 16 pieces (straight, corner, T, crossing, dead end) from its four edge neighbours. A drag always lays an edge-connected L: first along the SE / NW axis, then along NE / SW.
- **Upgrades:** drag a higher tier over existing roads to upgrade them in place; you pay only the price difference. Equal or higher tiers on the path are skipped.
- **Bridges:** drag straight across water, from land to land, over at most **8** water tiles. A deck costs 5× the tier's road price and 3× its upkeep. Bridges only link along their own axis, and buildings cannot use a deck as their road access. Demolishing a deck leaves the water.
- **Roundabouts:** only on a 3- or 4-way land junction. A plain junction carries ×0.7 of its tier's capacity; a roundabout carries ×1.5.
- **Outside connection:** a road network that reaches the **map edge** connects the city to the outside. Buildings on an isolated network still work, but grow at a quarter of the rate, only to half full, and never upgrade. The inspect card shows "No outside link - slow growth".

### Zones
- **Painting:** each R / C / I menu starts with three zone tools: **Low**, **Medium** and **High density** ($5 per tile, drag a rectangle). Water, roads, forest and rock are skipped. **Dezone** (Roads menu, free) removes zones; bulldozing bare zoned ground does too. Demolishing a building keeps its zone.
- **Growth:** once a day, each sector with low-income demand grows up to 1 + demand ÷ 30 (max 3) new tier-1 buildings on zoned lots: all 4 tiles of the 2×2 lot must carry the same zone, be empty and touch a road (pylons block a lot). Growing is free.
- **Density caps the tier:** low-density buildings stay tier 1, medium reach tier 2, high reach tier 3 (upgrades still need a full building, happy residents and demand for the next income level).
- **Placed buildings:** the building cards under the zones place that exact building. It keeps its tier forever (no upgrade, no downgrade) but still fills only while its income level has demand.

### Ranks and unlocks
A new city starts as a **Settlement**. The best population it ever reaches decides its rank, and each rank pays a one-off bonus and unlocks buildings:

| Rank | People | Bonus | Unlocks |
|---|---|---|---|
| Village | 300 | $5,000 | Asphalt, medium-density zones, placed tier-2 buildings, police, clinic, church, solar, pumps, water treatment & towers, bus depot, sewage outflow |
| Town | 1,000 | $15,000 | Avenues, roundabouts, high-density zones, placed tier-3 buildings, coal, incinerator, sewage treatment, mines, harbours |
| City | 2,500 | $30,000 | Recycling centres |
| Metropolis | 5,000 | $60,000 | - |

Sandbox cities (New City → Sandbox) have everything unlocked.

### Trade
Industry makes goods (1 / 1.4 / 2 per worker by tier, mines 0.5), farms make food (2 per farmer × soil × season). Residents use 0.1 goods and 0.15 food each, shop workers 0.2 goods. The surplus is **exported** ($0.50 per good, $0.35 per food) and shortfalls **imported** for 30% more, both limited by the **trade capacity**: 200 a day for every dirt road leaving the map (asphalt ×2, avenue ×3) plus 500 per harbour. Needs nobody can import make shops lose customers; goods nobody can ship make industry lose orders. The statistics panel (`G`) shows the breakdown.

### City services
| Building | Cost | Upkeep | Does |
|---|---|---|---|
| Police station | $400 | $12 | Safety for homes within 7 tiles. From 500 residents, homes without police lose safety |
| Fire station | $400 | $12 | Puts out fires within 7 tiles the next day and stops new ones there |
| Clinic | $500 | $15 | Health for homes within 7 tiles |
| Landfill | $600 | $8 | Stores garbage (up to 300 a day) until 15,000 are used up; a full landfill is done for good. Pollutes radius 3 |
| Incinerator | $1,400 | $15 | Burns 200 garbage a day and turns it into 100 power on its grid. Pollutes radius 5 |
| Recycling centre | $1,100 | $20 | Recycles 120 garbage a day, cleanly |
| Mine | $800 | — | 30 industrial jobs on a coal / iron / stone deposit; pays more on rich deposits and slowly uses them up; boosts industrial demand |
| Bus depot | $700 | $15 | 30% of commuters within 8 tiles take the bus instead of a car |

- **Garbage:** every occupant makes 0.1 garbage a day. Trucks take it by road to the facilities on the same road network: recycling first, then incinerators, then landfills. Garbage never vanishes on its own; a building no facility can take loses 15 health for its residents (from 150 citizens).
- **Sewage:** waste water (as much as a building's water use) runs through roads and pipes to a **sewage outflow** ($250, $2/day, must touch water, pollutes radius 4) or a **sewage treatment plant** ($1,200, $8/day), 300 a day each. From 400 citizens, homes without sewers lose 15 health. Both are in the **Waste & Sewage** menu with the garbage buildings.
- **Buses:** each bus depot runs 3 yellow buses on the roads around it.
- **Fires** start only once a town has 300 residents: rarely, at random, in any building except parks, farms, fire stations and utilities. An unchecked fire spreads to nearby buildings and burns its building down after 4 days (the zone stays). The message log (`L`) lists every fire.
- **Land value** (0-100, Land value map view): 30 + parks, schools, shops, police, clinics and nearby water, minus pollution and jammed roads. A building needs **40** to reach tier 2 and **60** for tier 3; zones grow the richest wanted tier the density and land value allow.
- **Saving:** `F5` saves to `city.sav`, `F9` loads it (or the monthly / on-exit autosave). `Z` undoes the last build and refunds its cost.

### Traffic (daily)
- Every occupied, road-connected home sends **60%** of its residents to work. Each home searches its road network (asphalt is 1.5× and avenues 2× as fast as dirt) and fills the **nearest jobs** first; a job building takes as many commuters as it has workers.
- Commuters with no job left in reach drive to the nearest map-edge road if the network has an outside connection.
- Each trip adds 1 to every road tile on its route. **Congestion** = trips ÷ capacity (with the junction factors above).
- A home whose average route congestion is over 100% loses up to **20 happiness** (20 × (congestion − 1)). City-wide jams over 100% cut commercial demand by up to 25, and the budget hint says "Traffic jams".
- The **Traffic** map view colours roads green (free) → yellow (busy) → red (jammed). Cars on the map follow the same loads and slow down on jammed tiles; they are visual only.

### Power & water
Utilities travel along **roads, power lines and water pipes**. Every road carries both; a power line (dragged, $8 per tile, overground pylons) carries power, a pipe (dragged, $6 per tile, underground, can pass under roads and buildings) carries water. Lines cost $1 a day per 10 tiles. Everything connected shares one grid, and a building touching two networks joins them. Power and water buildings need **no road** — only a road or line of their own kind touching them.

| Building | Cost | Upkeep | Does |
|---|---|---|---|
| Coal plant | $1,500 | $20 | 400 power, 500 on a coal deposit. Pollutes radius 5 |
| Wind turbines | $400 | $3 | 60 power |
| Solar farm | $800 | $8 | 120 power × season (summer 1.2, autumn 0.8, winter 0.5) |
| Transformer | $300 | $4 | Delivers up to 150 power to the buildings on its grid |
| Water well | $150 | $1 | 80 clean water × groundwater (radius 2) |
| Underground pump | $600 | $8 | 400 raw water × groundwater (radius 4) |
| Water treatment | $900 | $12 | Cleans 400 raw water. Mild pollution radius 3 |
| Water tower | $350 | $5 | Stores 300 spare clean water for dry days |
| Power line | $8 / tile | $1 per 10 tiles | Carries power; blocks buildings, crosses open land |
| Water pipe | $6 / tile | $1 per 10 tiles | Carries water underground |

- **Power needs transformers.** A plant alone powers nothing. A grid delivers the smaller of its plant output and 150 power per transformer, to any building connected to it, however far away; add transformers as demand grows. A plant that is cut off or has no transformer on its grid blinks a warning, and its inspect card says why.
- **Seeing the networks:** the Power and Water map views draw every line, pipe and road that carries them; choosing a power or water tool switches to that view automatically.
- **Groundwater is a reserve.** Wells and pumps lower it in their radius as they deliver water, and their output falls with it. It recharges slowly towards its natural level (faster in spring and near lakes, slower in summer). The Groundwater and Water views show the live reserve; the placement pill shows a well's or pump's output at the cursor.
- **Untreated water:** pump water beyond the treatment capacity still reaches homes but costs them 20 health.
- **Demand** = occupants × use per person (power: homes 0.25, shops 0.5, industry 0.8; water: 0.2 / 0.1 / 0.3) × tier (1 / 1.3 / 1.7) × city size (1 + population / 3,000). Empty buildings still need 10%.
- **Penalty:** towns under **150** citizens need no utilities. After that, a building without power or water gets no new occupants and no upgrades and slowly shrinks to a quarter full; homes lose 12 happiness per missing utility. Blinking icons mark unserved buildings, the top bar shows power and water delivered vs needed, and a toast warns about shortages at most once a week.

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
- **No outside connection:** a quarter of that gain, up to half of capacity (a fuller building loses a tenth of its capacity per day down to half).
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
| Groundwater | Underground water reserve for wells and pumps; drains as they pump, recharges slowly |
| Coal / iron / stone | Ore clusters (reserved for future mines) |

**Map View** (top right or `O`) tints the map:
- **Fertility:** red = barren, green = rich.
- **Groundwater:** deeper blue = more water (the live reserve).
- **Traffic / Power / Water:** road congestion; served (green) vs unserved (red) buildings, plants and transformer range; clean / untreated / dry buildings over the reserve.
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
