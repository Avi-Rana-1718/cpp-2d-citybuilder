#!/usr/bin/env python3
"""
Power & water building sprites (2x2 lots, 48 x H, drawn 2x in game).

Writes assets/sprites/util_<name>.png for the eight utility kinds:
coal, wind, solar, transformer, well, pump, treatment, tower.

Layout matches the other 2x2 building sprites: the game draws the sprite
at (lot x - 16, lot y) with its origin H - 40 pixels from the top, so the
lot's diamond is centred on column 24, row H - 24 (half size 24 x 12 in
sprite pixels). Everything stands on that diamond.
"""
import os
from PIL import Image

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")


def shade(c, d):
    return (max(0, min(255, c[0] + d)), max(0, min(255, c[1] + d)), max(0, min(255, c[2] + d)), c[3] if len(c) > 3 else 255)


def put(img, x, y, c):
    if 0 <= x < img.width and 0 <= y < img.height:
        img.putpixel((x, y), c)


def diamond(img, cx, cy, hw, hh, col, dither=0):
    for y in range(cy - hh, cy + hh + 1):
        for x in range(cx - hw, cx + hw + 1):
            if abs(x - cx) / float(hw) + abs(y - cy) / float(hh) <= 1.001:
                put(img, x, y, shade(col, -dither) if dither and (x + 2 * y) % 3 == 0 else col)


def box(img, cx, cy, hw, hh, h, top, left, right):
    """Iso box standing on the diamond centred at (cx, cy)."""
    for x in range(cx - hw, cx + 1):
        yb = cy + (x - (cx - hw)) * hh // hw
        for y in range(yb - h, yb + 1):
            put(img, x, y, left if (x + y) % 2 == 0 else shade(left, -10))
    for x in range(cx, cx + hw + 1):
        yb = cy + hh - (x - cx) * hh // hw
        for y in range(yb - h, yb + 1):
            put(img, x, y, right if (x + y) % 2 == 0 else shade(right, -10))
    diamond(img, cx, cy - h, hw, hh, top, dither=12)


def cylinder(img, cx, cy, r, h, side, top):
    """Upright cylinder: rr = horizontal radius, vertical radius r // 2."""
    rv = max(1, r // 2)
    for x in range(cx - r, cx + r + 1):
        t = (x - (cx - r)) / float(2 * r)
        c = shade(side, int(30 - 60 * t))
        dy = int(round(rv * (1 - ((x - cx) / float(r)) ** 2) ** 0.5))
        for y in range(cy - h, cy + dy + 1):
            put(img, x, y, c)
    for y in range(cy - h - rv, cy - h + rv + 1):
        for x in range(cx - r, cx + r + 1):
            if ((x - cx) / float(r)) ** 2 + ((y - (cy - h)) / float(rv)) ** 2 <= 1.0:
                put(img, x, y, top)


def line(img, x0, y0, x1, y1, c):
    n = max(abs(x1 - x0), abs(y1 - y0), 1)
    for i in range(n + 1):
        put(img, round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n), c)


def lot(h, ground=(116, 112, 104, 255)):
    img = Image.new("RGBA", (48, h), (0, 0, 0, 0))
    cy = h - 24
    diamond(img, 24, cy, 23, 11, ground, dither=10)
    return img, cy


CONCRETE = (150, 148, 140, 255)
GRAVEL = (128, 124, 116, 255)
GRASS = (92, 150, 70, 255)


