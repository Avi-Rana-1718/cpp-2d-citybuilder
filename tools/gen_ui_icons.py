#!/usr/bin/env python3
"""
UI icon set (16x16 pixel art, drawn 2x in game) for the HUD.

Writes assets/sprites/ui/<name>.png. Icons that get tinted at runtime
(smile, view toggles) are drawn in light greys so a colour multiply works.
"""
import os
from PIL import Image, ImageDraw

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites", "ui")
S = 16
INK = (112, 122, 140, 255)     # outline: light enough to read on dark UI panels
DARK = (20, 22, 28, 255)       # details drawn on light fills (eyes, '!')
WHITE = (245, 247, 250, 255)


def canvas():
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def inspect():
    img, d = canvas()
    d.line((9, 9, 14, 14), fill=(205, 165, 105, 255), width=3)            # wooden handle
    d.ellipse((1, 1, 10, 10), outline=(200, 208, 220, 255), fill=(110, 180, 235, 255), width=2)
    d.ellipse((3, 3, 5, 5), fill=WHITE)
    return img


def residential():
    img, d = canvas()
    d.polygon([(1, 8), (8, 1), (15, 8)], fill=(214, 72, 58, 255), outline=INK)   # roof
    d.rectangle((3, 8, 13, 14), fill=(240, 228, 205, 255), outline=INK)       # walls
    d.rectangle((7, 10, 9, 14), fill=(120, 80, 50, 255))                       # door
    d.rectangle((4, 9, 5, 10), fill=(110, 180, 230, 255))
    d.rectangle((11, 9, 12, 10), fill=(110, 180, 230, 255))
    return img


def commercial():
    img, d = canvas()
    d.rectangle((2, 6, 13, 14), fill=(235, 238, 242, 255), outline=INK)
    for k, x in enumerate(range(1, 15, 3)):                                   # striped awning
        d.rectangle((x, 3, x + 2, 6), fill=(52, 152, 219, 255) if k % 2 == 0 else WHITE)
    d.line((1, 3, 14, 3), fill=INK)
    d.rectangle((4, 9, 7, 14), fill=(120, 190, 235, 255), outline=INK)
    d.rectangle((9, 9, 12, 12), fill=(120, 190, 235, 255), outline=INK)
    return img


def industrial():
    img, d = canvas()
    d.rectangle((11, 1, 13, 7), fill=(150, 150, 155, 255), outline=INK)        # chimney
    d.polygon([(1, 14), (1, 8), (5, 5), (5, 8), (9, 5), (9, 8), (14, 8), (14, 14)],
              fill=(214, 128, 50, 255), outline=INK)                          # sawtooth roof
    d.rectangle((3, 10, 5, 12), fill=(250, 220, 120, 255))
    d.rectangle((8, 10, 10, 12), fill=(250, 220, 120, 255))
    return img


def civic():
    img, d = canvas()
    d.polygon([(1, 5), (8, 1), (15, 5)], fill=(200, 175, 230, 255), outline=INK)   # pediment
    d.rectangle((2, 5, 14, 6), fill=(235, 230, 245, 255), outline=INK)
    for x in (3, 7, 11):                                                           # columns
        d.rectangle((x, 7, x + 2, 12), fill=(235, 230, 245, 255), outline=INK)
    d.rectangle((1, 13, 15, 14), fill=(200, 175, 230, 255), outline=INK)
    return img


def infrastructure():
    img, d = canvas()
    d.polygon([(8, 1), (13, 13), (3, 13)], fill=(240, 130, 40, 255), outline=INK)  # traffic cone
    d.line((6, 7, 10, 7), fill=WHITE, width=2)
    d.rectangle((1, 13, 15, 14), fill=(90, 95, 105, 255), outline=INK)
    return img


def road():
    img, d = canvas()
    d.polygon([(0, 11), (11, 0), (16, 0), (16, 5), (5, 16), (0, 16)], fill=(96, 102, 114, 255))   # asphalt band
    d.line((0, 11, 11, 0), fill=(200, 205, 212, 255))                                           # kerbs
    d.line((5, 16, 16, 5), fill=(200, 205, 212, 255))
    for k in range(3):                                                                           # centre dashes
        x = 2 + k * 5
        d.line((x, 13 - k * 5, x + 2, 11 - k * 5), fill=(250, 210, 60, 255), width=1)
    return img


def bulldoze():
    img, d = canvas()
    d.rectangle((4, 5, 11, 10), fill=(245, 196, 40, 255), outline=INK)       # cab/body
    d.rectangle((6, 3, 10, 5), fill=(160, 210, 240, 255), outline=INK)
    d.rectangle((0, 7, 2, 13), fill=(200, 200, 205, 255), outline=INK)        # blade
    d.rectangle((3, 11, 14, 14), fill=(60, 62, 70, 255), outline=INK)        # tracks
    return img


def coin():
    img, d = canvas()
    d.ellipse((1, 1, 14, 14), fill=(245, 196, 40, 255), outline=(150, 105, 10, 255), width=2)
    d.line((8, 4, 8, 11), fill=(150, 105, 10, 255), width=1)
    d.line((6, 5, 10, 5), fill=(150, 105, 10, 255))
    d.line((6, 8, 10, 8), fill=(150, 105, 10, 255))
    d.line((6, 11, 10, 11), fill=(150, 105, 10, 255))
    return img


def people():
    img, d = canvas()
    d.ellipse((2, 2, 6, 6), fill=(120, 190, 240, 255), outline=INK)
    d.pieslice((0, 7, 8, 17), 180, 360, fill=(120, 190, 240, 255), outline=INK)
    d.ellipse((9, 3, 13, 7), fill=(240, 160, 90, 255), outline=INK)
    d.pieslice((7, 8, 15, 18), 180, 360, fill=(240, 160, 90, 255), outline=INK)
    return img


