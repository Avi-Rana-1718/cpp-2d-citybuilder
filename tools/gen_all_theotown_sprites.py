#!/usr/bin/env python3
"""
TheoTown-style 2:1 Isometric Sprite Generator
Generates:
1. Ground diamond tiles (32x16 unscaled -> 64x32 scaled 2x):
   - grass, water, road, road-mid
   - zone_residental, zone_commercial, zone_industrial, zone_farm
2. 2x2 Multi-Tile Buildings (48xH unscaled -> 96x(2H) scaled 2x):
   - Residential: Tier 1 (v1, v2), Tier 2 (v1, v2), Tier 3 (v1, v2)
   - Commercial: Tier 1 (v1, v2), Tier 2 (v1, v2), Tier 3 (v1, v2)
   - Industrial: Tier 1 (v1, v2), Tier 2 (v1, v2), Tier 3 (v1, v2)
   - Farm: v1 (Barn & Silo), v2 (Windmill Homestead)
   - Civics: School, Church, Park, Harbour
"""
import os
from PIL import Image

SPRITES_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")
os.makedirs(SPRITES_DIR, exist_ok=True)

# -------------------------------------------------------------
# 1. 1x1 Ground Diamond Tiles (32x16)
# -------------------------------------------------------------
def make_grass():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            v = ((x * 7 + y * 13) % 23) - 11
            img.putpixel((x, y), (55 + v // 2, 160 + v, 48 + v // 3, 255))
    return img

def make_water():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            wave = ((x + y * 2) % 6 == 0)
            if wave:
                img.putpixel((x, y), (65, 170, 235, 255))
            else:
                d = ((x * 3 + y * 5) % 15)
                img.putpixel((x, y), (30 + d // 2, 95 + d, 185 + d, 255))
    return img

def make_road():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            noise = ((x * 11 + y * 17) % 19) - 9
            img.putpixel((x, y), (145 + noise, 130 + noise, 110 + noise, 255))
    # Dashed road center line
    for (x, y) in [(15, 7), (16, 7), (15, 8), (16, 8)]:
        img.putpixel((x, y), (230, 225, 210, 255))
    return img

def make_road_mid():
    img = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        for x in range(16 - hw, 16 + hw):
            noise = ((x * 13 + y * 11) % 17) - 8
            img.putpixel((x, y), (150 + noise, 135 + noise, 115 + noise, 255))
    # Intersection markings
    for (x, y) in [(14, 7), (15, 7), (16, 8), (17, 8)]:
        img.putpixel((x, y), (235, 230, 215, 255))
    return img

def make_zone_base(color_tint, border_color):
    """Draws a TheoTown style zone tile: grass meadow with colored dashed boundary markers."""
    img = make_grass()
    for y in range(16):
        hw = (y + 1) * 2 if y < 8 else (16 - y) * 2
        x_left = 16 - hw
        x_right = 16 + hw - 1
        # Perimeter dashed border
        if (x_left + y) % 4 < 2:
            img.putpixel((x_left, y), border_color)
            img.putpixel((x_right, y), border_color)
        # Subtle translucent colored tint over grass
        for x in range(x_left + 1, x_right):
            curr = img.getpixel((x, y))
            r = int(curr[0] * 0.75 + color_tint[0] * 0.25)
            g = int(curr[1] * 0.75 + color_tint[1] * 0.25)
            b = int(curr[2] * 0.75 + color_tint[2] * 0.25)
            img.putpixel((x, y), (r, g, b, 255))
    return img

# -------------------------------------------------------------
# 2. 2:1 Isometric Helpers (Pure 2:1 Geometry, ZERO Rectangles)
# -------------------------------------------------------------
def draw_iso_diamond(img, cx, cy, hw, hh, color):
    """
    Draws a 2:1 isometric diamond centered at (cx, cy).
    hw: half width, hh: half height (strictly hh = hw // 2).
    Every pixel follows 2:1 isometric slope. ZERO flat or vertical edges.
    """
    for y in range(cy - hh, cy + hh + 1):
        for x in range(cx - hw, cx + hw + 1):
            dx = abs(x - cx)
            dy = abs(y - cy)
            if (dx / float(hw)) + (dy / float(hh)) <= 1.001:
                if 0 <= x < img.width and 0 <= y < img.height:
                    img.putpixel((x, y), color)

def draw_iso_box(img, cx, cy, hw, hh, wall_h, top_col, left_col, right_col):
    """
    Draws a 3D isometric building block at center (cx, cy) on ground.
    hw: half width, hh: half height (hh = hw // 2).
    wall_h: height of walls in pixels.
    All roof and ground edges strictly follow authentic 2:1 slope.
    """
    # 1. Left Wall (facing SW)
    for x in range(cx - hw, cx + 1):
        rel_x = x - (cx - hw)
        y_bot = cy + rel_x * hh // hw
        y_top = y_bot - wall_h
        for y in range(y_top, y_bot + 1):
            if 0 <= x < img.width and 0 <= y < img.height:
                c = left_col if (x + y) % 2 == 0 else (max(0, left_col[0] - 12), max(0, left_col[1] - 12), max(0, left_col[2] - 12), 255)
                img.putpixel((x, y), c)

    # 2. Right Wall (facing SE)
    for x in range(cx, cx + hw + 1):
        rel_x = x - cx
        y_bot = cy + hh - rel_x * hh // hw
        y_top = y_bot - wall_h
        for y in range(y_top, y_bot + 1):
            if 0 <= x < img.width and 0 <= y < img.height:
                c = right_col if (x + y) % 2 == 0 else (max(0, right_col[0] - 12), max(0, right_col[1] - 12), max(0, right_col[2] - 12), 255)
                img.putpixel((x, y), c)

    # 3. Top Roof Diamond
    roof_cy = cy - wall_h
    for y in range(roof_cy - hh, roof_cy + hh + 1):
        for x in range(cx - hw, cx + hw + 1):
            dx = abs(x - cx)
            dy = abs(y - roof_cy)
            if (dx / float(hw)) + (dy / float(hh)) <= 1.001:
                if 0 <= x < img.width and 0 <= y < img.height:
                    c = top_col if (x + y * 2) % 3 == 0 else (max(0, top_col[0] - 15), max(0, top_col[1] - 15), max(0, top_col[2] - 15), 255)
                    img.putpixel((x, y), c)

# -------------------------------------------------------------
# 3. Residential 2x2 Tiers & Variants
# -------------------------------------------------------------
def make_res_t1_v1():
    # 48x48: Suburban Cottages (pair of gabled houses with picket fence)
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    base_y = 8

    # House 1 (North/Back Cottage: cx=18, cy=base_y+16, hw=10, hh=5, wall_h=8)
    draw_iso_box(img, 18, base_y + 16, 10, 5, 8, (215, 75, 60, 255), (230, 220, 205, 255), (180, 170, 155, 255))
    # Chimney 1
    for cy in range(base_y + 3, base_y + 10):
        img.putpixel((22, cy), (150, 60, 50, 255))
    img.putpixel((22, base_y + 2), (235, 235, 240, 220)) # Smoke puff

    # House 2 (South/Front Cottage: cx=32, cy=base_y+26, hw=10, hh=5, wall_h=8)
    draw_iso_box(img, 32, base_y + 26, 10, 5, 8, (70, 110, 160, 255), (225, 215, 200, 255), (175, 165, 150, 255))
    # Chimney 2
    for cy in range(base_y + 13, base_y + 20):
        img.putpixel((36, cy), (150, 60, 50, 255))
    img.putpixel((36, base_y + 12), (235, 235, 240, 220))

    # Front pathway & wooden front doors
    for (dx, dy) in [(14, base_y + 15), (14, base_y + 16), (28, base_y + 25), (28, base_y + 26)]:
        img.putpixel((dx, dy), (120, 70, 35, 255))
    return img

def make_res_t1_v2():
    # 48x48: Spacious Bungalow with wraparound porch and stone chimney
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    base_y = 8

    # Main bungalow block (cx=24, cy=base_y+20, hw=15, hh=8, wall_h=9)
    draw_iso_box(img, 24, base_y + 20, 15, 8, 9, (190, 85, 55, 255), (235, 225, 210, 255), (185, 175, 160, 255))
    # Stone Chimney
    for cx in [16, 17]:
        for cy in range(base_y + 4, base_y + 14):
            img.putpixel((cx, cy), (130, 130, 135, 255))
    img.putpixel((16, base_y + 3), (240, 240, 245, 220))

    # Front glass windows
    for (wx, wy) in [(20, base_y + 18), (28, base_y + 22)]:
        img.putpixel((wx, wy), (180, 230, 255, 255))
    return img

def make_res_t2_v1():
    # 48x54: Brick Brownstone Apartments (3 stories, windows, rooftop AC)
    img = Image.new("RGBA", (48, 54), (0, 0, 0, 0))
    base_y = 14
    draw_iso_diamond(img, 24, base_y + 20, 17, 8, (80, 80, 85, 255)) # Paved sidewalk diamond

    # 3-Story Brick Complex (cx=24, cy=base_y+20, hw=16, hh=8, wall_h=18)
    draw_iso_box(img, 24, base_y + 20, 16, 8, 18, (80, 85, 90, 255), (175, 75, 60, 255), (135, 55, 45, 255))

    # Rooftop Air Conditioner & Access Hatch
    draw_iso_box(img, 20, base_y + 2, 4, 2, 3, (160, 165, 170, 255), (130, 135, 140, 255), (100, 105, 110, 255))
    draw_iso_box(img, 28, base_y + 4, 3, 2, 2, (150, 155, 160, 255), (120, 125, 130, 255), (90, 95, 100, 255))

    # Rows of Apartment Windows (illuminated warm white)
    for floor in range(3):
        fy = base_y + 7 + floor * 5
        for x_rel in [12, 16, 20]:
            img.putpixel((x_rel, fy), (255, 245, 180, 255))
        for x_rel in [28, 32, 36]:
            img.putpixel((x_rel, fy + 2), (240, 230, 160, 255))
    return img

def make_res_t2_v2():
    # 48x54: Modern Townhouse Courtyard Complex
    img = Image.new("RGBA", (48, 54), (0, 0, 0, 0))
    base_y = 14

    # Complex Building (cx=24, cy=base_y+20, hw=15, hh=8, wall_h=16)
    draw_iso_box(img, 24, base_y + 20, 15, 8, 16, (60, 90, 120, 255), (220, 215, 200, 255), (170, 165, 150, 255))

    # Balconies & Terraces
    for (bx, by) in [(14, base_y + 11), (18, base_y + 13), (30, base_y + 15)]:
        img.putpixel((bx, by), (80, 80, 85, 255))
        img.putpixel((bx, by - 1), (190, 230, 255, 255))
    return img

def make_res_t3_v1():
    # 48x68: Modern Highrise Luxury Tower (Skyscraper with glass terraces & rooftop pool)
    img = Image.new("RGBA", (48, 68), (0, 0, 0, 0))
    base_y = 28
    draw_iso_diamond(img, 24, base_y + 20, 16, 8, (85, 90, 95, 255)) # Granite plaza diamond

    # Skyscraper Tower (cx=24, cy=base_y+20, hw=14, hh=7, wall_h=34)
    draw_iso_box(img, 24, base_y + 20, 14, 7, 34, (55, 130, 185, 255), (120, 190, 235, 255), (80, 150, 200, 255))

    # Rooftop Penthouse & Pool
    draw_iso_box(img, 24, base_y - 14, 6, 3, 5, (60, 190, 240, 255), (220, 225, 230, 255), (180, 185, 190, 255))
    img.putpixel((24, base_y - 19), (255, 215, 0, 255)) # Beacon light

    # Vertical curtain-wall glass grid stripes
    for fy in range(base_y - 10, base_y + 15, 4):
        for gx in [15, 19, 23]:
            img.putpixel((gx, fy), (230, 245, 255, 255))
        for gx in [25, 29, 33]:
            img.putpixel((gx, fy + 2), (200, 230, 250, 255))
    return img

def make_res_t3_v2():
    # 48x68: Twin Highrise Plaza (Twin glass towers with skybridge)
    img = Image.new("RGBA", (48, 68), (0, 0, 0, 0))
    base_y = 28
    draw_iso_diamond(img, 24, base_y + 20, 16, 8, (80, 85, 90, 255))

    # Tower 1 (West: cx=17, cy=base_y+16, hw=8, hh=4, wall_h=32)
    draw_iso_box(img, 17, base_y + 16, 8, 4, 32, (45, 110, 165, 255), (110, 175, 220, 255), (70, 135, 180, 255))
    # Tower 2 (East: cx=31, cy=base_y+23, hw=8, hh=4, wall_h=30)
    draw_iso_box(img, 31, base_y + 23, 8, 4, 30, (45, 110, 165, 255), (110, 175, 220, 255), (70, 135, 180, 255))

    # Skybridge connecting the towers at mid-height
    for sbx in range(21, 28):
        for sby in range(base_y - 2, base_y + 1):
            img.putpixel((sbx, sby), (210, 220, 230, 255))
    return img

# -------------------------------------------------------------
# 4. Commercial 2x2 Tiers & Variants
# -------------------------------------------------------------
def make_com_t1_v1():
    # 48x48: Corner Grocery Store & Bakery with striped awnings
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    base_y = 8

    # Commercial Building (cx=24, cy=base_y+20, hw=15, hh=8, wall_h=12)
    draw_iso_box(img, 24, base_y + 20, 15, 8, 12, (200, 160, 80, 255), (230, 220, 180, 255), (180, 170, 140, 255))

    # Red/White Striped Awnings
    for ax in range(12, 23):
        ay = base_y + 14 + (ax - 12) * 5 // 10
        c = (220, 45, 45, 255) if ax % 3 == 0 else (245, 245, 250, 255)
        img.putpixel((ax, ay), c)
    # Illuminated Storefront Windows
    for wx in range(13, 22):
        wy = base_y + 16 + (wx - 13) * 5 // 10
        img.putpixel((wx, wy), (255, 240, 140, 255))
    return img

def make_com_t1_v2():
    # 48x48: Local Diner & Mini Mart with neon billboard
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    base_y = 8

    # Diner Block (cx=24, cy=base_y+20, hw=14, hh=7, wall_h=11)
    draw_iso_box(img, 24, base_y + 20, 14, 7, 11, (60, 150, 170, 255), (235, 230, 220, 255), (185, 180, 170, 255))

    # Neon Signboard on Roof
    for sx in range(19, 29):
        img.putpixel((sx, base_y + 4), (255, 70, 120, 255))
        img.putpixel((sx, base_y + 5), (255, 210, 50, 255))
    return img

def make_com_t2_v1():
    # 48x56: Two-Story Shopping Plaza with glass atrium
    img = Image.new("RGBA", (48, 56), (0, 0, 0, 0))
    base_y = 16
    draw_iso_diamond(img, 24, base_y + 20, 17, 8, (95, 95, 100, 255))

    # Shopping Plaza Complex (cx=24, cy=base_y+20, hw=16, hh=8, wall_h=20)
    draw_iso_box(img, 24, base_y + 20, 16, 8, 20, (50, 115, 140, 255), (210, 215, 225, 255), (160, 165, 175, 255))

    # Center Glass Atrium & Skylight (cyan glass)
    draw_iso_box(img, 24, base_y - 2, 7, 4, 4, (120, 220, 255, 255), (100, 190, 230, 255), (70, 150, 190, 255))

    # Neon Brand Signs & Double Glass Doors
    for dx in range(16, 21):
        dy = base_y + 16 + (dx - 16) * 5 // 5
        img.putpixel((dx, dy), (200, 240, 255, 255))
    return img

def make_com_t2_v2():
    # 48x56: Department Store with classic stone facade & clock
    img = Image.new("RGBA", (48, 56), (0, 0, 0, 0))
    base_y = 16
    draw_iso_diamond(img, 24, base_y + 20, 17, 8, (90, 95, 100, 255))

    draw_iso_box(img, 24, base_y + 20, 16, 8, 20, (140, 70, 60, 255), (225, 215, 195, 255), (175, 165, 150, 255))

    # Central Clock Medallion
    img.putpixel((24, base_y + 4), (250, 220, 50, 255))
    img.putpixel((24, base_y + 3), (250, 220, 50, 255))
    return img

def make_com_t3_v1():
    # 48x70: Corporate Banking Skyscraper with reflective glass & spire
    img = Image.new("RGBA", (48, 70), (0, 0, 0, 0))
    base_y = 30
    draw_iso_diamond(img, 24, base_y + 20, 16, 8, (80, 85, 90, 255))

    # Main Corporate Highrise (cx=24, cy=base_y+20, hw=15, hh=8, wall_h=38)
    draw_iso_box(img, 24, base_y + 20, 15, 8, 38, (40, 100, 160, 255), (100, 180, 240, 255), (60, 130, 190, 255))

    # Stepped Crown & Spire Antenna
    draw_iso_box(img, 24, base_y - 20, 8, 4, 6, (30, 80, 140, 255), (140, 210, 255, 255), (90, 160, 210, 255))
    for sy in range(base_y - 32, base_y - 25):
        img.putpixel((24, sy), (230, 230, 240, 255))
    img.putpixel((24, base_y - 33), (255, 40, 40, 255)) # Red air safety beacon
    return img

def make_com_t3_v2():
    # 48x70: International Financial Center with helipad
    img = Image.new("RGBA", (48, 70), (0, 0, 0, 0))
    base_y = 30
    draw_iso_diamond(img, 24, base_y + 20, 16, 8, (85, 90, 95, 255))

    # Curved Profile Tower (cx=24, cy=base_y+20, hw=14, hh=7, wall_h=36)
    draw_iso_box(img, 24, base_y + 20, 14, 7, 36, (70, 75, 80, 255), (130, 205, 245, 255), (85, 155, 200, 255))

    # Rooftop Helipad 'H'
    hx, hy = 24, base_y - 18
    for dy in range(-2, 3):
        img.putpixel((hx - 2, hy + dy), (255, 215, 0, 255))
        img.putpixel((hx + 2, hy + dy), (255, 215, 0, 255))
    img.putpixel((hx - 1, hy), (255, 215, 0, 255))
    img.putpixel((hx, hy), (255, 215, 0, 255))
    img.putpixel((hx + 1, hy), (255, 215, 0, 255))
    return img

# -------------------------------------------------------------
# 5. Industrial 2x2 Tiers & Variants
# -------------------------------------------------------------
def make_ind_t1_v1():
    # 48x48: Scrap Workshop & Iron Foundry (sheds, scrap yard, chimney)
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    base_y = 8

    # Workshop Shed (cx=22, cy=base_y+18, hw=12, hh=6, wall_h=10)
    draw_iso_box(img, 22, base_y + 18, 12, 6, 10, (130, 70, 50, 255), (145, 90, 70, 255), (105, 60, 45, 255))

    # Brick Smokestack with Smoke (cx=36)
    for cy in range(base_y + 2, base_y + 20):
        img.putpixel((36, cy), (140, 65, 45, 255))
    img.putpixel((36, base_y + 1), (220, 220, 225, 220))
    img.putpixel((37, base_y), (200, 200, 210, 180))

    # Scrap metal piles & pallets
    for (sx, sy) in [(10, base_y + 22), (12, base_y + 24), (28, base_y + 28)]:
        img.putpixel((sx, sy), (180, 160, 110, 255))
    return img

def make_ind_t1_v2():
    # 48x48: Corrugated Warehouse & Loading Yard
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    base_y = 8

    # Warehouse (cx=24, cy=base_y+20, hw=15, hh=8, wall_h=12)
    draw_iso_box(img, 24, base_y + 20, 15, 8, 12, (110, 115, 120, 255), (160, 165, 175, 255), (120, 125, 135, 255))

    # Rollup Loading Doors
    for lx in range(14, 20):
        ly = base_y + 17 + (lx - 14) * 5 // 6
        img.putpixel((lx, ly), (80, 85, 90, 255))
    return img

def make_ind_t2_v1():
    # 48x56: Manufacturing Plant (dual sawtooth roofs & tall brick chimney)
    img = Image.new("RGBA", (48, 56), (0, 0, 0, 0))
    base_y = 16

    # Factory Complex (cx=22, cy=base_y+20, hw=14, hh=7, wall_h=18)
    draw_iso_box(img, 22, base_y + 20, 14, 7, 18, (150, 145, 140, 255), (165, 75, 55, 255), (125, 50, 35, 255))

    # Sawtooth angled skylights
    for sx in range(12, 28):
        sy = base_y - 2 + ((sx - 12) % 8) * 3 // 8
        img.putpixel((sx, sy), (200, 230, 255, 255))

    # Heavy Industrial Smokestack with active exhaust
    for sy in range(base_y - 12, base_y + 16):
        img.putpixel((36, sy), (150, 60, 45, 255))
        img.putpixel((37, sy), (110, 45, 30, 255))
    img.putpixel((36, base_y - 13), (240, 240, 245, 230))
    img.putpixel((37, base_y - 14), (225, 230, 240, 190))
    return img

def make_ind_t2_v2():
    # 48x56: Heavy Machinery & Assembly Plant
    img = Image.new("RGBA", (48, 56), (0, 0, 0, 0))
    base_y = 16

    draw_iso_box(img, 24, base_y + 20, 15, 8, 18, (120, 125, 130, 255), (150, 130, 90, 255), (110, 95, 65, 255))

    # Storage Tanks & Pipes
    draw_iso_box(img, 34, base_y + 12, 4, 2, 8, (180, 185, 190, 255), (160, 165, 170, 255), (120, 125, 130, 255))
    return img

def make_ind_t3_v1():
    # 48x62: High-Tech Microchip Fab Campus (gleaming cleanroom facility, zero pollution)
    img = Image.new("RGBA", (48, 62), (0, 0, 0, 0))
    base_y = 22
    draw_iso_diamond(img, 24, base_y + 20, 17, 8, (110, 115, 120, 255))

    # Clean Tech Facility (cx=24, cy=base_y+20, hw=16, hh=8, wall_h=22)
    draw_iso_box(img, 24, base_y + 20, 16, 8, 22, (220, 225, 235, 255), (240, 245, 250, 255), (195, 200, 210, 255))

    # Solar Array Panels on Roof (dark blue photovoltaic cells)
    for sx in range(15, 28):
        sy = base_y - 4 + (sx - 15) * 5 // 13
        img.putpixel((sx, sy), (30, 60, 120, 255))
    # Filtration Pods
    draw_iso_box(img, 32, base_y + 6, 4, 2, 6, (140, 210, 240, 255), (180, 190, 205, 255), (140, 150, 165, 255))
    return img

def make_ind_t3_v2():
    # 48x62: Automated Robotics & AI Logistics Hub
    img = Image.new("RGBA", (48, 62), (0, 0, 0, 0))
    base_y = 22
    draw_iso_diamond(img, 24, base_y + 20, 17, 8, (105, 110, 115, 255))

    draw_iso_box(img, 24, base_y + 20, 16, 8, 22, (60, 65, 75, 255), (200, 210, 220, 255), (150, 160, 170, 255))

    # Glowing Cyan Tech Strips
    for fy in [base_y + 4, base_y + 10]:
        for tx in range(12, 23):
            img.putpixel((tx, fy + (tx - 12) * 5 // 11), (50, 220, 255, 255))
    return img

# -------------------------------------------------------------
# 6. Farm 2x2 Variants
# -------------------------------------------------------------
def make_farm_v1():
    # 48x48: Red Barn & Grain Silo with golden wheat fields
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    base_y = 8

    # Red Barn (cx=20, cy=base_y+20, hw=12, hh=6, wall_h=12)
    draw_iso_box(img, 20, base_y + 20, 12, 6, 12, (210, 80, 70, 255), (180, 45, 35, 255), (135, 30, 25, 255))
    # White 'X' Barn Door
    for dy in range(base_y + 15, base_y + 20):
        img.putpixel((16, dy), (240, 240, 245, 255))
        img.putpixel((17, dy), (240, 240, 245, 255))

    # Cylindrical Silo (cx=34)
    draw_iso_box(img, 34, base_y + 16, 4, 2, 14, (180, 185, 190, 255), (195, 200, 205, 255), (150, 155, 160, 255))
    # Silo Dome
    img.putpixel((34, base_y - 1), (220, 225, 230, 255))
    return img

def make_farm_v2():
    # 48x48: Windmill Homestead with crop irrigation
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    base_y = 8

    # Stone Mill Tower (cx=24, cy=base_y+18, hw=8, hh=4, wall_h=16)
    draw_iso_box(img, 24, base_y + 18, 8, 4, 16, (140, 60, 50, 255), (210, 205, 190, 255), (160, 155, 140, 255))

    # 4 Windmill Sails (cross)
    mx, my = 24, base_y - 2
    for d in range(-6, 7):
        img.putpixel((mx + d, my), (245, 245, 250, 255))
        img.putpixel((mx, my + d), (245, 245, 250, 255))
    img.putpixel((mx, my), (140, 70, 35, 255)) # Hub
    return img

# -------------------------------------------------------------
# 7. Civics 2x2 Structures
# -------------------------------------------------------------
def make_school_2x2():
    # 48x56: Elementary Academy Campus with clock tower & schoolyard
    img = Image.new("RGBA", (48, 56), (0, 0, 0, 0))
    base_y = 16

    # School Complex (cx=24, cy=base_y+20, hw=16, hh=8, wall_h=16)
    draw_iso_box(img, 24, base_y + 20, 16, 8, 16, (65, 80, 105, 255), (180, 60, 50, 255), (135, 45, 38, 255))

    # Central Bell/Clock Tower
    draw_iso_box(img, 24, base_y, 5, 3, 12, (180, 50, 40, 255), (235, 235, 240, 255), (190, 195, 200, 255))
    # Golden Bell inside tower
    img.putpixel((24, base_y - 6), (250, 210, 35, 255))
    img.putpixel((24, base_y - 13), (250, 215, 40, 255)) # Spire finial

    # Classroom Windows & Entrance
    for wx in [14, 18, 30, 34]:
        img.putpixel((wx, base_y + 14), (180, 235, 255, 255))
    return img

def make_church_2x2():
    # 48x64: Gothic Cathedral with twin spires & stained glass rose window
    img = Image.new("RGBA", (48, 64), (0, 0, 0, 0))
    base_y = 24

    # Cathedral Nave (cx=24, cy=base_y+20, hw=15, hh=8, wall_h=22)
    draw_iso_box(img, 24, base_y + 20, 15, 8, 22, (60, 70, 85, 255), (170, 175, 185, 255), (130, 135, 145, 255))

    # Front Steeple Spire (cx=17, cy=base_y+10, hw=5, hh=3, wall_h=26)
    draw_iso_box(img, 17, base_y + 10, 5, 3, 26, (50, 60, 75, 255), (190, 195, 205, 255), (145, 150, 160, 255))
    # Gold Steeple Crucifix Cross
    img.putpixel((17, base_y - 20), (255, 215, 0, 255))
    img.putpixel((17, base_y - 21), (255, 215, 0, 255))
    img.putpixel((16, base_y - 20), (255, 215, 0, 255))
    img.putpixel((18, base_y - 20), (255, 215, 0, 255))

    # Stained Glass Lancet Window
    for (gx, gy) in [(24, base_y + 6), (25, base_y + 7), (24, base_y + 7)]:
        img.putpixel((gx, gy), (180, 70, 220, 255)) # Violet stained glass
    return img

def make_park_2x2():
    # 48x46: Grand City Park with central marble fountain, oak grove & gazebo
    img = Image.new("RGBA", (48, 46), (0, 0, 0, 0))
    base_y = 6

    # Cobblestone Circular Plaza (center: 24, base_y+20) - 2:1 isometric ellipse
    for dy in range(-4, 5):
        for dx in range(-8, 9):
            if (dx/8.0)**2 + (dy/4.0)**2 <= 1.0:
                img.putpixel((24 + dx, base_y + 20 + dy), (185, 185, 190, 255))
    # Blue Fountain Pool (2:1 isometric ellipse)
    for dy in range(-2, 3):
        for dx in range(-4, 5):
            if (dx/4.0)**2 + (dy/2.0)**2 <= 1.0:
                img.putpixel((24 + dx, base_y + 20 + dy), (60, 180, 245, 255))
    img.putpixel((24, base_y + 19), (220, 240, 255, 255)) # Water spray
    img.putpixel((24, base_y + 18), (240, 250, 255, 255))

    # 3D Oak Tree Canopy (Trunk at 12, base_y+16)
    for ty in range(base_y + 10, base_y + 17):
        img.putpixel((12, ty), (100, 60, 35, 255))
    # Foliage Canopy (2:1 isometric ellipse)
    for dy in range(-6, 6):
        for dx in range(-7, 8):
            if (dx/7.0)**2 + (dy/5.5)**2 <= 1.0:
                px = 12 + dx
                py = base_y + 6 + dy
                if 0 <= px < 48 and 0 <= py < 46:
                    c = (70, 185, 55, 255) if dx <= 0 and dy <= 0 else (35, 115, 40, 255)
                    img.putpixel((px, py), c)
    return img

def make_harbour_2x2():
    # 48x52: Maritime Shipping Port with wooden pier dock, cargo crane & containers
    img = Image.new("RGBA", (48, 52), (0, 0, 0, 0))
    base_y = 12

    # Wooden pier dock (2:1 isometric box)
    draw_iso_box(img, 24, base_y + 20, 16, 8, 3, (155, 125, 85, 255), (130, 100, 65, 255), (105, 80, 50, 255))

    # Shipping Cargo Containers (Red, Blue, Green stack)
    draw_iso_box(img, 18, base_y + 17, 6, 3, 6, (210, 50, 45, 255), (180, 40, 35, 255), (140, 30, 25, 255))
    draw_iso_box(img, 28, base_y + 15, 6, 3, 6, (45, 110, 200, 255), (40, 90, 175, 255), (30, 70, 140, 255))

    # Cargo Crane (Yellow steel crane)
    for cy in range(base_y - 2, base_y + 14):
        img.putpixel((14, cy), (250, 200, 30, 255))
    for cx in range(14, 26):
        img.putpixel((cx, base_y - 2), (250, 200, 30, 255))
    img.putpixel((24, base_y), (80, 80, 85, 255)) # Crane cable & hook
    return img

# -------------------------------------------------------------
# Main Generation Runner
# -------------------------------------------------------------
def build_all():
    sprites = {
        # 1x1 Foundations & Roads
        "grass.png": make_grass(),
        "water.png": make_water(),
        "road.png": make_road(),
        "road-mid.png": make_road_mid(),
        # 1x1 Zone Designation Bases
        "zone_residental.png": make_zone_base((46, 204, 113), (255, 255, 255, 255)),
        "zone_commercial.png": make_zone_base((52, 152, 219), (255, 255, 255, 255)),
        "zone_industrial.png": make_zone_base((230, 126, 34), (255, 230, 50, 255)),
        "zone_farm.png": make_zone_base((180, 150, 50), (220, 220, 220, 255)),
        # 2x2 Residential Tiers & Variants
        "res_t1_v1.png": make_res_t1_v1(),
        "res_t1_v2.png": make_res_t1_v2(),
        "res_t2_v1.png": make_res_t2_v1(),
        "res_t2_v2.png": make_res_t2_v2(),
        "res_t3_v1.png": make_res_t3_v1(),
        "res_t3_v2.png": make_res_t3_v2(),
        # 2x2 Commercial Tiers & Variants
        "com_t1_v1.png": make_com_t1_v1(),
        "com_t1_v2.png": make_com_t1_v2(),
        "com_t2_v1.png": make_com_t2_v1(),
        "com_t2_v2.png": make_com_t2_v2(),
        "com_t3_v1.png": make_com_t3_v1(),
        "com_t3_v2.png": make_com_t3_v2(),
        # 2x2 Industrial Tiers & Variants
        "ind_t1_v1.png": make_ind_t1_v1(),
        "ind_t1_v2.png": make_ind_t1_v2(),
        "ind_t2_v1.png": make_ind_t2_v1(),
        "ind_t2_v2.png": make_ind_t2_v2(),
        "ind_t3_v1.png": make_ind_t3_v1(),
        "ind_t3_v2.png": make_ind_t3_v2(),
        # 2x2 Farm Variants
        "farm_v1.png": make_farm_v1(),
        "farm_v2.png": make_farm_v2(),
        # 2x2 Civics
        "school.png": make_school_2x2(),
        "church.png": make_church_2x2(),
        "park.png": make_park_2x2(),
        "harbour.png": make_harbour_2x2(),
    }

    # Aliases for default lookups
    sprites["residental.png"] = sprites["res_t1_v1.png"]
    sprites["commercial.png"] = sprites["com_t1_v1.png"]
    sprites["industrial.png"] = sprites["ind_t1_v1.png"]
    sprites["farm.png"] = sprites["farm_v1.png"]

    for filename, img in sprites.items():
        path = os.path.join(SPRITES_DIR, filename)
        img.save(path)
        print(f"Generated {filename} ({img.width}x{img.height}) -> {path}")

if __name__ == "__main__":
    build_all()
