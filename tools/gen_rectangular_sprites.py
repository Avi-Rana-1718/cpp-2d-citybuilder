#!/usr/bin/env python3
"""
Generates clean, authentic RECTANGULAR pixel-art sprites for TheoTown city builder.
All building sprites are flat-bottomed rectangles with vertical walls.
Zero diamond cutouts or V-shaped pointed wedges.
"""
import os
from PIL import Image

SPRITES_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")
os.makedirs(SPRITES_DIR, exist_ok=True)

# -------------------------------------------------------------
# Rectangular Drawing Helpers
# -------------------------------------------------------------
def fill_rect(img, x1, y1, x2, y2, color):
    """Draws a solid filled rectangle."""
    for y in range(max(0, y1), min(img.height, y2 + 1)):
        for x in range(max(0, x1), min(img.width, x2 + 1)):
            img.putpixel((x, y), color)

def draw_window(img, x, y, w, h, lit=True):
    col = (255, 240, 150, 255) if lit else (70, 90, 115, 255)
    border = (40, 50, 60, 255)
    fill_rect(img, x, y, x + w - 1, y + h - 1, border)
    fill_rect(img, x + 1, y + 1, x + w - 2, y + h - 2, col)

# -------------------------------------------------------------
# Residential Rectangular Sprites
# -------------------------------------------------------------
def make_res_t1_v1():
    # 48x44: Suburban Cottages (Flat bottom at y=43)
    img = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
    # Base pavement / foundation: x=2..45, y=41..43
    fill_rect(img, 2, 41, 45, 43, (120, 125, 120, 255))
    
    # Cottage 1 (left): x=4..22
    # Roof (pitched): y=12..22
    for y in range(12, 23):
        hw = (y - 12) + 2
        fill_rect(img, 13 - hw, y, 13 + hw, y, (160, 55, 45, 255))
    # Chimney
    fill_rect(img, 18, 8, 21, 15, (120, 45, 35, 255))
    # Walls: y=23..41
    fill_rect(img, 4, 23, 22, 41, (225, 215, 195, 255))
    # Door: x=11..15, y=32..41
    fill_rect(img, 11, 32, 15, 41, (90, 50, 35, 255))
    # Windows
    draw_window(img, 6, 26, 4, 5, True)
    draw_window(img, 17, 26, 4, 5, True)

    # Cottage 2 (right): x=25..43
    for y in range(14, 24):
        hw = (y - 14) + 2
        fill_rect(img, 34 - hw, y, 34 + hw, y, (60, 110, 150, 255))
    fill_rect(img, 25, 24, 43, 41, (235, 230, 215, 255))
    fill_rect(img, 32, 33, 36, 41, (100, 60, 40, 255))
    draw_window(img, 27, 27, 4, 5, True)
    draw_window(img, 38, 27, 4, 5, True)
    return img

def make_res_t1_v2():
    # 48x44: Bungalow Homestead
    img = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
    fill_rect(img, 3, 40, 44, 43, (110, 115, 110, 255))
    # Roof: y=12..22
    for y in range(12, 23):
        hw = (y - 12) * 2 + 1
        fill_rect(img, 24 - hw, y, 24 + hw, y, (140, 75, 40, 255))
    # Stone walls: y=23..40
    fill_rect(img, 5, 23, 42, 40, (210, 200, 185, 255))
    # Porch with columns
    fill_rect(img, 14, 27, 33, 29, (100, 55, 30, 255))
    fill_rect(img, 16, 29, 18, 40, (230, 230, 230, 255))
    fill_rect(img, 29, 29, 31, 40, (230, 230, 230, 255))
    # Door
    fill_rect(img, 22, 31, 26, 40, (80, 45, 25, 255))
    # Windows
    draw_window(img, 8, 27, 5, 6, True)
    draw_window(img, 35, 27, 5, 6, True)
    return img

