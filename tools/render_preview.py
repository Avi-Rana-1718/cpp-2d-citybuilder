#!/usr/bin/env python3
"""
Renders a visual snapshot of the TheoTown city simulation world:
- Pass 1: All 1x1 ground diamond tiles (grass, water, roads, zone bases)
- Pass 2: All 2x2 3D isometric buildings in (x + y) depth order
"""
import os
from PIL import Image

def render_preview():
    sprites_dir = "assets/sprites"
    sprites = {}
    for f in os.listdir(sprites_dir):
        if f.endswith(".png"):
            sprites[f[:-4]] = Image.open(os.path.join(sprites_dir, f)).convert("RGBA")
    
    scaled = {}
    for k, v in sprites.items():
        scaled[k] = v.resize((v.width * 2, v.height * 2), Image.NEAREST)
    
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

    # 1. 1x1 Roads Network
    for x in range(20): set_road(x, 8)
    for y in range(20): set_road(9, y)
    for x in range(1, 9): set_road(x, 3)
    for y in range(3, 9): set_road(1, y)
    for x in range(10, 19): set_road(x, 3)
    for y in range(3, 9): set_road(18, y)
    for x in range(1, 9): set_road(x, 15)
    for y in range(8, 16): set_road(1, y)
    for x in range(10, 19): set_road(x, 15)
    for y in range(8, 16): set_road(18, y)

    # 2. 2x2 Residential Buildings
    set_2x2(3, 4, "res_t1_v1", "zone_residental") # T1 Suburban Cottages
    set_2x2(6, 4, "res_t2_v1", "zone_residental") # T2 Brick Apartments
    set_2x2(3, 1, "res_t3_v1", "zone_residental") # T3 Highrise Luxury Tower
    set_2x2(6, 1, "res_t2_v2", "zone_residental") # T2 Urban Condominiums

    # 3. 2x2 Civics Hub
    set_2x2(7, 6, "school", "grass")
    set_2x2(2, 6, "church", "grass")
    set_2x2(4, 6, "park",   "grass")

    # 4. 2x2 Commercial Downtown
    set_2x2(10, 4, "com_t1_v1", "zone_commercial") # T1 Corner Grocery
    set_2x2(13, 4, "com_t2_v1", "zone_commercial") # T2 Shopping Plaza
    set_2x2(10, 1, "com_t3_v1", "zone_commercial") # T3 Corporate Banking Tower
    set_2x2(13, 1, "com_t2_v2", "zone_commercial") # T2 Department Store
    set_2x2(15, 6, "park",      "grass")

    # 5. 2x2 Agricultural Farmlands
    set_2x2(3, 9,  "farm_v1", "zone_farm") # Barn & Silo
    set_2x2(6, 9,  "farm_v2", "zone_farm") # Windmill Homestead
    set_2x2(3, 12, "farm_v1", "zone_farm")
    set_2x2(6, 12, "farm_v2", "zone_farm")

    # 6. 2x2 Industrial District
    set_2x2(10, 9,  "ind_t1_v1", "zone_industrial") # T1 Scrap Workshop
    set_2x2(13, 9,  "ind_t2_v1", "zone_industrial") # T2 Manufacturing Plant
    set_2x2(10, 12, "ind_t3_v1", "zone_industrial") # T3 Microchip Clean-Fab
    set_2x2(13, 12, "ind_t2_v2", "zone_industrial") # T2 Machine Assembly

    # 7. Water Basin & Harbours
    for x in range(7, 17):
        for y in range(17, 20):
            ground[x][y] = "water"
    set_2x2(8, 15,  "harbour", "grass")
    set_2x2(12, 15, "harbour", "grass")

    # PASS 1: Render All Ground Tiles
    for i in range(20):
        for j in range(20):
            px = 32 * i + 50
            py = 32 * j + 16 * i + 50
            g_key = ground[i][j]
            sp = scaled[g_key]
            canvas.alpha_composite(sp, (px, py))

    # PASS 2: Render All 3D Buildings in Depth Order (i + j)
    for depth in range(40):
        for i in range(depth + 1):
            j = depth - i
            if 0 <= i < 20 and 0 <= j < 20:
                b_key = buildings[i][j]
                if b_key is not None and b_key in scaled:
                    sp = scaled[b_key]
                    px = 32 * i + 50
                    py = 32 * j + 16 * i + 50
                    # 2x2 ground diamond is 80px tall when scaled by 2 (40 unscaled)
                    offset_y = sp.height - 80 if sp.height > 80 else 0
                    draw_y = py - offset_y
                    canvas.alpha_composite(sp, (px, draw_y))

    canvas.save("tools/current_world.png")
    print("Successfully rendered 2x2 TheoTown city to tools/current_world.png!")

if __name__ == "__main__":
    render_preview()