def smile():
    img, d = canvas()
    d.ellipse((1, 1, 14, 14), fill=(235, 235, 235, 255), outline=INK)       # tinted at runtime
    d.rectangle((5, 5, 6, 6), fill=DARK)
    d.rectangle((9, 5, 10, 6), fill=DARK)
    d.arc((4, 5, 11, 12), 20, 160, fill=DARK, width=1)
    return img


def season_spring():
    img, d = canvas()
    d.line((8, 14, 8, 6), fill=(70, 140, 50, 255), width=2)
    d.ellipse((2, 3, 8, 8), fill=(120, 210, 90, 255), outline=INK)
    d.ellipse((8, 1, 14, 6), fill=(140, 225, 100, 255), outline=INK)
    return img


def season_summer():
    img, d = canvas()
    for x0, y0, x1, y1 in [(8, 0, 8, 2), (8, 13, 8, 15), (0, 8, 2, 8), (13, 8, 15, 8),
                           (2, 2, 3, 3), (12, 12, 13, 13), (12, 3, 13, 2), (2, 13, 3, 12)]:
        d.line((x0, y0, x1, y1), fill=(250, 200, 40, 255), width=1)
    d.ellipse((4, 4, 11, 11), fill=(252, 210, 60, 255), outline=(200, 140, 20, 255))
    return img


def season_autumn():
    img, d = canvas()
    d.polygon([(8, 1), (10, 5), (14, 4), (12, 8), (15, 10), (10, 11), (8, 14),
               (6, 11), (1, 10), (4, 8), (2, 4), (6, 5)], fill=(225, 110, 35, 255), outline=INK)
    d.line((8, 6, 8, 15), fill=(120, 60, 20, 255))
    return img


def season_winter():
    img, d = canvas()
    c = (170, 215, 250, 255)
    d.line((8, 1, 8, 14), fill=c, width=2)
    d.line((2, 4, 14, 11), fill=c, width=2)
    d.line((2, 11, 14, 4), fill=c, width=2)
    d.ellipse((6, 6, 9, 9), fill=WHITE)
    return img


def view_normal():
    img, d = canvas()
    d.polygon([(0, 13), (5, 5), (8, 9), (11, 3), (16, 13)], fill=(200, 205, 210, 255), outline=INK)
    d.polygon([(9, 6), (11, 3), (13, 6)], fill=WHITE)
    return img


def view_fertility():
    img, d = canvas()
    d.line((8, 15, 8, 4), fill=(200, 200, 200, 255), width=1)
    for y in (4, 7, 10):
        d.ellipse((4, y, 8, y + 3), fill=(225, 225, 225, 255), outline=INK)
        d.ellipse((8, y, 12, y + 3), fill=(225, 225, 225, 255), outline=INK)
    return img


def view_water():
    img, d = canvas()
    d.polygon([(8, 1), (13, 9), (3, 9)], fill=(225, 225, 225, 255))
    d.ellipse((3, 5, 13, 15), fill=(225, 225, 225, 255), outline=INK)
    d.ellipse((5, 9, 7, 11), fill=WHITE)
    return img


def view_deposits():
    img, d = canvas()
    d.polygon([(2, 12), (5, 6), (10, 5), (14, 9), (12, 14), (5, 14)],
              fill=(215, 215, 220, 255), outline=INK)
    d.rectangle((5, 8, 7, 10), fill=(60, 60, 65, 255))
    d.rectangle((9, 9, 11, 11), fill=(200, 90, 40, 255))
    return img


def warning():
    img, d = canvas()
    d.polygon([(8, 1), (15, 14), (1, 14)], fill=(245, 196, 40, 255), outline=INK)
    d.rectangle((7, 5, 8, 10), fill=DARK)
    d.rectangle((7, 12, 8, 13), fill=DARK)
    return img


def star():
    img, d = canvas()
    d.polygon([(8, 1), (10, 6), (15, 6), (11, 9), (13, 14), (8, 11), (3, 14), (5, 9), (1, 6), (6, 6)],
              fill=(250, 205, 50, 255), outline=(170, 120, 10, 255))
    return img


def speed_pause():
    img, d = canvas()
    d.rectangle((4, 3, 6, 12), fill=WHITE)
    d.rectangle((9, 3, 11, 12), fill=WHITE)
    return img


def triangles(n):
    def make():
        img, d = canvas()
        w = 6 if n > 1 else 8
        total = w * n - (n - 1) * 2
        x = (S - total) // 2
        for _ in range(n):
            d.polygon([(x, 3), (x + w, 8), (x, 13)], fill=WHITE)
            x += w - 2
        return img
    return make


ICONS = {
    "inspect": inspect, "residential": residential, "commercial": commercial,
    "industrial": industrial, "civic": civic, "infrastructure": infrastructure,
    "bulldoze": bulldoze, "road": road, "coin": coin, "people": people, "smile": smile,
    "season_spring": season_spring, "season_summer": season_summer,
    "season_autumn": season_autumn, "season_winter": season_winter,
    "view_normal": view_normal, "view_fertility": view_fertility,
    "view_water": view_water, "view_deposits": view_deposits,
    "warning": warning, "star": star,
    "speed_pause": speed_pause, "speed_1": triangles(1), "speed_2": triangles(2), "speed_4": triangles(3),
}


def build_all():
    os.makedirs(OUT, exist_ok=True)
    for name, fn in ICONS.items():
        img = fn()
        img.save(os.path.join(OUT, name + ".png"))
    print(f"Generated {len(ICONS)} icons -> {os.path.abspath(OUT)}")


if __name__ == "__main__":
    build_all()
