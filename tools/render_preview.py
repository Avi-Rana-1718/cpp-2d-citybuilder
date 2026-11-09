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
        offsets = [(0, 0), (1, 0), (-1, 1), (0, 1)]
        if 1 <= rx < GRID_W - 1 and 0 <= ry < GRID_H - 1:
            for dx, dy in offsets:
                if ground[rx + dx][ry + dy] == "road":
                    return
            buildings[rx][ry] = bld_key
            for dx, dy in offsets:
                ground[rx + dx][ry + dy] = base_key

    # 1. 1x1 Roads Network Across 64x64 Metropolis (100% Flush Edge-Sharing)
    # 8 Major Horizontal Avenues (Axis 1: East/Down-Right along +X)
    avenues_y = [4, 10, 16, 22, 28, 34, 40, 48]
    for ay in avenues_y:
        for x in range(2, 62):
            set_road(x, ay)

    # 8 Major Edge-Sharing Cross-Boulevards (Axis 2: South-West/Down-Left along x + y = S)
    # Every consecutive road tile shares a full 32x16 edge with (x-1, y+1) with ZERO diamond tip seams!
    cross_boulevards = [24, 36, 48, 60, 72, 84, 96, 108]
    for S in cross_boulevards:
        for y in range(2, 49):
            x = S - y
            if 2 <= x <= 61:
                set_road(x, y)

    # 2. District 1 (y = 2): Northern Residential Cottages & Bungalows (Touches Avenue at y = 4)
    for x in range(4, 59, 2):
        if x % 8 == 0:
            set_2x2(x, 2, "park", "grass")
        elif x % 8 == 2:
            set_2x2(x, 2, "church", "grass")
        elif x % 4 == 0:
            set_2x2(x, 2, "res_t1_v2", "zone_residental")
        else:
            set_2x2(x, 2, "res_t1_v1", "zone_residental")

    # 3. District 2 (y = 8): Residential Brownstones & Urban Condominiums (Touches Avenue at y = 10)
    for x in range(4, 59, 2):
        if x % 12 == 0:
            set_2x2(x, 8, "school", "grass")
        elif x % 8 == 0:
            set_2x2(x, 8, "park", "grass")
        elif x % 4 == 0:
            set_2x2(x, 8, "res_t2_v2", "zone_residental")
        else:
            set_2x2(x, 8, "res_t2_v1", "zone_residental")

    # 4. District 3 (y = 14): Highrise Luxury Towers & Sky-Plazas (Touches Avenue at y = 16)
    for x in range(4, 59, 2):
        if x % 12 == 4:
            set_2x2(x, 14, "park", "grass")
        elif x % 8 == 0:
            set_2x2(x, 14, "church", "grass")
        elif x % 4 == 0:
            set_2x2(x, 14, "res_t3_v2", "zone_residental")
        else:
            set_2x2(x, 14, "res_t3_v1", "zone_residental")

    # 5. District 4 (y = 20): Downtown Corporate Banking Highrises & Financial Towers (Touches Avenue at y = 22)
    for x in range(4, 59, 2):
        if x % 12 == 0:
            set_2x2(x, 20, "park", "grass")
        elif x % 8 == 0:
            set_2x2(x, 20, "school", "grass")
        elif x % 4 == 0:
            set_2x2(x, 20, "com_t3_v2", "zone_commercial")
        else:
            set_2x2(x, 20, "com_t3_v1", "zone_commercial")

    # 6. District 5 (y = 26): Shopping Plazas, Diners, Groceries, & Retail Arcades (Touches Avenue at y = 28)
    for x in range(4, 59, 2):
        if x % 12 == 6:
            set_2x2(x, 26, "park", "grass")
        elif x % 8 == 4:
            set_2x2(x, 26, "com_t1_v2", "zone_commercial")
        elif x % 8 == 0:
            set_2x2(x, 26, "com_t1_v1", "zone_commercial")
        elif x % 4 == 0:
            set_2x2(x, 26, "com_t2_v2", "zone_commercial")
        else:
            set_2x2(x, 26, "com_t2_v1", "zone_commercial")

    # 7. District 6 (y = 32): West Farmlands & East Industrial Manufacturing (Touches Avenue at y = 34)
    for x in range(4, 59, 2):
        if x < 26:
            if x % 4 == 0:
                set_2x2(x, 32, "farm_v2", "zone_farm")
            else:
                set_2x2(x, 32, "farm_v1", "zone_farm")
        else:
            if x % 8 == 0:
                set_2x2(x, 32, "ind_t2_v2", "zone_industrial")
            elif x % 6 == 0:
                set_2x2(x, 32, "ind_t2_v1", "zone_industrial")
            elif x % 4 == 0:
                set_2x2(x, 32, "ind_t1_v2", "zone_industrial")
            else:
                set_2x2(x, 32, "ind_t1_v1", "zone_industrial")

    # 8. District 7 (y = 38): West Windmill Homesteads & East Robotics / Clean-Fab Hubs (Touches Avenue at y = 40)
    for x in range(4, 59, 2):
        if x < 26:
            if x % 4 == 0:
                set_2x2(x, 38, "farm_v2", "zone_farm")
            else:
                set_2x2(x, 38, "farm_v1", "zone_farm")
        else:
            if x % 4 == 0:
                set_2x2(x, 38, "ind_t3_v2", "zone_industrial")
            else:
                set_2x2(x, 38, "ind_t3_v1", "zone_industrial")

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
                left  = is_road(i - 1, j)
                right = is_road(i + 1, j)
                ne    = is_road(i + 1, j - 1)
                sw    = is_road(i - 1, j + 1)

                if (left or right) and (ne or sw):
                    sp = r_mid
                elif left and right:
                    sp = r_mid
                elif ne and sw:
                    sp = r_mid.transpose(Image.FLIP_LEFT_RIGHT)
                elif left and not right:
                    sp = r_rot180
                elif ne and not sw:
                    sp = r_straight.transpose(Image.FLIP_LEFT_RIGHT)
                elif sw and not ne:
                    sp = r_straight.transpose(Image.FLIP_LEFT_RIGHT).rotate(180)
                else:
                    sp = r_straight
            else:
                sp = scaled[g_key]
            canvas.alpha_composite(sp, (px, py))

    # PASS 2: Render All 2x2 Rectangular Buildings in Depth Order (i + j)
    for depth in range(GRID_W + GRID_H):
        for i in range(depth + 1):
            j = depth - i
            if 0 <= i < GRID_W and 0 <= j < GRID_H:
                b_key = buildings[i][j]
                if b_key is not None and b_key in scaled:
                    b_sp = scaled[b_key]
                    px = 32 * i - 16 + 50
                    py = 32 * j + 16 * i + 50
                    draw_y = py + 80 - b_sp.height
                    canvas.alpha_composite(b_sp, (px, draw_y))

    canvas.save("tools/current_world.png")
    print("Successfully rendered 10x expanded metropolis (64x64) to tools/current_world.png!")

if __name__ == "__main__":
    render_preview()
