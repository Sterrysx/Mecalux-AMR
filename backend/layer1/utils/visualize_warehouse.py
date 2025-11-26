#!/usr/bin/env python3
"""
Warehouse Visualizer with POI Overlay
=====================================

Generates an image of the warehouse map with Points of Interest (POIs) marked.
Supports DECIMETERS, CENTIMETERS, and MILLIMETERS resolutions.

POI Types:
- CHARGING (Cx):  Blue circles - Battery charging stations
- PICKUP (Px):    Green squares - Locations where robot picks up packages
- DROPOFF (Px):   Orange squares - Locations where robot drops off packages

The robot footprint and POI marker sizes are calculated dynamically based on
physical dimensions, not hardcoded pixel values.

Usage:
    python visualize_warehouse.py [resolution]
    
    resolution: 'dm' (decimeters), 'cm' (centimeters), 'mm' (millimeters)
                Default: 'dm'

Output:
    warehouse_with_pois.png - Warehouse map with POIs marked
"""

import json
import os
import sys
from PIL import Image, ImageDraw, ImageFont

# Physical constants (in meters)
ROBOT_SIZE_METERS = 0.5      # Robot footprint is 50cm x 50cm
POI_MARKER_METERS = 0.4      # POI marker radius in meters

# Resolution configurations - dynamic sizing based on physical dimensions
RESOLUTIONS = {
    'dm': {
        'name': 'DECIMETERS',
        'pixels_per_meter': 10,   # 1 pixel = 10cm = 0.1m
    },
    'cm': {
        'name': 'CENTIMETERS', 
        'pixels_per_meter': 100,  # 1 pixel = 1cm = 0.01m
    },
    'mm': {
        'name': 'MILLIMETERS',
        'pixels_per_meter': 1000, # 1 pixel = 1mm = 0.001m
    }
}

def get_draw_sizes(res_key):
    """Calculate draw sizes based on resolution using physical logic."""
    ppm = RESOLUTIONS[res_key]['pixels_per_meter']
    
    robot_size_px = int(ROBOT_SIZE_METERS * ppm)
    poi_radius_px = int(POI_MARKER_METERS * ppm)
    
    # Ensure minimum visible sizes
    robot_size_px = max(robot_size_px, 3)
    poi_radius_px = max(poi_radius_px, 4)
    
    # Font size scales with resolution
    font_size = max(8, int(0.3 * ppm))  # 30cm font height
    line_width = max(1, int(0.05 * ppm))  # 5cm line width
    
    return {
        'robot_size_px': robot_size_px,
        'poi_radius': poi_radius_px,
        'font_size': font_size,
        'line_width': line_width,
        'meters_per_pixel': 1.0 / ppm
    }

# Color scheme
COLORS = {
    'wall': (40, 40, 40),           # Dark gray for obstacles
    'floor': (240, 240, 240),       # Light gray for walkable
    'charging': (0, 120, 255),      # Blue for charging stations
    'charging_fill': (100, 180, 255),
    'pickup': (0, 180, 80),         # Green for pickup zones
    'pickup_fill': (100, 220, 150),
    'dropoff': (255, 140, 0),       # Orange for dropoff zones
    'dropoff_fill': (255, 180, 100),
    'inactive': (180, 180, 180),    # Gray for inactive POIs
    'text': (0, 0, 0),              # Black text
    'grid': (200, 200, 200),        # Grid lines
}


def load_map(map_path):
    """Load the bitmap map from text file."""
    with open(map_path, 'r') as f:
        lines = f.readlines()
    
    # First line contains dimensions
    dims = lines[0].strip().split()
    width, height = int(dims[0]), int(dims[1])
    
    # Parse grid data
    grid = []
    for line in lines[1:height+1]:
        row = []
        for char in line.rstrip('\n'):
            if char == '.':
                row.append(True)   # Walkable
            elif char == '#':
                row.append(False)  # Obstacle
        # Pad row if needed
        while len(row) < width:
            row.append(False)
        grid.append(row[:width])
    
    # Pad grid if needed
    while len(grid) < height:
        grid.append([False] * width)
    
    return grid, width, height


def load_pois(poi_path):
    """Load POIs from JSON configuration."""
    with open(poi_path, 'r') as f:
        data = json.load(f)
    return data.get('poi', [])


