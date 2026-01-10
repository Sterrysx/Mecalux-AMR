#!/usr/bin/env python3
"""
Generate a warehouse floor texture with concrete appearance and grid lines
"""
from PIL import Image, ImageDraw
import random
import math

def generate_warehouse_floor(size=512, tile_size=64):
    """
    Generate a warehouse floor texture
    
    Args:
        size: Image size (square)
        tile_size: Size of each floor tile in pixels
    """
    # Create base concrete color with noise
    img = Image.new('RGB', (size, size))
    pixels = img.load()
    
    # Generate concrete texture with noise
    for y in range(size):
        for x in range(size):
            # Base concrete gray color
            base_color = 160
            
            # Add random noise for concrete texture
            noise = random.randint(-15, 15)
            
            # Add subtle pattern based on position
            pattern = int(10 * math.sin(x * 0.1) * math.sin(y * 0.1))
            
            # Combine
            color = max(0, min(255, base_color + noise + pattern))
            
            # Make it slightly warmer (add a bit of brown)
            r = min(255, color + 5)
            g = color
            b = max(0, color - 5)
            
            pixels[x, y] = (r, g, b)
    
    # Add grid lines (warehouse floor tiles)
    draw = ImageDraw.Draw(img)
    
    # Draw vertical and horizontal lines
    for i in range(0, size, tile_size):
        # Vertical lines
        draw.line([(i, 0), (i, size)], fill=(100, 100, 100), width=2)
        # Horizontal lines
        draw.line([(0, i), (size, i)], fill=(100, 100, 100), width=2)
    
    # Add thicker lines every 4 tiles (warehouse sections)
    section_size = tile_size * 4
    for i in range(0, size, section_size):
        draw.line([(i, 0), (i, size)], fill=(80, 80, 80), width=4)
        draw.line([(0, i), (size, i)], fill=(80, 80, 80), width=4)
    
    # Add some wear and tear (darker spots)
    for _ in range(20):
        x = random.randint(0, size-1)
        y = random.randint(0, size-1)
        radius = random.randint(5, 20)
        draw.ellipse([x-radius, y-radius, x+radius, y+radius], 
                     fill=(120, 120, 120), outline=None)
    
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