def make_res_t2_v1():
    # 48x52: Brick Brownstone Apartments
    img = Image.new("RGBA", (48, 52), (0, 0, 0, 0))
    # Concrete base
    fill_rect(img, 2, 49, 45, 51, (95, 95, 100, 255))
    # Red-brick main structure: x=4..43, y=10..49
    fill_rect(img, 4, 10, 43, 49, (165, 60, 48, 255))
    # Ornate roof cornice
    fill_rect(img, 3, 8, 44, 11, (75, 80, 85, 255))
    fill_rect(img, 2, 7, 45, 8, (120, 125, 130, 255))
    # Entrance stoop & door
    fill_rect(img, 20, 38, 27, 49, (180, 180, 185, 255))
    fill_rect(img, 22, 38, 25, 46, (60, 35, 25, 255))
    # Windows on 3 floors
    for floor_y in [14, 23, 32]:
        for win_x in [7, 14, 29, 36]:
            draw_window(img, win_x, floor_y, 4, 5, True)
    # Fire escape ladders
    fill_rect(img, 20, 16, 27, 17, (40, 40, 45, 255))
    fill_rect(img, 20, 25, 27, 26, (40, 40, 45, 255))
    fill_rect(img, 20, 34, 27, 35, (40, 40, 45, 255))
    return img

def make_res_t2_v2():
    # 48x52: Urban Condominiums (Modern glass & steel)
    img = Image.new("RGBA", (48, 52), (0, 0, 0, 0))
    fill_rect(img, 3, 49, 44, 51, (80, 85, 90, 255))
    # Modern white facade: x=4..43, y=8..49
    fill_rect(img, 4, 8, 43, 49, (220, 225, 230, 255))
    # Charcoal accent bands
    fill_rect(img, 3, 6, 44, 8, (45, 50, 55, 255))
    # Glass balconies on each level
    for fy in [12, 21, 30, 39]:
        fill_rect(img, 7, fy, 40, fy + 5, (90, 165, 215, 255))
        fill_rect(img, 7, fy + 4, 40, fy + 5, (50, 70, 90, 255))
    # Glass entry lobby
    fill_rect(img, 18, 43, 29, 49, (120, 190, 235, 255))
    fill_rect(img, 22, 44, 25, 49, (60, 70, 80, 255))
    return img

def make_res_t3_v1():
    # 48x64: Highrise Luxury Tower
    img = Image.new("RGBA", (48, 64), (0, 0, 0, 0))
    fill_rect(img, 4, 61, 43, 63, (70, 75, 80, 255))
    # Skyscraper tower: x=6..41, y=10..61
    fill_rect(img, 6, 10, 41, 61, (50, 120, 180, 255))
    # Stepped crown
    fill_rect(img, 12, 5, 35, 9, (40, 95, 145, 255))
    fill_rect(img, 18, 2, 29, 4, (30, 75, 115, 255))
    # Antenna spire
    fill_rect(img, 23, 0, 24, 2, (240, 240, 240, 255))
    # Glass curtain wall windows
    for fy in range(12, 57, 5):
        for fx in [9, 15, 21, 27, 33]:
            fill_rect(img, fx, fy, fx + 3, fy + 3, (170, 225, 255, 255))
    # Entrance canopy
    fill_rect(img, 16, 56, 31, 58, (220, 200, 80, 255))
    fill_rect(img, 20, 58, 27, 61, (240, 240, 240, 255))
    return img

