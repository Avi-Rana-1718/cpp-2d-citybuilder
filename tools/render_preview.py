#!/usr/bin/env python3
"""
Renders a visual snapshot of the TheoTown city simulation world:
- Pass 1: All 1x1 ground diamond tiles (grass, water, roads with authentic autotiling, zone bases)
- Pass 2: All 2x2 3D isometric buildings in (x + y) depth order
- Zero diagonal road artifacts, zero harbour-on-road overlaps, full individual building placement
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
    
    r_mid = scaled["road-mid"]
    r_mid_flip = r_mid.transpose(Image.FLIP_LEFT_RIGHT)
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

    # 1. 1x1 Roads Network
    # North Boulevard (y = 3)
    for x in range(1, 19): set_road(x, 3)
    # Central Boulevard (y = 8)
    for x in range(1, 19): set_road(x, 8)
    # South Shore Road (y = 13)
    for x in range(1, 19): set_road(x, 13)
    # West Avenue Links
    for y in range(4, 8): set_road(1, y)
    for y in range(9, 13): set_road(1, y)
    # East Avenue Links
    for y in range(4, 8): set_road(18, y)
    for y in range(9, 13): set_road(18, y)
    # Central Link
    for y in range(4, 8): set_road(8, y)

    # 2. 2x2 Residential Buildings (NW quadrant)
    set_2x2(2, 1, "res_t3_v1", "zone_residental") # T3 Highrise Luxury Tower
    set_2x2(5, 1, "res_t2_v2", "zone_residental") # T2 Urban Condominiums
    set_2x2(2, 4, "res_t1_v1", "zone_residental") # T1 Suburban Cottages
    set_2x2(5, 4, "res_t2_v1", "zone_residental") # T2 Brick Apartments

    # 3. 2x2 Civics Hub
    set_2x2(2, 6, "school", "grass")             # Elementary School Campus
    set_2x2(5, 6, "park",   "grass")             # Grand Nature Park & Fountain
    set_2x2(10, 6, "church", "grass")            # Gothic Cathedral
    set_2x2(13, 6, "school", "grass")            # Elementary School Campus
    set_2x2(16, 4, "park",   "grass")            # Downtown Plaza Park

    # 4. 2x2 Commercial Downtown (NE quadrant)
    set_2x2(10, 1, "com_t3_v1", "zone_commercial") # T3 Corporate Banking Tower
    set_2x2(13, 1, "com_t2_v2", "zone_commercial") # T2 Department Store
    set_2x2(10, 4, "com_t1_v1", "zone_commercial") # T1 Corner Grocery
    set_2x2(13, 4, "com_t2_v1", "zone_commercial") # T2 Shopping Plaza

    # 5. 2x2 Agricultural Farmlands (SW quadrant)
    set_2x2(2, 9,  "farm_v1", "zone_farm")       # Barn & Wheat Silo
    set_2x2(5, 9,  "farm_v2", "zone_farm")       # Windmill Homestead
    set_2x2(2, 11, "farm_v1", "zone_farm")       # Barn & Silo
    set_2x2(5, 11, "farm_v2", "zone_farm")       # Windmill Homestead

    # 6. 2x2 Industrial District (SE quadrant)
    set_2x2(10, 9,  "ind_t1_v1", "zone_industrial") # T1 Scrap Workshop
    set_2x2(13, 9,  "ind_t2_v1", "zone_industrial") # T2 Manufacturing Plant
    set_2x2(16, 9,  "ind_t3_v1", "zone_industrial") # T3 Microchip Clean-Fab
    set_2x2(10, 11, "ind_t2_v2", "zone_industrial") # T2 Heavy Assembly
    set_2x2(13, 11, "ind_t3_v1", "zone_industrial") # T3 Clean-Fab Tech

    # 7. Water Basin & Harbours (South shoreline)
    for x in range(5, 20):
        for y in range(16, 20):
            ground[x][y] = "water"

    # Harbours placed cleanly on shoreline (entrance at y=14 touches road at y=13, pier at y=15 touches water at y=16)
    set_2x2(8, 14,  "harbour", "grass")          # Trade Harbour
    set_2x2(12, 14, "harbour", "grass")          # Trade Harbour

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
                ne = is_road(i + 1, j - 1)
                sw = is_road(i - 1, j + 1)
                up = is_road(i, j - 1)
                down = is_road(i, j + 1)

                if (left or right) and (ne or sw):
                    sp = r_mid.copy()
                    sp.alpha_composite(r_mid_flip)
                elif left and right:
                    sp = r_mid
                elif ne and sw:
                    sp = r_mid_flip
                elif up and down:
                    sp = r_mid
                elif left and not right:
                    sp = r_rot180
                elif right and not left:
                    sp = r_straight
                elif ne and not sw:
                    sp = r_straight.transpose(Image.FLIP_LEFT_RIGHT)
                elif sw and not ne:
                    sp = r_rot180.transpose(Image.FLIP_LEFT_RIGHT)
                else:
                    sp = r_straight
            else:
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
