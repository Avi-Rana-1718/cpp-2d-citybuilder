import os
import shutil
from PIL import Image

out_dir = 'extracted_theotown'
sprites_dir = 'assets/sprites'

mapping = {
    # 1. Residential
    'res_t1_v1': 'world_1024_0_005_63x43.png',    # Suburban Cottages v1
    'res_t1_v2': 'world_1024_0_011_63x43.png',    # Bungalow Homestead v2
    'res_t2_v1': 'world_1024_0_018_63x49.png',    # Brick Brownstone Apartments v1
    'res_t2_v2': 'world_1024_0_025_63x55.png',    # Urban Condominiums v2
    'res_t3_v1': 'world_1024_0_046_95x103.png',   # Highrise Luxury Tower v1
    'res_t3_v2': 'world_1024_0_073_95x129.png',   # Twin Highrise Plaza v2
    
    # 2. Commercial
    'com_t1_v1': 'world_0_1024_013_63x43.png',    # Corner Grocery Store v1
    'com_t1_v2': 'world_1024_1024_008_63x43.png', # Local Diner & Mini Mart v2
    'com_t2_v1': 'world_0_1024_000_63x63.png',    # Shopping Plaza Arcade v1
    'com_t2_v2': 'world_1024_1024_002_63x47.png', # Department Store v2
    'com_t3_v1': 'world_1024_0_048_95x131.png',   # Corporate Banking Highrise v1
    'com_t3_v2': 'world_1024_0_055_95x147.png',   # International Financial Center v2
    
    # 3. Industrial
    'ind_t1_v1': 'world_1024_1024_046_61x55.png', # Scrap Workshop & Depot v1
    'ind_t1_v2': 'world_0_1024_019_63x55.png',    # Warehouse & Loading Dock v2
    'ind_t2_v1': 'world_1024_1024_038_63x47.png', # Manufacturing Plant v1
    'ind_t2_v2': 'world_1024_1024_044_85x73.png', # Heavy Assembly Facility v2
    'ind_t3_v1': 'world_1024_0_045_95x61.png',    # Microchip Clean-Fab v1
    'ind_t3_v2': 'world_0_1024_007_95x85.png',    # Robotics Automation Hub v2
    
    # 4. Agricultural Farms
    'farm_v1':   'world_1024_1024_036_95x77.png', # Red Barn & Grain Silo v1
    'farm_v2':   'world_0_1024_006_63x63.png',    # Old Dutch Windmill Homestead v2
    
    # 5. Civics & Maritime
    'church':    'world_1024_1024_034_63x91.png', # Gothic Cathedral with Bell Tower Steeple
    'school':    'world_1024_1024_016_95x79.png', # Elementary School & Courtyard
    'park':      'world_1024_1024_042_105x81.png',# Grand Nature Park with Trees & Pathway
    'harbour':   'world_0_1024_033_85x57.png',    # Shoreline Harbour Port with Cranes & Pier
}

# Aliases
mapping['residental'] = mapping['res_t1_v1']
mapping['commercial'] = mapping['com_t1_v1']
mapping['industrial'] = mapping['ind_t1_v1']
mapping['farm']       = mapping['farm_v1']

print(f"Installing {len(mapping)} genuine TheoTown building sprites to {sprites_dir}...")

for key, fname in mapping.items():
    src_path = os.path.join(out_dir, fname)
    if not os.path.exists(src_path):
        print(f"ERROR: missing source file {src_path}")
        continue
    src = Image.open(src_path).convert('RGBA')
    w = 48
    h = int(round(src.height * (48.0 / src.width)))
    resized = src.resize((w, h), Image.LANCZOS)
    
    final_h = max(44, h)
    canvas = Image.new('RGBA', (48, final_h), (0, 0, 0, 0))
    # Bottom align
    canvas.paste(resized, (0, final_h - h))
    
    dst_path = os.path.join(sprites_dir, f"{key}.png")
    canvas.save(dst_path)
    print(f"  Installed {key}.png (src: {fname}, size: {w}x{final_h})")

# Also copy to build/assets/sprites if build directory exists
build_sprites_dir = 'build/assets/sprites'
if os.path.exists(build_sprites_dir):
    for f in os.listdir(sprites_dir):
        if f.endswith('.png'):
            shutil.copy2(os.path.join(sprites_dir, f), os.path.join(build_sprites_dir, f))
    print(f"Synchronized sprites to {build_sprites_dir}")

print("Installation complete!")