def coal():
    img, cy = lot(72, (96, 92, 86, 255))
    diamond(img, 33, cy + 3, 8, 4, (28, 28, 32, 255), dither=8)           # coal heap
    diamond(img, 33, cy + 1, 5, 2, (40, 40, 46, 255))
    box(img, 18, cy + 2, 12, 6, 14, (90, 92, 98, 255), (150, 70, 52, 255), (115, 52, 40, 255))   # boiler hall
    for wx in range(9, 17, 3):
        put(img, wx, cy - 3 + (wx - 6) // 2, (250, 210, 120, 255))
    cylinder(img, 32, cy - 4, 3, 30, (170, 165, 160, 255), (60, 55, 50, 255))  # chimney
    for i, y in enumerate(range(cy - 36, cy - 30)):
        put(img, 32, y, (190, 60, 50, 255) if i % 3 == 0 else (175, 170, 165, 255))
    for (sx, sy, r) in [(33, 8, 3), (37, 4, 4), (42, 1, 3)]:                # smoke
        for y in range(sy - r, sy + r + 1):
            for x in range(sx - r, sx + r + 1):
                if (x - sx) ** 2 + (y - sy) ** 2 <= r * r:
                    put(img, x, y, (205, 205, 210, 200) if (x + y) % 3 else (180, 180, 188, 200))
    return img


def turbine(img, x, base, h, phase):
    for y in range(base - h, base + 1):
        put(img, x, y, (235, 238, 242, 255))
        put(img, x + 1, y, (190, 195, 205, 255))
    hub = (x, base - h)
    import math
    for k in range(3):
        a = phase + k * 2.0944
        line(img, hub[0], hub[1], hub[0] + round(11 * math.cos(a)), hub[1] + round(11 * math.sin(a)), (245, 247, 250, 255))
    put(img, hub[0], hub[1], (120, 125, 135, 255))
    put(img, hub[0] + 1, hub[1], (120, 125, 135, 255))


def wind():
    img, cy = lot(76, GRASS)
    diamond(img, 15, cy + 2, 4, 2, CONCRETE)
    diamond(img, 33, cy - 2, 4, 2, CONCRETE)
    turbine(img, 32, cy - 2, 40, 0.4)
    turbine(img, 14, cy + 2, 36, 1.3)
    return img


def solar():
    img, cy = lot(44, GRASS)
    for row in range(3):
        for col in range(3):
            pcx = 24 + (col - row) * 7
            pcy = cy - 4 + (col + row) * 3 - 3
            diamond(img, pcx, pcy + 1, 6, 3, (60, 60, 64, 255))                # frame / shadow
            diamond(img, pcx, pcy, 5, 2, (40, 70, 140, 255))
            put(img, pcx - 1, pcy - 1, (150, 200, 250, 255))
            put(img, pcx + 2, pcy, (110, 160, 230, 255))
    return img


def transformer():
    img, cy = lot(50, GRAVEL)
    box(img, 17, cy + 1, 6, 3, 8, (140, 145, 150, 255), (110, 115, 122, 255), (88, 92, 100, 255))
    box(img, 30, cy - 2, 6, 3, 8, (140, 145, 150, 255), (110, 115, 122, 255), (88, 92, 100, 255))
    for (x, y) in [(15, cy - 10), (19, cy - 10), (28, cy - 13), (32, cy - 13)]:   # insulators
        for k in range(4):
            put(img, x, y - k, (225, 140, 60, 255) if k % 2 else (240, 240, 235, 255))
    line(img, 15, cy - 14, 28, cy - 17, (40, 40, 40, 255))
    line(img, 19, cy - 14, 32, cy - 17, (40, 40, 40, 255))
    # fence posts around the edge
    for i in range(0, 24, 4):
        for (x, y) in [(1 + i, cy - i // 2), (24 + i, cy - 11 + i // 2), (1 + i, cy + i // 2), (24 + i, cy + 11 - i // 2)]:
            put(img, x, y, (200, 200, 205, 255))
            put(img, x, y - 1, (200, 200, 205, 255))
    diamond(img, 24, cy + 6, 2, 1, (250, 205, 50, 255))                     # warning sign
    return img


def well():
    img, cy = lot(48, GRASS)
    cylinder(img, 24, cy + 2, 6, 4, (150, 140, 128, 255), (30, 60, 110, 255))    # stone ring + water
    for x in (19, 29):
        for y in range(cy - 10, cy + 1):
            put(img, x, y, (110, 75, 45, 255))
    line(img, 17, cy - 9, 24, cy - 14, (160, 60, 45, 255))                      # roof
    line(img, 24, cy - 14, 31, cy - 9, (130, 45, 35, 255))
    line(img, 18, cy - 9, 24, cy - 13, (160, 60, 45, 255))
    line(img, 24, cy - 13, 30, cy - 9, (130, 45, 35, 255))
    line(img, 19, cy - 7, 29, cy - 7, (90, 60, 35, 255))                         # crank
    put(img, 24, cy - 6, (80, 80, 85, 255))
    put(img, 24, cy - 5, (120, 90, 50, 255))                                     # bucket
    return img


def pump():
    img, cy = lot(58, GRAVEL)
    box(img, 18, cy + 1, 11, 6, 13, (70, 110, 150, 255), (140, 160, 180, 255), (105, 125, 145, 255))  # pump house
    for wx in (10, 14):
        put(img, wx, cy - 6 + (wx - 7) // 2, (190, 230, 255, 255))
    cylinder(img, 34, cy - 1, 5, 12, (90, 140, 200, 255), (150, 190, 230, 255))   # raw water tank
    for x in range(26, 32):                                                       # pipe to tank
        put(img, x, cy - 3, (110, 115, 125, 255))
        put(img, x, cy - 2, (80, 85, 95, 255))
    cylinder(img, 30, cy + 7, 2, 3, (110, 115, 125, 255), (40, 40, 45, 255))       # bore head
    return img


def treatment():
    img, cy = lot(50, CONCRETE)
    cylinder(img, 15, cy + 1, 8, 4, (170, 170, 165, 255), (60, 130, 120, 255))     # settling basin (murky)
    cylinder(img, 33, cy - 2, 8, 4, (170, 170, 165, 255), (70, 160, 225, 255))     # clean basin
    line(img, 9, cy - 3, 21, cy - 3, (90, 90, 95, 255))                            # bridge / scraper arm
    line(img, 27, cy - 6, 39, cy - 6, (90, 90, 95, 255))
    box(img, 25, cy + 8, 5, 3, 7, (60, 100, 140, 255), (200, 205, 210, 255), (160, 165, 172, 255))  # control hut
    return img


def tower():
    img, cy = lot(78, GRASS)
    legs = [(16, cy + 2), (32, cy + 2), (20, cy - 3), (28, cy + 6)]
    top = cy - 26
    for (x, y) in legs:
        line(img, x, y, 24 + (x - 24) // 2, top, (120, 125, 135, 255))
    line(img, 18, cy - 8, 30, cy - 8, (120, 125, 135, 255))
    cylinder(img, 24, top, 11, 14, (120, 180, 230, 255), (90, 140, 190, 255))       # tank
    diamond(img, 24, top - 18, 6, 3, (80, 120, 165, 255))                          # roof cap
    for x in range(14, 35, 4):
        put(img, x, top - 6, (230, 240, 250, 255))                                  # band
    return img


def police():
    img, cy = lot(52, CONCRETE)
    box(img, 22, cy + 1, 14, 7, 12, (50, 70, 130, 255), (215, 220, 230, 255), (170, 176, 190, 255))
    for wx in (12, 16, 20):
        put(img, wx, cy - 5 + (wx - 8) // 2, (120, 170, 240, 255))
    diamond(img, 22, cy - 12, 4, 2, (40, 60, 160, 255))                        # roof sign
    put(img, 21, cy - 15, (230, 40, 40, 255))                                  # siren lights
    put(img, 23, cy - 15, (60, 120, 255, 255))
    box(img, 37, cy + 3, 4, 2, 3, (30, 30, 40, 255), (240, 240, 245, 255), (60, 80, 170, 255))   # patrol car
    return img


def fire():
    img, cy = lot(56, CONCRETE)
    box(img, 20, cy + 1, 13, 7, 14, (110, 40, 35, 255), (205, 60, 45, 255), (160, 45, 35, 255))
    for k in range(3):                                                         # garage doors
        x0 = 10 + 6 * k
        for y in range(cy - 6 + k * 3, cy + k * 3):
            put(img, x0, y, (235, 230, 220, 255))
            put(img, x0 + 1, y, (210, 205, 195, 255))
    box(img, 33, cy - 2, 3, 2, 22, (90, 90, 95, 255), (200, 200, 205, 255), (160, 160, 165, 255))   # hose tower
    return img


def clinic():
    img, cy = lot(52, CONCRETE)
    box(img, 23, cy + 1, 15, 7, 13, (220, 225, 230, 255), (240, 242, 246, 255), (200, 205, 212, 255))
    for x in range(21, 26):
        put(img, x, cy - 15, (220, 40, 50, 255))                               # red cross on roof
    for y in range(cy - 17, cy - 12):
        put(img, 23, y, (220, 40, 50, 255))
    for wx in (11, 15, 19):
        put(img, wx, cy - 4 + (wx - 8) // 2, (130, 200, 240, 255))
    return img


def landfill():
    img, cy = lot(44, (120, 105, 80, 255))
    for (cx, dy, r, c) in [(16, 2, 7, (110, 100, 85)), (28, -1, 8, (95, 88, 78)), (22, 4, 5, (130, 115, 90))]:
        diamond(img, cx, cy + dy, r, r // 2 + 1, c + (255,), dither=14)
        diamond(img, cx, cy + dy - 2, r - 3, max(1, (r - 3) // 2), shade(c + (255,), 15), dither=10)
    for (x, y, col) in [(15, cy - 1, (200, 60, 50)), (27, cy - 3, (60, 120, 200)), (30, cy, (230, 200, 60)), (19, cy + 3, (240, 240, 240))]:
        put(img, x, y, col + (255,))
    box(img, 37, cy + 4, 3, 2, 3, (200, 170, 40, 255), (230, 200, 60, 255), (190, 160, 40, 255))   # bulldozer
    return img


def mine():
    img, cy = lot(60, (110, 100, 90, 255))
    diamond(img, 30, cy + 3, 8, 4, (40, 38, 36, 255), dither=8)                # ore pile
    for x in (12, 22):                                                         # headframe legs
        line(img, x, cy + 2, 17, cy - 22, (120, 70, 40, 255))
    line(img, 13, cy - 10, 21, cy - 10, (120, 70, 40, 255))
    cylinder(img, 17, cy - 22, 3, 1, (90, 90, 95, 255), (160, 160, 165, 255))  # wheel
    box(img, 16, cy + 4, 6, 3, 6, (90, 80, 70, 255), (150, 120, 90, 255), (120, 95, 70, 255))   # shaft house
    for (x, y) in [(36, cy - 1), (39, cy + 1)]:
        box(img, x, y, 2, 1, 2, (60, 55, 50, 255), (170, 150, 60, 255), (130, 115, 45, 255))     # carts
    return img


def bus():
    img, cy = lot(48, CONCRETE)
    box(img, 18, cy, 12, 6, 10, (80, 85, 95, 255), (200, 190, 150, 255), (165, 155, 120, 255))     # depot hall
    for k in range(2):                                                         # buses
        bx, by = 32 + 4 * k, cy + 3 + 2 * k
        box(img, bx, by, 6, 3, 4, (240, 240, 240, 255), (245, 200, 50, 255), (215, 170, 40, 255))
    return img


def outflow():
    img, cy = lot(44, GRASS)
    box(img, 20, cy + 1, 7, 4, 5, (110, 105, 95, 255), (150, 145, 135, 255), (120, 115, 105, 255))   # pump house
    for k in range(10):                                                         # pipe towards the water
        put(img, 26 + k, cy + 3 + k // 2, (90, 95, 100, 255))
        put(img, 26 + k, cy + 4 + k // 2, (60, 62, 66, 255))
    diamond(img, 38, cy + 9, 4, 2, (110, 100, 60, 255))                         # murky outflow
    return img


def sewage_plant():
    img, cy = lot(48, CONCRETE)
    cylinder(img, 15, cy + 1, 7, 3, (160, 160, 155, 255), (120, 105, 60, 255))   # sludge tank
    cylinder(img, 31, cy - 2, 7, 3, (160, 160, 155, 255), (90, 130, 110, 255))   # aeration tank
    for x in range(26, 37, 3):
        put(img, x, cy - 5, (220, 235, 230, 255))                                 # bubbles
    box(img, 25, cy + 8, 5, 3, 6, (70, 80, 70, 255), (190, 195, 185, 255), (150, 155, 145, 255))
    return img


def incinerator():
    img, cy = lot(70, (110, 100, 90, 255))
    box(img, 19, cy + 1, 12, 6, 15, (80, 80, 85, 255), (140, 130, 120, 255), (105, 98, 90, 255))
    for wx in (10, 14):
        put(img, wx, cy - 5 + (wx - 7) // 2, (255, 150, 60, 255))               # furnace glow
    cylinder(img, 32, cy - 3, 3, 30, (150, 140, 135, 255), (40, 35, 35, 255))
    for (sx, sy, r) in [(33, 8, 3), (36, 4, 4), (40, 1, 3)]:
        for y in range(sy - r, sy + r + 1):
            for x in range(sx - r, sx + r + 1):
                if (x - sx) ** 2 + (y - sy) ** 2 <= r * r:
                    put(img, x, y, (120, 115, 110, 210) if (x + y) % 3 else (95, 90, 88, 210))
    return img


def recycling():
    img, cy = lot(50, CONCRETE)
    box(img, 20, cy + 1, 13, 7, 12, (60, 140, 80, 255), (210, 215, 205, 255), (170, 175, 165, 255))
    for (x, col) in [(31, (60, 140, 220)), (35, (70, 170, 90)), (39, (230, 190, 60))]:   # sorted bins
        box(img, x, cy + 4 - (x - 31) // 3, 2, 1, 4, col + (255,), shade(col + (255,), -10), shade(col + (255,), -40))
    for k in range(3):                                                          # recycling arrows on the roof
        put(img, 18 + 2 * k, cy - 13, (240, 250, 240, 255))
    return img


def bus_sprites():
    """Buses: the van sprites, stretched and repainted yellow."""
    out = {}
    for d in ("ne", "se", "sw", "nw"):
        van = Image.open(os.path.join(OUT, "car_van_%s.png" % d)).convert("RGBA")
        big = van.resize((int(van.width * 1.3), int(van.height * 1.2)), Image.NEAREST)
        px = big.load()
        for y in range(big.height):
            for x in range(big.width):
                r, g, b, a = px[x, y]
                if a == 0:
                    continue
                hi, lo = max(r, g, b), min(r, g, b)
                window = b > r + 15 and b > g
                if not window and hi > 110:                                       # body -> school-bus yellow
                    light = (r + g + b) / 3.0 / 200.0
                    px[x, y] = (min(255, int(245 * light)), min(255, int(190 * light)), min(255, int(40 * light)), a)
        out["bus_%s" % d] = big
    return out


SPRITES = {
    "util_coal": coal, "util_wind": wind, "util_solar": solar, "util_transformer": transformer,
    "util_well": well, "util_pump": pump, "util_treatment": treatment, "util_tower": tower,
    "svc_police": police, "svc_fire": fire, "svc_clinic": clinic, "svc_landfill": landfill,
    "svc_mine": mine, "svc_bus": bus,
    "util_outflow": outflow, "util_sewage": sewage_plant, "svc_incinerator": incinerator, "svc_recycling": recycling,
}


def build_all():
    os.makedirs(OUT, exist_ok=True)
    for name, fn in SPRITES.items():
        fn().save(os.path.join(OUT, name + ".png"))
    for name, img in bus_sprites().items():
        img.save(os.path.join(OUT, name + ".png"))
    print(f"Generated {len(SPRITES)} utility sprites -> {os.path.abspath(OUT)}")


if __name__ == "__main__":
    build_all()
