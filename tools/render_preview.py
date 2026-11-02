#!/usr/bin/env python3
"""
Renders a visual snapshot of the 10x Expanded TheoTown Metropolis:
- 64x64 Tile Grid (4,096 tiles, 10x larger than original 20x20)
- 8 Horizontal Avenues and 6 Vertical Boulevards connecting all districts
- Residential Suburbs, Urban Brownstones, Highrise Luxury Towers, Downtown Banking Core
- Agricultural Windmill Belt, Heavy Manufacturing Hub, and Deepwater Ocean Port
- Autonomous Vehicles, Citizen Pedestrians, and Dynamic Building Animations
"""
import os, math, random
from PIL import Image, ImageDraw

def render_preview():
    sprites_dir = "assets/sprites"
    sprites = {}
    for f in os.listdir(sprites_dir):
        if f.endswith(".png"):
            sprites[f[:-4]] = Image.open(os.path.join(sprites_dir, f)).convert("RGBA")
    
    scaled = {}
    for k, v in sprites.items():
        scaled[k] = v.resize((v.width * 2, v.height * 2), Image.NEAREST)
    
    r_mid = scaled["road-mid"]
    r_straight = scaled["road"]
    r_rot180 = r_straight.rotate(180)
    
    GRID_W = 64
    GRID_H = 64
    
    canvas_w = 32 * GRID_W + 64 + 100
    canvas_h = 32 * GRID_H + 16 * GRID_W + 64 + 120
    canvas = Image.new("RGBA", (canvas_w, canvas_h), (15, 20, 25, 255))
    
    # 64x64 grid ground and building maps
    ground = [["grass" for _ in range(GRID_H)] for _ in range(GRID_W)]
    buildings = [[None for _ in range(GRID_H)] for _ in range(GRID_W)]
    
    def set_road(x, y):
        if 0 <= x < GRID_W and 0 <= y < GRID_H:
            ground[x][y] = "road"
            
    def set_2x2(rx, ry, bld_key, base_key):
        if 0 <= rx < GRID_W - 1 and 0 <= ry < GRID_H - 1:
            buildings[rx][ry] = bld_key
            for dx in range(2):
                for dy in range(2):
                    ground[rx + dx][ry + dy] = base_key

    # 1. 1x1 Roads Network Across 64x64 Metropolis
    # 8 Major Horizontal Avenues
    avenues_y = [4, 10, 16, 22, 28, 34, 40, 48]
    for ay in avenues_y:
        for x in range(2, 62):
            set_road(x, ay)

    # 6 Major Vertical Boulevards Connecting All Districts
    boulevards_x = [2, 14, 26, 38, 50, 61]
    for bx in boulevards_x:
        for y in range(4, 49):
            set_road(bx, y)

    # 2. District 1 (y = 1): Northern Residential Cottages & Bungalows
    for x in range(4, 59, 2):
        if x in boulevards_x: continue
        if x % 8 == 0:
            set_2x2(x, 1, "park", "grass")
        elif x % 8 == 2:
            set_2x2(x, 1, "church", "grass")
        elif x % 4 == 0:
            set_2x2(x, 1, "res_t1_v2", "zone_residental")
        else:
            set_2x2(x, 1, "res_t1_v1", "zone_residental")

    # 3. District 2 (y = 7): Residential Brownstones & Urban Condominiums
    for x in range(4, 59, 2):
        if x in boulevards_x: continue
        if x % 12 == 0:
            set_2x2(x, 7, "school", "grass")
        elif x % 8 == 0:
            set_2x2(x, 7, "park", "grass")
        elif x % 4 == 0:
            set_2x2(x, 7, "res_t2_v2", "zone_residental")
        else:
            set_2x2(x, 7, "res_t2_v1", "zone_residental")

    # 4. District 3 (y = 13): Highrise Luxury Towers & Sky-Plazas
    for x in range(4, 59, 2):
        if x in boulevards_x: continue
        if x % 12 == 4:
            set_2x2(x, 13, "park", "grass")
        elif x % 8 == 0:
            set_2x2(x, 13, "church", "grass")
        elif x % 4 == 0:
            set_2x2(x, 13, "res_t3_v2", "zone_residental")
        else:
            set_2x2(x, 13, "res_t3_v1", "zone_residental")

    # 5. District 4 (y = 19): Downtown Corporate Banking Highrises & Financial Towers
    for x in range(4, 59, 2):
        if x in boulevards_x: continue
        if x % 12 == 0:
            set_2x2(x, 19, "park", "grass")
        elif x % 8 == 0:
            set_2x2(x, 19, "school", "grass")
        elif x % 4 == 0:
            set_2x2(x, 19, "com_t3_v2", "zone_commercial")
        else:
            set_2x2(x, 19, "com_t3_v1", "zone_commercial")

    # 6. District 5 (y = 25): Shopping Plazas, Diners, Groceries, & Retail Arcades
    for x in range(4, 59, 2):
        if x in boulevards_x: continue
        if x % 12 == 6:
            set_2x2(x, 25, "park", "grass")
        elif x % 8 == 4:
            set_2x2(x, 25, "com_t1_v2", "zone_commercial")
        elif x % 8 == 0:
            set_2x2(x, 25, "com_t1_v1", "zone_commercial")
        elif x % 4 == 0:
            set_2x2(x, 25, "com_t2_v2", "zone_commercial")
        else:
            set_2x2(x, 25, "com_t2_v1", "zone_commercial")

    # 7. District 6 (y = 31): West Farmlands & East Industrial Manufacturing
    for x in range(4, 59, 2):
        if x in boulevards_x: continue
        if x < 26:
            if x % 4 == 0:
                set_2x2(x, 31, "farm_v2", "zone_farm")
            else:
                set_2x2(x, 31, "farm_v1", "zone_farm")
        else:
            if x % 8 == 0:
                set_2x2(x, 31, "ind_t2_v2", "zone_industrial")
            elif x % 6 == 0:
                set_2x2(x, 31, "ind_t2_v1", "zone_industrial")
            elif x % 4 == 0:
                set_2x2(x, 31, "ind_t1_v2", "zone_industrial")
            else:
                set_2x2(x, 31, "ind_t1_v1", "zone_industrial")

    # 8. District 7 (y = 37): West Windmill Homesteads & East Robotics / Clean-Fab Hubs
    for x in range(4, 59, 2):
        if x in boulevards_x: continue
        if x < 26:
            if x % 4 == 0:
                set_2x2(x, 37, "farm_v2", "zone_farm")
            else:
                set_2x2(x, 37, "farm_v1", "zone_farm")
        else:
            if x % 4 == 0:
                set_2x2(x, 37, "ind_t3_v2", "zone_industrial")
            else:
                set_2x2(x, 37, "ind_t3_v1", "zone_industrial")

    # 9. District 8 (y = 49): Shoreline Harbours & Deep Ocean Water
    for x in range(2, GRID_W):
        for y in range(51, GRID_H):
            ground[x][y] = "water"

    for hx in range(4, 57, 4):
        set_2x2(hx, 49, "harbour", "grass")

    def is_road(x, y):
        return 0 <= x < GRID_W and 0 <= y < GRID_H and ground[x][y] == "road"

    # PASS 1: Render All Ground Tiles with Autotiling
    for i in range(GRID_W):
        for j in range(GRID_H):
            px = 32 * i + 50
            py = 32 * j + 16 * i + 50
            g_key = ground[i][j]
            if g_key == "road":
                left = is_road(i - 1, j)
                right = is_road(i + 1, j)
                up = is_road(i, j - 1)
                down = is_road(i, j + 1)

                if (left or right) and (up or down):
                    sp = r_mid
                elif left and right:
                    sp = r_mid
                elif up and down:
                    sp = r_mid
                elif left:
                    sp = r_rot180
                elif right:
                    sp = r_straight
                else:
                    sp = r_straight
            else:
                sp = scaled[g_key]
            canvas.alpha_composite(sp, (px, py))

    # Active Vehicles Snapshot
    vehicles = []
    v_types = ["sedan", "taxi", "coupe", "van"]
    # Seed traffic across the 8 avenues
    random.seed(42)
    for ay in avenues_y:
        for vx in range(4, 60, 6):
            vt = random.choice(v_types)
            vd = "se" if (vx % 12 == 4) else "nw"
            vehicles.append((vx, ay, vt, vd, 0.5, (-2 if vd == "se" else 2, 4 if vd == "se" else -4)))

    # Seed traffic on the 6 boulevards
    for bx in boulevards_x:
        for vy in range(6, 46, 8):
            vt = random.choice(v_types)
            vd = "sw" if (vy % 16 == 6) else "ne"
            vehicles.append((bx, vy, vt, vd, 0.5, (-4 if vd == "sw" else 4, -2 if vd == "sw" else 2)))

    # Active Pedestrians Snapshot
    pedestrians = []
    for ay in [4, 10, 16, 22, 28]:
        for px in range(5, 59, 4):
            ct = random.randint(0, 3)
            pedestrians.append((px, ay, ct, random.randint(0, 1), -10, -4))

    # Citizens in parks and civic grounds
    civic_spots = [(12, 7), (20, 13), (28, 19), (36, 25), (16, 1), (44, 7), (52, 13)]
    for (cx, cy) in civic_spots:
        pedestrians.append((cx, cy, random.randint(0, 3), 0, 4, 6))
        pedestrians.append((cx + 1, cy + 1, 3, 1, -6, 10))

    # PASS 2: Render All Rectangular Buildings, Vehicles, and Pedestrians in Depth Order (i + j)
    for depth in range(GRID_W + GRID_H):
        # 1. Draw vehicles at this depth
        for (vx, vy, vtype, vdir, vprog, vshift) in vehicles:
            if vx + vy == depth:
                v_key = f"car_{vtype}_{vdir}"
                if v_key in scaled:
                    v_sp = scaled[v_key]
                    sx = 32 * vx + 50 + 32
                    sy = 32 * vy + 16 * vx + 50 + 16
                    if vdir == "se":
                        tx, ty = sx + 32, sy + 16
                    elif vdir == "nw":
                        tx, ty = sx - 32, sy - 16
                    elif vdir == "sw":
                        tx, ty = sx, sy + 32
                    else:
                        tx, ty = sx, sy - 32
                    pos_x = sx + vprog * (tx - sx) + vshift[0] - v_sp.width // 2
                    pos_y = sy + vprog * (ty - sy) + vshift[1] - v_sp.height // 2
                    canvas.alpha_composite(v_sp, (int(pos_x), int(pos_y)))

        # 2. Draw pedestrians at this depth
        for (px_tile, py_tile, ctype, cframe, ox, oy) in pedestrians:
            if px_tile + py_tile == depth:
                p_key = f"citizen_{ctype}_walk_{cframe}"
                if p_key in scaled:
                    p_sp = scaled[p_key]
                    base_x = 32 * px_tile + 50 + 32 + ox - p_sp.width // 2
                    base_y = 32 * py_tile + 16 * px_tile + 50 + 16 + oy - p_sp.height
                    canvas.alpha_composite(p_sp, (int(base_x), int(base_y)))

        # 3. Draw 2x2 buildings at this depth
        for i in range(depth + 1):
            j = depth - i
            if 0 <= i < GRID_W and 0 <= j < GRID_H:
                b_key = buildings[i][j]
                if b_key is not None and b_key in scaled:
                    b_sp = scaled[b_key]
                    px = 32 * i + 50
                    py = 32 * j + 16 * i + 50
                    draw_y = py + 80 - b_sp.height
                    canvas.alpha_composite(b_sp, (px, draw_y))

    # PASS 3: Dynamic Building Animations
    # 1. Spinning Windmills
    if "windmill_blades" in scaled:
        w_sails = scaled["windmill_blades"].rotate(45, resample=Image.BICUBIC)
        for fx in range(4, 26, 4):
            for fy in [31, 37]:
                farm_x = 32 * fx + 50
                farm_y = 32 * fy + 16 * fx + 50 + 80 - scaled["farm_v2"].height
                hub_x = farm_x + 36 - w_sails.width // 2
                hub_y = farm_y + 16 - w_sails.height // 2
                canvas.alpha_composite(w_sails, (hub_x, hub_y))

    # 2. Billowing Smokestacks on Factories
    for ix in range(28, 60, 4):
        for iy, ikey in [(31, "ind_t1_v1"), (37, "ind_t3_v1")]:
            fac_x = 32 * ix + 50
            fac_y = 32 * iy + 16 * ix + 50 + 80 - scaled[ikey].height
            puffs = [(12, -4, 0), (16, -18, 1), (22, -34, 2), (28, -50, 3)]
            for (off_x, off_y, frame) in puffs:
                p_key = f"smoke_puff_{frame}"
                if p_key in scaled:
                    smk = scaled[p_key]
                    canvas.alpha_composite(smk, (fac_x + off_x, fac_y + off_y))

    # 3. Sparkling Park Fountains
    if "fountain_1" in scaled:
        fnt = scaled["fountain_1"]
        for fx in range(8, 56, 12):
            for fy in [1, 7, 13, 19, 25]:
                park_x = 32 * fx + 50
                park_y = 32 * fy + 16 * fx + 50 + 80 - scaled["park"].height
                canvas.alpha_composite(fnt, (park_x + 48 - fnt.width // 2, park_y + 26 - fnt.height // 2))

    # 4. Skyscraper Rooftop Red Warning Beacons
    if "beacon_light" in scaled:
        bcn = scaled["beacon_light"]
        for bx in range(4, 60, 4):
            for by, bkey, off_y in [(13, "res_t3_v1", -12), (19, "com_t3_v1", -24)]:
                b_x = 32 * bx + 50
                b_y = 32 * by + 16 * bx + 50 + 80 - scaled[bkey].height
                canvas.alpha_composite(bcn, (b_x + 46 - bcn.width // 2, b_y + off_y - bcn.height // 2))

    # 5. Harbour Crane Amber Warning Lights
    if "harbour_crane_light" in scaled:
        h_light = scaled["harbour_crane_light"]
        for hx in range(4, 57, 4):
            harb_x = 32 * hx + 50
            harb_y = 32 * 49 + 16 * hx + 50 + 80 - scaled["harbour"].height
            canvas.alpha_composite(h_light, (harb_x + 42 - h_light.width // 2, harb_y - 8))

    canvas.save("tools/current_world.png")
    print("Successfully rendered 10x expanded metropolis (64x64) to tools/current_world.png!")

if __name__ == "__main__":
    render_preview()