def make_res_t3_v2():
    # 48x64: Twin Highrise Plaza
    img = Image.new("RGBA", (48, 64), (0, 0, 0, 0))
    fill_rect(img, 2, 61, 45, 63, (65, 70, 75, 255))
    # Tower 1 (left): x=4..21, y=8..61
    fill_rect(img, 4, 8, 21, 61, (45, 110, 165, 255))
    # Tower 2 (right): x=26..43, y=8..61
    fill_rect(img, 26, 8, 43, 61, (45, 110, 165, 255))
    # Skybridge connecting towers at y=28..33
    fill_rect(img, 21, 28, 26, 33, (180, 220, 245, 255))
    fill_rect(img, 21, 28, 26, 29, (40, 50, 60, 255))
    fill_rect(img, 21, 32, 26, 33, (40, 50, 60, 255))
    # Windows
    for fy in range(12, 57, 6):
        for fx in [6, 12, 17, 28, 33, 39]:
            fill_rect(img, fx, fy, fx + 2, fy + 3, (180, 230, 255, 255))
    # Plaza lobby base: x=4..43, y=55..61
    fill_rect(img, 4, 55, 43, 61, (210, 215, 220, 255))
    return img

# -------------------------------------------------------------
# Commercial Rectangular Sprites
# -------------------------------------------------------------
def make_com_t1_v1():
    # 48x44: Corner Grocery & Bakery
    img = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
    fill_rect(img, 3, 41, 44, 43, (110, 110, 115, 255))
    # Upper facade: x=5..42, y=14..41
    fill_rect(img, 5, 14, 42, 41, (230, 210, 180, 255))
    # Parapet cornice
    fill_rect(img, 4, 11, 43, 14, (160, 80, 60, 255))
    # Storefront Awning (red & white stripes)
    for x in range(6, 42):
        col = (210, 45, 45, 255) if (x // 3) % 2 == 0 else (245, 245, 245, 255)
        fill_rect(img, x, 27, x, 31, col)
    # Glass display windows
    fill_rect(img, 8, 32, 20, 40, (140, 200, 235, 255))
    fill_rect(img, 28, 32, 40, 40, (140, 200, 235, 255))
    # Entrance door
    fill_rect(img, 22, 33, 26, 41, (90, 55, 35, 255))
    # Second-floor apartment windows
    draw_window(img, 9, 18, 5, 6, True)
    draw_window(img, 21, 18, 5, 6, True)
    draw_window(img, 33, 18, 5, 6, True)
    return img

def make_com_t1_v2():
    # 48x44: Local Diner & Mini Mart
    img = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
    fill_rect(img, 3, 41, 44, 43, (100, 105, 110, 255))
    fill_rect(img, 5, 16, 42, 41, (215, 225, 235, 255))
    # Neon diner trim (turquoise & magenta)
    fill_rect(img, 4, 13, 43, 16, (40, 180, 180, 255))
    fill_rect(img, 14, 8, 34, 13, (225, 45, 120, 255)) # "DINER" sign box
    # Large plate glass panoramic diner windows
    fill_rect(img, 7, 26, 40, 39, (120, 195, 240, 255))
    fill_rect(img, 7, 38, 40, 39, (50, 70, 90, 255))
    fill_rect(img, 21, 28, 27, 41, (230, 230, 235, 255)) # Chrome door
    return img

def make_com_t2_v1():
    # 48x52: Shopping Plaza Arcade
    img = Image.new("RGBA", (48, 52), (0, 0, 0, 0))
    fill_rect(img, 2, 49, 45, 51, (85, 90, 95, 255))
    # Plaza building: x=4..43, y=8..49
    fill_rect(img, 4, 8, 43, 49, (215, 220, 225, 255))
    # Green glass facade panels
    fill_rect(img, 4, 6, 43, 8, (35, 120, 95, 255))
    # Second floor glass gallery
    fill_rect(img, 7, 14, 40, 26, (95, 175, 155, 255))
    for x in range(7, 41, 6):
        fill_rect(img, x, 14, x, 26, (40, 60, 55, 255))
    # Ground floor storefront canopy
    fill_rect(img, 5, 29, 42, 32, (30, 100, 80, 255))
    # Ground floor shops
    fill_rect(img, 7, 33, 19, 48, (140, 210, 235, 255))
    fill_rect(img, 28, 33, 40, 48, (140, 210, 235, 255))
    fill_rect(img, 21, 33, 26, 49, (240, 240, 245, 255)) # Glass entrance
    return img

def make_com_t2_v2():
    # 48x52: Department Store Arcade
    img = Image.new("RGBA", (48, 52), (0, 0, 0, 0))
    fill_rect(img, 2, 49, 45, 51, (90, 95, 100, 255))
    fill_rect(img, 4, 10, 43, 49, (225, 215, 195, 255))
    # Grand pediment & clock
    fill_rect(img, 14, 3, 33, 10, (200, 185, 160, 255))
    fill_rect(img, 22, 5, 25, 8, (245, 215, 50, 255)) # Gold clock
    # Columns along facade
    for cx in [6, 14, 22, 30, 38]:
        fill_rect(img, cx, 10, cx + 2, 48, (245, 240, 230, 255))
    # Windows
    for fy in [15, 26]:
        for wx in [10, 18, 26, 34]:
            draw_window(img, wx, fy, 4, 6, True)
    # Entrance
    fill_rect(img, 18, 38, 29, 49, (80, 50, 35, 255))
    return img

def make_com_t3_v1():
    # 48x64: Corporate Banking Highrise
    img = Image.new("RGBA", (48, 64), (0, 0, 0, 0))
    fill_rect(img, 3, 61, 44, 63, (65, 70, 75, 255))
    # Main tower: x=6..41, y=8..61
    fill_rect(img, 6, 8, 41, 61, (35, 95, 155, 255))
    # Stepped glass crown
    fill_rect(img, 10, 4, 37, 7, (25, 75, 125, 255))
    fill_rect(img, 16, 1, 31, 3, (20, 60, 100, 255))
    fill_rect(img, 23, 0, 24, 1, (255, 255, 255, 255)) # Spire beacon
    # Reflective blue glass grid
    for fy in range(10, 56, 5):
        for fx in range(8, 40, 6):
            fill_rect(img, fx, fy, fx + 4, fy + 3, (120, 205, 255, 255))
    # Grand Banking Entrance & Pillars
    fill_rect(img, 6, 55, 41, 61, (220, 225, 230, 255))
    fill_rect(img, 18, 56, 29, 61, (30, 40, 50, 255))
    return img

def make_com_t3_v2():
    # 48x64: International Financial Center
    img = Image.new("RGBA", (48, 64), (0, 0, 0, 0))
    fill_rect(img, 3, 61, 44, 63, (60, 65, 70, 255))
    fill_rect(img, 5, 8, 42, 61, (45, 55, 65, 255))
    # Helipad on top
    fill_rect(img, 9, 4, 38, 7, (180, 185, 190, 255))
    fill_rect(img, 21, 5, 26, 6, (230, 50, 50, 255)) # 'H'
    # Curved cyan glass curtain wall
    for fy in range(10, 56, 5):
        fill_rect(img, 8, fy, 39, fy + 3, (100, 215, 240, 255))
        fill_rect(img, 8, fy + 3, 39, fy + 4, (30, 70, 85, 255))
    # Entrance
    fill_rect(img, 16, 56, 31, 61, (240, 245, 250, 255))
    return img

# -------------------------------------------------------------
# Industrial Rectangular Sprites
# -------------------------------------------------------------
def make_ind_t1_v1():
    # 48x44: Scrap Workshop & Foundry
    img = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
    fill_rect(img, 3, 41, 44, 43, (85, 85, 90, 255))
    # Brick factory hall: x=5..42, y=16..41
    fill_rect(img, 5, 16, 42, 41, (135, 75, 55, 255))
    # Smokestack (left): x=7..11, y=4..16
    fill_rect(img, 7, 4, 11, 16, (110, 60, 45, 255))
    fill_rect(img, 6, 2, 12, 4, (70, 70, 75, 255))
    fill_rect(img, 7, 0, 11, 2, (200, 205, 210, 180)) # Smoke puff
    # Corrugated metal roof
    for x in range(13, 43):
        col = (115, 120, 125, 255) if x % 2 == 0 else (95, 100, 105, 255)
        fill_rect(img, x, 14, x, 18, col)
    # Roll-up cargo doors
    fill_rect(img, 15, 27, 26, 41, (160, 165, 170, 255))
    for y in range(29, 41, 3):
        fill_rect(img, 15, y, 26, y, (100, 105, 110, 255))
    # Furnace glow window
    fill_rect(img, 31, 28, 38, 34, (245, 140, 40, 255))
    return img

def make_ind_t1_v2():
    # 48x44: Warehouse & Logistics Depot
    img = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
    fill_rect(img, 2, 41, 45, 43, (90, 95, 100, 255))
    # Steel warehouse shed: x=4..43, y=14..41
    fill_rect(img, 4, 14, 43, 41, (170, 175, 185, 255))
    # Blue roof
    fill_rect(img, 3, 10, 44, 14, (45, 80, 120, 255))
    # 2 Roll-up loading docks
    fill_rect(img, 8, 25, 20, 41, (120, 125, 130, 255))
    fill_rect(img, 25, 25, 37, 41, (120, 125, 130, 255))
    for y in range(27, 41, 3):
        fill_rect(img, 8, y, 20, y, (80, 85, 90, 255))
        fill_rect(img, 25, y, 37, y, (80, 85, 90, 255))
    return img

def make_ind_t2_v1():
    # 48x52: Manufacturing Plant
    img = Image.new("RGBA", (48, 52), (0, 0, 0, 0))
    fill_rect(img, 2, 49, 45, 51, (80, 85, 90, 255))
    fill_rect(img, 4, 16, 43, 49, (140, 145, 150, 255))
    # Twin industrial smokestacks
    fill_rect(img, 8, 4, 12, 16, (175, 80, 65, 255))
    fill_rect(img, 7, 2, 13, 4, (60, 60, 65, 255))
    fill_rect(img, 18, 6, 22, 16, (175, 80, 65, 255))
    fill_rect(img, 17, 4, 23, 6, (60, 60, 65, 255))
    # Sawtooth skylight roof
    for sx in [24, 33]:
        for y in range(10, 17):
            fill_rect(img, sx + (y - 10), y, sx + 8, y, (90, 160, 210, 255))
    # Assembly floor windows
    for wx in [7, 16, 25, 34]:
        draw_window(img, wx, 24, 5, 8, True)
    # Cargo bay
    fill_rect(img, 16, 37, 31, 49, (100, 105, 115, 255))
    return img

def make_ind_t2_v2():
    # 48x52: Heavy Machine Assembly
    img = Image.new("RGBA", (48, 52), (0, 0, 0, 0))
    fill_rect(img, 2, 49, 45, 51, (75, 80, 85, 255))
    fill_rect(img, 4, 12, 43, 49, (85, 90, 100, 255))
    # Overhead yellow gantry crane structure on roof
    fill_rect(img, 8, 4, 11, 12, (235, 190, 40, 255))
    fill_rect(img, 36, 4, 39, 12, (235, 190, 40, 255))
    fill_rect(img, 8, 4, 39, 6, (235, 190, 40, 255))
    # Turbine vents & exhaust
    fill_rect(img, 21, 6, 26, 12, (180, 185, 190, 255))
    # Steel bay doors
    fill_rect(img, 12, 30, 35, 49, (140, 145, 155, 255))
    for y in range(32, 49, 4):
        fill_rect(img, 12, y, 35, y, (90, 95, 105, 255))
    return img

def make_ind_t3_v1():
    # 48x60: Microchip Clean-Fab Hub
    img = Image.new("RGBA", (48, 60), (0, 0, 0, 0))
    fill_rect(img, 2, 57, 45, 59, (90, 95, 100, 255))
    # Pristine white high-tech clean room building
    fill_rect(img, 4, 14, 43, 57, (240, 245, 250, 255))
    # Solar panel roof array (navy blue)
    fill_rect(img, 6, 8, 41, 14, (30, 50, 95, 255))
    for x in range(6, 42, 6):
        fill_rect(img, x, 8, x, 14, (120, 160, 220, 255))
    # Glowing cyan data lines
    fill_rect(img, 4, 24, 43, 25, (40, 220, 240, 255))
    fill_rect(img, 4, 38, 43, 39, (40, 220, 240, 255))
    # Clean room air-lock entrance
    fill_rect(img, 17, 44, 30, 57, (70, 80, 90, 255))
    fill_rect(img, 20, 46, 27, 57, (180, 240, 255, 255))
    return img

def make_ind_t3_v2():
    # 48x60: Robotics Automation Facility
    img = Image.new("RGBA", (48, 60), (0, 0, 0, 0))
    fill_rect(img, 2, 57, 45, 59, (80, 85, 90, 255))
    fill_rect(img, 4, 14, 43, 57, (60, 65, 75, 255))
    # Satellite dish & comms tower
    fill_rect(img, 22, 2, 25, 14, (200, 205, 210, 255))
    fill_rect(img, 18, 4, 29, 6, (220, 225, 230, 255))
    # Amber sensor windows
    for fy in range(20, 48, 8):
        for fx in [8, 17, 26, 35]:
            fill_rect(img, fx, fy, fx + 4, fy + 4, (250, 180, 45, 255))
    # Heavy blast doors
    fill_rect(img, 16, 45, 31, 57, (40, 45, 50, 255))
    return img

# -------------------------------------------------------------
# Agricultural Farms
# -------------------------------------------------------------
def make_farm_v1():
    # 48x44: Barn & Wheat Silo
    img = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
    fill_rect(img, 2, 41, 45, 43, (120, 110, 80, 255))
    # Silo (left): x=4..13, y=8..41
    fill_rect(img, 4, 12, 13, 41, (190, 195, 200, 255))
    fill_rect(img, 4, 8, 13, 11, (150, 155, 160, 255)) # Silo dome
    # Red Barn (right): x=15..43, y=14..41
    # Gambrel roof: y=14..24
    for y in range(14, 25):
        hw = (y - 14) + 3
        fill_rect(img, 29 - hw, y, 29 + hw, y, (195, 200, 205, 255))
    # Crimson wood barn walls
    fill_rect(img, 15, 25, 43, 41, (175, 45, 35, 255))
    # White barn doors with X-cross
    fill_rect(img, 24, 30, 34, 41, (245, 245, 245, 255))
    fill_rect(img, 25, 31, 33, 41, (160, 40, 30, 255))
    fill_rect(img, 28, 30, 29, 41, (245, 245, 245, 255))
    # Hayloft window
    fill_rect(img, 27, 20, 31, 24, (245, 245, 245, 255))
    fill_rect(img, 28, 21, 30, 24, (70, 40, 20, 255))
    return img

def make_farm_v2():
    # 48x44: Windmill Homestead
    img = Image.new("RGBA", (48, 44), (0, 0, 0, 0))
    fill_rect(img, 2, 41, 45, 43, (110, 120, 80, 255))
    # Cottage house (left): x=4..20, y=22..41
    fill_rect(img, 4, 26, 20, 41, (230, 220, 200, 255))
    for y in range(20, 26):
        fill_rect(img, 4 + (y - 20), y, 20 - (y - 20), y, (150, 80, 45, 255)) # Thatched roof
    draw_window(img, 7, 30, 4, 5, True)
    # Windmill tower (right): x=24..38, y=12..41
    fill_rect(img, 25, 12, 37, 41, (215, 210, 200, 255))
    fill_rect(img, 27, 8, 35, 12, (120, 70, 40, 255)) # Cap
    # 4 Windmill Sails (white timber)
    center = (31, 14)
    fill_rect(img, 31, 0, 32, 28, (245, 245, 245, 255))  # Vertical sail
    fill_rect(img, 17, 13, 45, 14, (245, 245, 245, 255)) # Horizontal sail
    fill_rect(img, 30, 13, 32, 15, (60, 40, 20, 255))   # Rotor hub
    return img

# -------------------------------------------------------------
# Civics & Infrastructure (2x2)
# -------------------------------------------------------------
def make_school():
    # 48x52: Elementary School
    img = Image.new("RGBA", (48, 52), (0, 0, 0, 0))
    fill_rect(img, 2, 49, 45, 51, (100, 105, 110, 255))
    # Classic red-brick schoolhouse: x=4..43, y=14..49
    fill_rect(img, 4, 14, 43, 49, (180, 65, 50, 255))
    # Central clock/bell tower: x=20..27, y=2..14
    fill_rect(img, 20, 6, 27, 14, (180, 65, 50, 255))
    fill_rect(img, 19, 2, 28, 6, (60, 120, 110, 255)) # Patina copper roof
    fill_rect(img, 22, 8, 25, 11, (250, 225, 60, 255)) # School clock
    # White cornices & trim
    fill_rect(img, 3, 13, 44, 15, (245, 245, 245, 255))
    # Windows
    for fy in [19, 29]:
        for wx in [7, 13, 30, 36]:
            draw_window(img, wx, fy, 4, 6, True)
    # Entrance
    fill_rect(img, 20, 38, 27, 49, (245, 245, 245, 255))
    fill_rect(img, 22, 40, 25, 49, (70, 45, 30, 255))
    return img

def make_church():
    # 48x60: Community Cathedral
    img = Image.new("RGBA", (48, 60), (0, 0, 0, 0))
    fill_rect(img, 2, 57, 45, 59, (90, 95, 100, 255))
    # Cathedral limestone nave: x=4..43, y=20..57
    fill_rect(img, 4, 20, 43, 57, (215, 210, 200, 255))
    # Central Gothic Spire: x=20..27, y=4..20
    fill_rect(img, 20, 10, 27, 20, (190, 185, 175, 255))
    for y in range(4, 10):
        hw = (y - 4) // 2
        fill_rect(img, 23 - hw, y, 24 + hw, y, (65, 125, 115, 255)) # Copper spire
    fill_rect(img, 23, 0, 24, 4, (245, 215, 50, 255)) # Gold Cross
    # Stained-glass Rose Window
    fill_rect(img, 21, 24, 26, 29, (230, 50, 60, 255))
    fill_rect(img, 22, 25, 25, 28, (60, 160, 240, 255))
    # Arched stained glass windows along sides
    for wx in [8, 14, 31, 37]:
        fill_rect(img, wx, 28, wx + 3, 44, (90, 180, 230, 255))
        fill_rect(img, wx + 1, 26, wx + 2, 28, (90, 180, 230, 255))
    # Ornate arched double doors
    fill_rect(img, 20, 43, 27, 57, (85, 50, 30, 255))
    fill_rect(img, 21, 41, 26, 43, (85, 50, 30, 255))
    return img

def make_park():
    # 48x40: Grand Nature Park
    img = Image.new("RGBA", (48, 40), (0, 0, 0, 0))
    # Lush grass lawn: x=2..45, y=10..39
    fill_rect(img, 2, 10, 45, 39, (65, 170, 60, 255))
    # Paved stone plaza in center: x=16..31, y=16..34
    fill_rect(img, 16, 16, 31, 34, (190, 195, 200, 255))
    # Fountain basin (outer ring)
    fill_rect(img, 19, 19, 28, 31, (130, 135, 145, 255))
    fill_rect(img, 21, 21, 26, 29, (60, 175, 235, 255)) # Sparkling blue water
    fill_rect(img, 23, 17, 24, 25, (230, 245, 255, 255)) # Water jet spray
    # Shaded oak trees on left and right
    # Left tree: trunk at x=7..9, y=24..34; foliage at y=10..24
    fill_rect(img, 8, 24, 9, 34, (100, 60, 35, 255))
    for y in range(10, 25):
        hw = 6 if y < 18 else 4
        fill_rect(img, 8 - hw, y, 9 + hw, y, (45, 135, 45, 255))
    # Right tree
    fill_rect(img, 38, 24, 39, 34, (100, 60, 35, 255))
    for y in range(10, 25):
        hw = 6 if y < 18 else 4
        fill_rect(img, 38 - hw, y, 39 + hw, y, (45, 135, 45, 255))
    # Colorful flower beds
    for fx in [13, 14, 33, 34]:
        fill_rect(img, fx, 32, fx + 1, 34, (245, 70, 120, 255))
    return img

def make_harbour():
    # 48x48: Maritime Trade Port
    img = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    # Concrete dock quay: x=2..45, y=32..47
    fill_rect(img, 2, 32, 45, 47, (130, 135, 140, 255))
    # Wooden pier pilings along water edge
    for px in range(4, 45, 8):
        fill_rect(img, px, 44, px + 2, 47, (80, 50, 30, 255))
    # Yellow Cargo Gantry Crane (left): x=4..18, y=4..32
    fill_rect(img, 5, 8, 7, 32, (245, 200, 40, 255))
    fill_rect(img, 15, 8, 17, 32, (245, 200, 40, 255))
    fill_rect(img, 4, 8, 26, 11, (245, 200, 40, 255)) # Crane boom
    fill_rect(img, 12, 11, 14, 20, (50, 50, 55, 255))  # Hoist cable
    fill_rect(img, 10, 20, 16, 25, (220, 50, 50, 255))  # Lifted red container
    # Stacked Shipping Containers (right)
    # Bottom blue container: x=24..43, y=24..32
    fill_rect(img, 24, 24, 43, 32, (35, 95, 170, 255))
    # Top green container: x=26..41, y=15..23
    fill_rect(img, 26, 15, 41, 23, (40, 145, 75, 255))
    return img

# -------------------------------------------------------------
# Generate All Sprites
# -------------------------------------------------------------
def generate_all():
    sprites = {
        # Residential
        "res_t1_v1": make_res_t1_v1(),
        "res_t1_v2": make_res_t1_v2(),
        "res_t2_v1": make_res_t2_v1(),
        "res_t2_v2": make_res_t2_v2(),
        "res_t3_v1": make_res_t3_v1(),
        "res_t3_v2": make_res_t3_v2(),
        # Commercial
        "com_t1_v1": make_com_t1_v1(),
        "com_t1_v2": make_com_t1_v2(),
        "com_t2_v1": make_com_t2_v1(),
        "com_t2_v2": make_com_t2_v2(),
        "com_t3_v1": make_com_t3_v1(),
        "com_t3_v2": make_com_t3_v2(),
        # Industrial
        "ind_t1_v1": make_ind_t1_v1(),
        "ind_t1_v2": make_ind_t1_v2(),
        "ind_t2_v1": make_ind_t2_v1(),
        "ind_t2_v2": make_ind_t2_v2(),
        "ind_t3_v1": make_ind_t3_v1(),
        "ind_t3_v2": make_ind_t3_v2(),
        # Farms
        "farm_v1": make_farm_v1(),
        "farm_v2": make_farm_v2(),
        # Civics
        "school": make_school(),
        "church": make_church(),
        "park": make_park(),
        "harbour": make_harbour(),
    }

    # Aliases
    sprites["residental"] = sprites["res_t1_v1"]
    sprites["commercial"] = sprites["com_t1_v1"]
    sprites["industrial"] = sprites["ind_t1_v1"]
    sprites["farm"] = sprites["farm_v1"]

    for name, img in sprites.items():
        path = os.path.join(SPRITES_DIR, f"{name}.png")
        img.save(path)
        print(f"Generated {name}.png ({img.width}x{img.height}) with flat rectangular base.")

if __name__ == "__main__":
    generate_all()
