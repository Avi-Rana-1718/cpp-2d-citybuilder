#!/usr/bin/env python3
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
    canvas_h = 32 * 20 + 16 * 20 + 64 + 100
    canvas = Image.new("RGBA", (canvas_w, canvas_h), (15, 20, 25, 255))
    
    # 20x20 grid test world
    grid = [["grass" for _ in range(20)] for _ in range(20)]
    
    # 1. Road Network
    for x in range(2, 17):
        grid[x][8] = "road"
    for y in range(2, 16):
        grid[9][y] = "road"
    for x in range(3, 10):
        grid[x][4] = "road"
    for x in range(9, 17):
        grid[x][4] = "road"
    for y in range(9, 15):
        grid[3][y] = "road"
        grid[15][y] = "road"
    for x in range(3, 16):
        grid[x][14] = "road"
        
    # 2. Residential neighborhood (NW)
    res_coords = [
        (4, 2), (5, 2), (6, 2), (7, 2),
        (4, 3), (5, 3), (6, 3), (7, 3),
        (4, 5), (5, 5), (6, 5),
        (4, 6), (5, 6), (6, 6),
        (4, 7), (5, 7)
    ]
    for rx, ry in res_coords:
        grid[rx][ry] = "residental"
            
    # 3. Civic Center & Parks
    grid[8][7] = "school"
    grid[7][7] = "church"
    grid[7][5] = "park"
    grid[8][5] = "park"
    grid[14][6] = "park"
            
    # 4. Commercial Downtown (NE)
    com_coords = [
        (10, 2), (11, 2), (12, 2), (13, 2), (14, 2),
        (10, 3), (11, 3), (12, 3), (13, 3), (14, 3),
        (10, 5), (11, 5), (12, 5), (13, 5),
        (10, 6), (11, 6), (12, 6),
        (10, 7), (11, 7)
    ]
    for cx, cy in com_coords:
        grid[cx][cy] = "commercial"
            
    # 5. Agricultural Farmlands (SW)
    for x in range(4, 9):
        for y in range(9, 14):
            grid[x][y] = "farm"
            
    # 6. Industrial Manufacturing District (SE)
    for x in range(10, 15):
        for y in range(9, 14):
            grid[x][y] = "industrial"
            
    # 7. Water Basin & Harbours (South shoreline)
    for x in range(9, 19):
        for y in range(16, 19):
            grid[x][y] = "water"
    for x in range(11, 17):
        grid[x][15] = "water"
    grid[12][14] = "harbour"
    grid[13][14] = "harbour"
    
    # Render all tiles in standard isometric order (i then j)
    for i in range(20):
        for j in range(20):
            px = 32 * i + 50
            py = 32 * j + 16 * i + 50
            tname = grid[i][j]
            sp = scaled[tname]
            
            # Ground diamond height is 32 (16 unscaled).
            # If sprite is taller, origin_y = (sp.height - 32)
            offset_y = sp.height - 32
            draw_y = py - offset_y
            
            # Draw grass under building if transparent
            if tname not in ["grass", "water"]:
                canvas.alpha_composite(scaled["grass"], (px, py))
                
            canvas.alpha_composite(sp, (px, draw_y))
            
    canvas.save("tools/current_world.png")
    print("Successfully rendered beautiful test world to tools/current_world.png!")

if __name__ == "__main__":
    render_preview()
