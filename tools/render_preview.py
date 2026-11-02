#!/usr/bin/env python3
"""
Renders a visual snapshot of the TheoTown city simulation world:
- Pass 1: All 1x1 ground diamond tiles (grass, water, roads with authentic autotiling, zone bases)
- Pass 2: All 2x2 flat-bottomed rectangular 3D buildings arranged SIDE BY SIDE
- Zero weird diamond-point wedges, zero harbour-on-road overlaps, full rectangular sprites
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

    # 5. Row 4 (y = 10): Shoreline Harbours & Water — ALL HARBOURS SIDE BY SIDE!
    for x in range(3, 20):
        for y in range(12, 20):
            ground[x][y] = "water"

    # Harbours sit directly on shoreline (entrance at y=10 touches road at y=9, piers at y=11 touch water at y=12)
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

    # PASS 2: Render All Rectangular Buildings in Depth Order (i + j)
    for depth in range(40):
        for i in range(depth + 1):
            j = depth - i
            if 0 <= i < 20 and 0 <= j < 20:
                b_key = buildings[i][j]
                if b_key is not None and b_key in scaled:
                    b_sp = scaled[b_key]
                    px = 32 * i + 50
                    py = 32 * j + 16 * i + 50
                    # Rectangular flat base aligns with py + 80 (bottom of 2x2 lot)
                    draw_y = py + 80 - b_sp.height
                    canvas.alpha_composite(b_sp, (px, draw_y))

    canvas.save("tools/current_world.png")
    print("Successfully rendered side-by-side rectangular TheoTown city to tools/current_world.png!")

if __name__ == "__main__":
    render_preview()