def load_inflated_map(inflated_path):
    """Load the inflated map if it exists."""
    if not os.path.exists(inflated_path):
        return None
    return load_map(inflated_path)


def create_warehouse_image(grid, width, height, pois, draw_sizes, res_name, inflated_grid=None, show_grid_lines=False):
    """Create the warehouse visualization image."""
    
    # Create base image
    img = Image.new('RGB', (width, height), COLORS['floor'])
    draw = ImageDraw.Draw(img)
    
    # Draw obstacles (walls)
    for y in range(height):
        for x in range(width):
            if not grid[y][x]:
                img.putpixel((x, y), COLORS['wall'])
            elif inflated_grid and not inflated_grid[y][x]:
                # Show inflated area in a slightly different shade
                img.putpixel((x, y), (220, 220, 230))
    
    # Draw grid lines if requested (for small maps)
    if show_grid_lines and width <= 100:
        step = draw_sizes['robot_size_px']
        for x in range(0, width, step):
            draw.line([(x, 0), (x, height)], fill=COLORS['grid'], width=1)
        for y in range(0, height, step):
            draw.line([(0, y), (width, y)], fill=COLORS['grid'], width=1)
    
    # Try to load a font
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 
                                   draw_sizes['font_size'])
    except:
        try:
            font = ImageFont.truetype("arial.ttf", draw_sizes['font_size'])
        except:
            font = ImageFont.load_default()
    
    poi_radius = draw_sizes['poi_radius']
    line_width = draw_sizes['line_width']
    
    # Draw POIs
    for poi in pois:
        x, y = poi['x'], poi['y']
        poi_id = poi['id']
        poi_type = poi.get('type', 'PICKUP').upper()
        is_active = poi.get('active', True)
        
        # Determine colors based on type and active status
        if not is_active:
            fill_color = COLORS['inactive']
            outline_color = (120, 120, 120)
        elif poi_type == 'CHARGING':
            fill_color = COLORS['charging_fill']
            outline_color = COLORS['charging']
        elif poi_type == 'DROPOFF':
            fill_color = COLORS['dropoff_fill']
            outline_color = COLORS['dropoff']
        else:  # PICKUP (default)
            fill_color = COLORS['pickup_fill']
            outline_color = COLORS['pickup']
        
        # Draw the POI marker
        if poi_type == 'CHARGING':
            # Circle for charging stations
            bbox = [x - poi_radius, y - poi_radius, 
                    x + poi_radius, y + poi_radius]
            draw.ellipse(bbox, fill=fill_color, outline=outline_color, width=line_width)
        else:
            # Square for pickup/dropoff zones
            half = poi_radius
            bbox = [x - half, y - half, x + half, y + half]
            draw.rectangle(bbox, fill=fill_color, outline=outline_color, width=line_width)
        
        # Draw the ID text
        text_bbox = draw.textbbox((0, 0), poi_id, font=font)
        text_width = text_bbox[2] - text_bbox[0]
        text_height = text_bbox[3] - text_bbox[1]
        text_x = x - text_width // 2
        text_y = y - text_height // 2
        
        # Draw text with white background for readability
        padding = 2
        draw.rectangle([text_x - padding, text_y - padding, 
                       text_x + text_width + padding, text_y + text_height + padding],
                       fill=(255, 255, 255, 200))
        draw.text((text_x, text_y), poi_id, fill=COLORS['text'], font=font)
    
    return img


