#!/usr/bin/env python3
"""
Generate a warehouse floor texture with concrete appearance and grid lines
"""
from PIL import Image, ImageDraw
import random
import math

def generate_warehouse_floor(size=512, tile_size=64):
    """
    Generate a warehouse floor texture with tiled concrete
    
    Args:
        size: Image size (square)
        tile_size: Size of each concrete tile in pixels
    """
    # Create base concrete color with noise
    img = Image.new('RGB', (size, size))
    pixels = img.load()
    
    # Generate concrete texture with varied noise per tile
    for tile_y in range(0, size, tile_size):
        for tile_x in range(0, size, tile_size):
            # Each tile has its own random seed for variation
            tile_base = random.randint(150, 170)
            
            # Fill the tile
            for y in range(tile_y, min(tile_y + tile_size, size)):
                for x in range(tile_x, min(tile_x + tile_size, size)):
                    # Add noise within the tile
                    noise = random.randint(-20, 20)
                    
                    # Add subtle diagonal pattern
                    pattern = int(5 * math.sin((x - tile_x) * 0.2) * math.cos((y - tile_y) * 0.2))
                    
                    # Combine
                    color = max(100, min(255, tile_base + noise + pattern))
                    
                    # Slightly warm gray (concrete)
                    r = min(255, color + 3)
                    g = color
                    b = max(0, color - 3)
                    
                    pixels[x, y] = (r, g, b)
    
    # Add grout lines between tiles
    draw = ImageDraw.Draw(img)
    
    # Draw grout lines (darker gray)
    grout_color = (90, 90, 90)
    for i in range(0, size, tile_size):
        # Vertical grout lines
        draw.line([(i, 0), (i, size)], fill=grout_color, width=3)
        draw.line([(i+1, 0), (i+1, size)], fill=(100, 100, 100), width=1)
        # Horizontal grout lines
        draw.line([(0, i), (size, i)], fill=grout_color, width=3)
        draw.line([(0, i+1), (size, i+1)], fill=(100, 100, 100), width=1)
    
    # Add some concrete imperfections (small cracks, spots)
    for _ in range(30):
        x = random.randint(0, size-1)
        y = random.randint(0, size-1)
        radius = random.randint(2, 8)
        darkness = random.randint(110, 140)
        draw.ellipse([x-radius, y-radius, x+radius, y+radius], 
                     fill=(darkness, darkness, darkness), outline=None)
    
    # Add thin random cracks in some tiles
    for _ in range(10):
        x1 = random.randint(0, size)
        y1 = random.randint(0, size)
        x2 = x1 + random.randint(-20, 20)
        y2 = y1 + random.randint(-20, 20)
        draw.line([(x1, y1), (x2, y2)], fill=(110, 110, 110), width=1)
    
    return img

if __name__ == '__main__':
    print("Generating warehouse floor texture...")
    texture = generate_warehouse_floor(size=512, tile_size=64)
    texture.save('warehouse_floor.png')
    print("Texture saved as warehouse_floor.png")
    
    # Also create a higher resolution version
    print("Generating high-resolution texture...")
    texture_hires = generate_warehouse_floor(size=1024, tile_size=128)
    texture_hires.save('warehouse_floor_hires.png')
    print("High-res texture saved as warehouse_floor_hires.png")
