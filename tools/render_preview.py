#!/usr/bin/env python3
"""
Renders a visual snapshot of the TheoTown city simulation world:
- Pass 1: All 1x1 ground diamond tiles (grass, water, roads with authentic autotiling, zone bases)
- Pass 2: Autonomous Vehicles, Pedestrians, and Rectangular 3D Buildings in Depth Order (i + j)
- Pass 3: Dynamic Building Animations (Windmill sails, industrial smoke puffs, park fountain water, skyscraper beacons)
"""
import os, math
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
    
    canvas_w = 32 * 20 + 64 + 100
    canvas_h = 32 * 20 + 16 * 20 + 64 + 120
    canvas = Image.new("RGBA", (canvas_w, canvas_h), (15, 20, 25, 255))
    
    # 20x20 grid ground and building maps
    ground = [["grass" for _ in range(20)] for _ in range(20)]
    buildings = [[None for _ in range(20)] for _ in range(20)]
    
    def set_road(x, y):
        if 0 <= x < 20 and 0 <= y < 20:
            ground[x][y] = "road"
            
    def set_2x2(rx, ry, bld_key, base_key):
        if 0 <= rx < 19 and 0 <= ry < 19:
            buildings[rx][ry] = bld_key
            for dx in range(2):
                for dy in range(2):
                    ground[rx + dx][ry + dy] = base_key

    # 1. Road Avenues Network
    for x in range(1, 19): set_road(x, 3)
    for x in range(1, 19): set_road(x, 6)
    for x in range(1, 19): set_road(x, 9)
    for y in range(3, 10): set_road(1, y)
    for y in range(3, 10): set_road(18, y)

    # 2. Row 1 (y = 1): Residential Sector — ALL BUILDINGS SIDE BY SIDE!
    set_2x2(2,  1, "res_t1_v1", "zone_residental") # T1 Suburban Cottages v1
    set_2x2(4,  1, "res_t1_v2", "zone_residental") # T1 Bungalow Homestead v2
    set_2x2(6,  1, "res_t2_v1", "zone_residental") # T2 Brick Apartments v1
    set_2x2(8,  1, "res_t2_v2", "zone_residental") # T2 Urban Condominiums v2
    set_2x2(10, 1, "res_t3_v1", "zone_residental") # T3 Highrise Luxury Tower v1
    set_2x2(12, 1, "res_t3_v2", "zone_residental") # T3 Twin Highrise Plaza v2
    set_2x2(14, 1, "park",      "grass")           # Grand Nature Park
    set_2x2(16, 1, "church",    "grass")           # Community Cathedral

    # 3. Row 2 (y = 4): Commercial Downtown — ALL BUILDINGS SIDE BY SIDE!
    set_2x2(2,  4, "com_t1_v1", "zone_commercial") # T1 Corner Grocery & Bakery v1
    set_2x2(4,  4, "com_t1_v2", "zone_commercial") # T1 Local Diner & Mini Mart v2
    set_2x2(6,  4, "com_t2_v1", "zone_commercial") # T2 Shopping Plaza Arcade v1
    set_2x2(8,  4, "com_t2_v2", "zone_commercial") # T2 Department Store Arcade v2
    set_2x2(10, 4, "com_t3_v1", "zone_commercial") # T3 Corporate Banking Highrise v1
    set_2x2(12, 4, "com_t3_v2", "zone_commercial") # T3 International Financial Center v2
    set_2x2(14, 4, "school",    "grass")           # Elementary School
    set_2x2(16, 4, "park",      "grass")           # Downtown Plaza Park

    # 4. Row 3 (y = 7): Agriculture & Industry — ALL BUILDINGS SIDE BY SIDE!
    set_2x2(2,  7, "farm_v1",   "zone_farm")       # Barn & Wheat Silo v1
    set_2x2(4,  7, "farm_v2",   "zone_farm")       # Windmill Homestead v2
    set_2x2(6,  7, "ind_t1_v1", "zone_industrial") # T1 Scrap Workshop v1
    set_2x2(8,  7, "ind_t1_v2", "zone_industrial") # T1 Warehouse & Depot v2
    set_2x2(10, 7, "ind_t2_v1", "zone_industrial") # T2 Manufacturing Plant v1
    set_2x2(12, 7, "ind_t2_v2", "zone_industrial") # T2 Heavy Assembly v2
    set_2x2(14, 7, "ind_t3_v1", "zone_industrial") # T3 Microchip Clean-Fab v1
    set_2x2(16, 7, "ind_t3_v2", "zone_industrial") # T3 Robotics Automation Hub v2

    # 5. Row 4 (y = 10): Shoreline Harbours & Water
    for x in range(3, 20):
        for y in range(12, 20):
            ground[x][y] = "water"

    set_2x2(4,  10, "harbour", "grass")
    set_2x2(6,  10, "harbour", "grass")
    set_2x2(8,  10, "harbour", "grass")
    set_2x2(10, 10, "harbour", "grass")
    set_2x2(12, 10, "harbour", "grass")
    set_2x2(14, 10, "harbour", "grass")

    def is_road(x, y):
        return 0 <= x < 20 and 0 <= y < 20 and ground[x][y] == "road"

    # PASS 1: Render All Ground Tiles with Autotiling
    for i in range(20):
        for j in range(20):
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

    # Autonomous Vehicles Snapshot (Coordinates, type, direction)
    active_vehicles = [
        # (tileX, tileY, type, direction, progress, laneShift)
        (3, 3, "sedan", "se", 0.4, (-2, 4)),
        (7, 3, "taxi",  "nw", 0.6, (2, -4)),
        (13, 3, "coupe", "se", 0.2, (-2, 4)),
        (1, 5, "van",   "sw", 0.5, (-4, -2)),
        (5, 6, "van",   "se", 0.7, (-2, 4)),
        (9, 6, "sedan", "nw", 0.3, (2, -4)),
        (14, 6, "taxi", "se", 0.5, (-2, 4)),
        (18, 7, "coupe", "ne", 0.4, (4, 2)),
        (6, 9, "taxi",  "se", 0.3, (-2, 4)),
        (11, 9, "sedan", "nw", 0.8, (2, -4)),
    ]

    # Pedestrians Snapshot (tileX, tileY, citizenType, walkFrame, offsetX, offsetY)
    active_pedestrians = [
        # Sidewalk strollers along North Avenue (y = 3)
        (2, 3, 0, 0, -10, -4),
        (5, 3, 2, 1, 10, 4),
        (8, 3, 1, 0, -10, -4),
        (11, 3, 0, 1, 10, 4),
        # Central Boulevard strollers (y = 6)
        (4, 6, 1, 0, -10, -4),
        (7, 6, 2, 1, 10, 4),
        (12, 6, 0, 0, -10, -4),
        # Students in School Courtyard (14, 4)
        (14, 4, 3, 0, 4, 8),
        (15, 4, 3, 1, -6, 12),
        # Citizens in Grand Nature Park (14, 1) and Plaza (16, 4)
        (14, 1, 0, 1, -8, 6),
        (15, 1, 2, 0, 12, 10),
        (16, 4, 1, 1, -4, 14),
        # Church Grounds (16, 1)
        (16, 1, 1, 0, 8, 12),
    ]

    # PASS 2: Render All Rectangular Buildings, Vehicles, and Pedestrians in Depth Order (i + j)
    for depth in range(40):
        # 1. Draw vehicles at this depth
        for (vx, vy, vtype, vdir, vprog, vshift) in active_vehicles:
            if vx + vy == depth:
                v_key = f"car_{vtype}_{vdir}"
                if v_key in scaled:
                    v_sp = scaled[v_key]
                    # Start tile center and target tile center
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
        for (px_tile, py_tile, ctype, cframe, ox, oy) in active_pedestrians:
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
            if 0 <= i < 20 and 0 <= j < 20:
                b_key = buildings[i][j]
                if b_key is not None and b_key in scaled:
                    b_sp = scaled[b_key]
                    px = 32 * i + 50
                    py = 32 * j + 16 * i + 50
                    draw_y = py + 80 - b_sp.height
                    canvas.alpha_composite(b_sp, (px, draw_y))

    # PASS 3: Dynamic Building Animations (Windmill, Chimney Smoke, Park Fountain, Aviation Beacons)
    # 1. Windmill Sails on farm_v2 at (4, 7)
    if "windmill_blades" in scaled:
        w_sails = scaled["windmill_blades"]
        # Rotate sails by 35 degrees
        rotated_sails = w_sails.rotate(35, resample=Image.BICUBIC)
        # Hub location on farm_v2
        farm_x = 32 * 4 + 50
        farm_y = 32 * 7 + 16 * 4 + 50 + 80 - scaled["farm_v2"].height
        hub_x = farm_x + 36 - rotated_sails.width // 2
        hub_y = farm_y + 16 - rotated_sails.height // 2
        canvas.alpha_composite(rotated_sails, (hub_x, hub_y))

    # 2. Billowing Industrial Smoke Puffs on Factory Smokestacks
    smoke_sources = [
        (6, 7, "ind_t1_v1", [(12, -4, 0), (16, -18, 1), (22, -34, 2), (28, -50, 3)]),
        (10, 7, "ind_t2_v1", [(50, 2, 0), (54, -14, 1), (60, -30, 2), (66, -46, 3)]),
        (14, 7, "ind_t3_v1", [(56, 12, 0), (60, -2, 1), (64, -16, 2)]),
    ]
    for (ix, iy, ikey, puffs) in smoke_sources:
        fac_x = 32 * ix + 50
        fac_y = 32 * iy + 16 * ix + 50 + 80 - scaled[ikey].height
        for (off_x, off_y, frame) in puffs:
            p_key = f"smoke_puff_{frame}"
            if p_key in scaled:
                smk = scaled[p_key]
                canvas.alpha_composite(smk, (fac_x + off_x, fac_y + off_y))

    # 3. Sparkling Park Fountain Water in Grand Nature Park (14, 1) and Plaza Park (16, 4)
    if "fountain_1" in scaled:
        fnt = scaled["fountain_1"]
        for (fx, fy) in [(14, 1), (16, 4)]:
            park_x = 32 * fx + 50
            park_y = 32 * fy + 16 * fx + 50 + 80 - scaled["park"].height
            canvas.alpha_composite(fnt, (park_x + 48 - fnt.width // 2, park_y + 26 - fnt.height // 2))

    # 4. Skyscraper Rooftop Red Warning Beacons
    if "beacon_light" in scaled:
        bcn = scaled["beacon_light"]
        beacons = [
            (10, 1, "res_t3_v1", 48, -12),
            (12, 1, "res_t3_v2", 48, -18),
            (10, 4, "com_t3_v1", 46, -24),
            (12, 4, "com_t3_v2", 46, -30),
        ]
        for (bx, by, bkey, off_x, off_y) in beacons:
            b_x = 32 * bx + 50
            b_y = 32 * by + 16 * bx + 50 + 80 - scaled[bkey].height
            canvas.alpha_composite(bcn, (b_x + off_x - bcn.width // 2, b_y + off_y - bcn.height // 2))

    # 5. Harbour Crane Amber Warning Light
    if "harbour_crane_light" in scaled:
        h_light = scaled["harbour_crane_light"]
        for hx in [4, 6, 8, 10, 12, 14]:
            harb_x = 32 * hx + 50
            harb_y = 32 * 10 + 16 * hx + 50 + 80 - scaled["harbour"].height
            canvas.alpha_composite(h_light, (harb_x + 42 - h_light.width // 2, harb_y - 8))

    canvas.save("tools/current_world.png")
    print("Successfully rendered animated TheoTown city to tools/current_world.png!")

if __name__ == "__main__":
    render_preview()