def create_legend(draw_sizes):
    """Create a legend image explaining the symbols."""
    legend_width = 220
    legend_height = 180
    
    img = Image.new('RGB', (legend_width, legend_height), (255, 255, 255))
    draw = ImageDraw.Draw(img)
    
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12)
        font_bold = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14)
    except:
        font = ImageFont.load_default()
        font_bold = font
    
    y_offset = 10
    
    # Title
    draw.text((10, y_offset), "Legend", fill=(0, 0, 0), font=font_bold)
    y_offset += 25
    
    # Charging stations
    draw.ellipse([10, y_offset, 26, y_offset + 16], 
                 fill=COLORS['charging_fill'], outline=COLORS['charging'], width=2)
    draw.text((35, y_offset), "Charging (Cx)", fill=(0, 0, 0), font=font)
    y_offset += 25
    
    # Pickup zones
    draw.rectangle([10, y_offset, 26, y_offset + 16],
                   fill=COLORS['pickup_fill'], outline=COLORS['pickup'], width=2)
    draw.text((35, y_offset), "Pickup (Px)", fill=(0, 0, 0), font=font)
    y_offset += 25
    
    # Dropoff zones
    draw.rectangle([10, y_offset, 26, y_offset + 16],
                   fill=COLORS['dropoff_fill'], outline=COLORS['dropoff'], width=2)
    draw.text((35, y_offset), "Dropoff (Px)", fill=(0, 0, 0), font=font)
    y_offset += 25
    
    # Walls
    draw.rectangle([10, y_offset, 26, y_offset + 16], fill=COLORS['wall'])
    draw.text((35, y_offset), "Walls/Obstacles", fill=(0, 0, 0), font=font)
    y_offset += 25
    
    # Inflated area
    draw.rectangle([10, y_offset, 26, y_offset + 16], fill=(220, 220, 230))
    draw.text((35, y_offset), "Safety Margin", fill=(0, 0, 0), font=font)
    
    return img


def main():
    # Parse arguments
    resolution_arg = 'dm'  # Default to decimeters
    if len(sys.argv) > 1:
        resolution_arg = sys.argv[1].lower()
        if resolution_arg not in RESOLUTIONS:
            print(f"Error: Unknown resolution '{resolution_arg}'")
            print(f"Valid options: {', '.join(RESOLUTIONS.keys())}")
            sys.exit(1)
    
    res_config = RESOLUTIONS[resolution_arg]
    draw_sizes = get_draw_sizes(resolution_arg)
    
    # Paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    assets_dir = os.path.join(script_dir, '..', 'assets')
    
    map_path = os.path.join(assets_dir, 'map_layout.txt')
    inflated_map_path = os.path.join(assets_dir, 'inflated_map.txt')
    poi_path = os.path.join(assets_dir, 'poi_config.json')
    output_path = os.path.join(assets_dir, 'warehouse_with_pois.png')
    legend_path = os.path.join(assets_dir, 'legend.png')
    
    print(f"Warehouse POI Visualizer")
    print(f"========================")
    print(f"Resolution: {res_config['name']}")
    print(f"Robot footprint: {draw_sizes['robot_size_px']} pixels ({ROBOT_SIZE_METERS*100:.0f}cm)")
    print(f"POI marker radius: {draw_sizes['poi_radius']} pixels ({POI_MARKER_METERS*100:.0f}cm)")
    print()
    
    # Load data
    print(f"Loading map from: {map_path}")
    grid, width, height = load_map(map_path)
    print(f"  Map size: {width} x {height} pixels")
    print(f"  Physical size: {width / res_config['pixels_per_meter']:.1f}m x {height / res_config['pixels_per_meter']:.1f}m")
    
    # Try to load inflated map
    inflated_grid = None
    if os.path.exists(inflated_map_path):
        print(f"Loading inflated map from: {inflated_map_path}")
        inflated_data = load_map(inflated_map_path)
        if inflated_data:
            inflated_grid = inflated_data[0]
            print(f"  Inflated map loaded successfully")
    
    print(f"Loading POIs from: {poi_path}")
    pois = load_pois(poi_path)
    print(f"  Found {len(pois)} POIs")
    
    # Count by type
    charging_count = sum(1 for p in pois if p.get('type', '').upper() == 'CHARGING')
    pickup_count = sum(1 for p in pois if p.get('type', '').upper() == 'PICKUP')
    dropoff_count = sum(1 for p in pois if p.get('type', '').upper() == 'DROPOFF')
    print(f"    Charging stations: {charging_count}")
    print(f"    Pickup zones: {pickup_count}")
    print(f"    Dropoff zones: {dropoff_count}")
    
    # Create visualization
    print(f"\nGenerating visualization...")
    img = create_warehouse_image(grid, width, height, pois, draw_sizes, 
                                  res_config['name'], inflated_grid, show_grid_lines=False)
    
    # Save output
    img.save(output_path)
    print(f"  Saved: {output_path}")
    
    # Create and save legend
    legend = create_legend(draw_sizes)
    legend.save(legend_path)
    print(f"  Saved: {legend_path}")
    
    print(f"\nDone!")


if __name__ == '__main__':
    main()
