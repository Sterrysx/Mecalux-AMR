#!/usr/bin/env python3
"""
NavMesh Graph Visualizer
========================

Generates an image of the navigation mesh graph overlaid on the warehouse map.
Shows nodes, edges, and POI mappings to validate the graph structure.

Supports DECIMETERS, CENTIMETERS, and MILLIMETERS resolutions.
All sizes are calculated dynamically based on physical dimensions.

POI Types:
- CHARGING (Cx):  Blue crosshairs - Battery charging stations
- PICKUP (Px):    Green crosshairs - Pickup locations
- DROPOFF (Px):   Orange crosshairs - Dropoff locations

Usage:
    python visualize_navmesh.py [resolution]
    
    resolution: 'dm' (decimeters), 'cm' (centimeters), 'mm' (millimeters)
                Default: 'dm'

Output:
    navmesh_graph.png - NavMesh graph visualization
"""

import json
import os
import sys
import csv
from PIL import Image, ImageDraw, ImageFont
import math

# Physical constants (in meters)
ROBOT_SIZE_METERS = 0.5      # Robot footprint is 50cm x 50cm
NODE_SIZE_METERS = 0.15      # Node marker radius
POI_MARKER_METERS = 0.35     # POI crosshair size

# Resolution configurations
RESOLUTIONS = {
    'dm': {
        'name': 'DECIMETERS',
        'pixels_per_meter': 10,   # 1 pixel = 10cm
    },
    'cm': {
        'name': 'CENTIMETERS',
        'pixels_per_meter': 100,  # 1 pixel = 1cm
    },
    'mm': {
        'name': 'MILLIMETERS',
        'pixels_per_meter': 1000, # 1 pixel = 1mm
    }
}

def get_draw_sizes(res_key):
    """Calculate draw sizes based on resolution using physical logic."""
    ppm = RESOLUTIONS[res_key]['pixels_per_meter']
    
    node_radius = int(NODE_SIZE_METERS * ppm)
    poi_radius = int(POI_MARKER_METERS * ppm)
    edge_width = max(1, int(0.03 * ppm))  # 3cm edge width
    font_size = max(6, int(0.25 * ppm))   # 25cm font height
    
    # Ensure minimum visible sizes
    node_radius = max(node_radius, 2)
    poi_radius = max(poi_radius, 4)
    
    return {
        'node_radius': node_radius,
        'poi_radius': poi_radius,
        'edge_width': edge_width,
        'font_size': font_size,
        'meters_per_pixel': 1.0 / ppm
    }

# Color scheme
COLORS = {
    'wall': (60, 60, 60),
    'floor': (245, 245, 245),
    'inflated': (230, 230, 240),
    'node': (100, 100, 200),
    'node_outline': (50, 50, 150),
    'edge': (180, 180, 220),
    'poi_charging': (0, 120, 255),
    'poi_pickup': (0, 180, 80),
    'poi_dropoff': (255, 140, 0),
    'poi_node': (255, 100, 100),  # Nodes with POIs
    'text': (0, 0, 0)
}


def load_map(map_path):
    """Load the bitmap map from text file."""
    with open(map_path, 'r') as f:
        lines = f.readlines()
    
    dims = lines[0].strip().split()
    width, height = int(dims[0]), int(dims[1])
    
    grid = []
    for line in lines[1:height+1]:
        row = []
        for char in line.rstrip('\n'):
            row.append(char == '.')
        while len(row) < width:
            row.append(False)
        grid.append(row[:width])
    
    while len(grid) < height:
        grid.append([False] * width)
    
    return grid, width, height


def load_graph(graph_path):
    """Load the NavMesh graph from CSV file."""
    nodes = []
    edges = []
    
    with open(graph_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            parts = line.split(',')
            if len(parts) < 3:
                continue
            
            node_id = int(parts[0].strip())
            x = int(parts[1].strip())
            y = int(parts[2].strip())
            
            nodes.append({'id': node_id, 'x': x, 'y': y})
            
            # Parse neighbors if present
            if len(parts) > 3 and parts[3].strip():
                neighbors_str = parts[3].strip()
                for neighbor in neighbors_str.split('|'):
                    if ':' in neighbor:
                        target_id, cost = neighbor.split(':')
                        target_id = int(target_id)
                        cost = float(cost)
                        # Only add edge once (from lower to higher ID)
                        if node_id < target_id:
                            edges.append({
                                'from': node_id,
                                'to': target_id,
                                'cost': cost
                            })
    
    return nodes, edges


def load_pois(poi_path):
    """Load POIs from JSON configuration."""
    with open(poi_path, 'r') as f:
        data = json.load(f)
    return data.get('poi', [])


def load_poi_registry(registry_path):
    """Load POI registry with node mappings."""
    if not os.path.exists(registry_path):
        return None
    with open(registry_path, 'r') as f:
        data = json.load(f)
    return data.get('poi', [])


def create_graph_image(grid, width, height, nodes, edges, pois, poi_registry, draw_sizes):
    """Create the NavMesh graph visualization."""
    
    # Create base image
    img = Image.new('RGB', (width, height), COLORS['floor'])
    draw = ImageDraw.Draw(img)
    
    # Draw obstacles
    for y in range(height):
        for x in range(width):
            if not grid[y][x]:
                img.putpixel((x, y), COLORS['wall'])
    
    # Create node lookup by ID
    node_by_id = {n['id']: n for n in nodes}
    
    # Build set of POI nodes for highlighting
    poi_nodes = set()
    if poi_registry:
        for poi in poi_registry:
            node_id = poi.get('nearestNodeId', -1)
            if node_id >= 0:
                poi_nodes.add(node_id)
    
    # Draw edges first (so nodes are on top)
    for edge in edges:
        from_node = node_by_id.get(edge['from'])
        to_node = node_by_id.get(edge['to'])
        if from_node and to_node:
            draw.line(
                [(from_node['x'], from_node['y']), 
                 (to_node['x'], to_node['y'])],
                fill=COLORS['edge'],
                width=draw_sizes['edge_width']
            )
    
    # Draw nodes
    node_radius = draw_sizes['node_radius']
    for node in nodes:
        x, y = node['x'], node['y']
        bbox = [x - node_radius, y - node_radius, 
                x + node_radius, y + node_radius]
        
        # Highlight POI nodes differently
        if node['id'] in poi_nodes:
            draw.ellipse(bbox, fill=COLORS['poi_node'], 
                        outline=COLORS['node_outline'], width=1)
        else:
            draw.ellipse(bbox, fill=COLORS['node'], 
                        outline=COLORS['node_outline'], width=1)
    
    # Draw POI markers
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 
                                   draw_sizes['font_size'])
    except:
        font = ImageFont.load_default()
    
    poi_radius = draw_sizes['poi_radius']
    line_width = max(1, draw_sizes['edge_width'])
    
    for poi in pois:
        x, y = poi['x'], poi['y']
        poi_id = poi['id']
        poi_type = poi.get('type', 'PICKUP').upper()
        
        if poi_type == 'CHARGING':
            color = COLORS['poi_charging']
        elif poi_type == 'DROPOFF':
            color = COLORS['poi_dropoff']
        else:  # PICKUP
            color = COLORS['poi_pickup']
        
        # Draw crosshair at POI location
        draw.line([(x - poi_radius, y), (x + poi_radius, y)], fill=color, width=line_width)
        draw.line([(x, y - poi_radius), (x, y + poi_radius)], fill=color, width=line_width)
        
        # Draw label
        text_bbox = draw.textbbox((0, 0), poi_id, font=font)
        text_width = text_bbox[2] - text_bbox[0]
        draw.text((x - text_width // 2, y + poi_radius + 2), poi_id, fill=color, font=font)
    
    return img


def print_graph_stats(nodes, edges, pois, poi_registry):
    """Print statistics about the graph."""
    print(f"\nGraph Statistics:")
    print(f"  Total nodes: {len(nodes)}")
    print(f"  Total edges: {len(edges)}")
    
    if nodes:
        # Calculate average connectivity
        edge_count = {}
        for edge in edges:
            edge_count[edge['from']] = edge_count.get(edge['from'], 0) + 1
            edge_count[edge['to']] = edge_count.get(edge['to'], 0) + 1
        
        avg_edges = sum(edge_count.values()) / len(nodes) if nodes else 0
        max_edges = max(edge_count.values()) if edge_count else 0
        min_edges = min(edge_count.values()) if edge_count else 0
        
        print(f"  Average edges per node: {avg_edges:.2f}")
        print(f"  Max edges on single node: {max_edges}")
        print(f"  Min edges on single node: {min_edges}")
        
        # Check for orphan nodes
        orphans = [n['id'] for n in nodes if n['id'] not in edge_count]
        if orphans:
            print(f"  Warning: {len(orphans)} orphan nodes (no edges)")
    
    print(f"\nPOI Mapping:")
    print(f"  Total POIs: {len(pois)}")
    
    if poi_registry:
        mapped = sum(1 for p in poi_registry if p.get('nearestNodeId', -1) >= 0)
        print(f"  Mapped to nodes: {mapped}")
        
        charging = [p for p in poi_registry if p.get('type', '').upper() == 'CHARGING']
        pickup = [p for p in poi_registry if p.get('type', '').upper() == 'PICKUP']
        dropoff = [p for p in poi_registry if p.get('type', '').upper() == 'DROPOFF']
        
        print(f"    Charging (Cx): {len(charging)}")
        print(f"    Pickup (Px): {len(pickup)}")
        print(f"    Dropoff (Px): {len(dropoff)}")
        
        # Show distance stats
        distances = [p.get('distanceToNode', 0) for p in poi_registry if p.get('nearestNodeId', -1) >= 0]
        if distances:
            print(f"  Avg distance to node: {sum(distances)/len(distances):.2f} px")
            print(f"  Max distance to node: {max(distances):.2f} px")


def create_combined_image(warehouse_img, graph_img, legend_img=None):
    """Combine warehouse and graph images side by side."""
    width = warehouse_img.width + graph_img.width + 20
    height = max(warehouse_img.height, graph_img.height)
    
    if legend_img:
        height = max(height, legend_img.height)
        width += legend_img.width + 10
    
    combined = Image.new('RGB', (width, height), (255, 255, 255))
    combined.paste(warehouse_img, (0, 0))
    combined.paste(graph_img, (warehouse_img.width + 10, 0))
    
    if legend_img:
        combined.paste(legend_img, (warehouse_img.width + graph_img.width + 20, 0))
    
    return combined


def main():
    # Parse arguments
    resolution_arg = 'dm'
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
    graph_path = os.path.join(assets_dir, 'graph_dump.csv')
    poi_path = os.path.join(assets_dir, 'poi_config.json')
    registry_path = os.path.join(assets_dir, 'poi_registry_export.json')
    output_path = os.path.join(assets_dir, 'navmesh_graph.png')
    
    print(f"NavMesh Graph Visualizer")
    print(f"========================")
    print(f"Resolution: {res_config['name']}")
    print(f"Node radius: {draw_sizes['node_radius']} pixels ({NODE_SIZE_METERS*100:.0f}cm)")
    print(f"POI marker: {draw_sizes['poi_radius']} pixels ({POI_MARKER_METERS*100:.0f}cm)")
    print()
    
    # Load data
    print(f"Loading map from: {map_path}")
    grid, width, height = load_map(map_path)
    print(f"  Map size: {width} x {height} pixels")
    
    print(f"Loading graph from: {graph_path}")
    if not os.path.exists(graph_path):
        print(f"  Error: Graph file not found. Run the C++ test first to generate it.")
        sys.exit(1)
    nodes, edges = load_graph(graph_path)
    print(f"  Loaded {len(nodes)} nodes, {len(edges)} edges")
    
    print(f"Loading POIs from: {poi_path}")
    pois = load_pois(poi_path)
    print(f"  Found {len(pois)} POIs")
    
    poi_registry = None
    if os.path.exists(registry_path):
        print(f"Loading POI registry from: {registry_path}")
        poi_registry = load_poi_registry(registry_path)
        print(f"  Found {len(poi_registry) if poi_registry else 0} mapped POIs")
    
    # Print statistics
    print_graph_stats(nodes, edges, pois, poi_registry)
    
    # Create visualization
    print(f"\nGenerating visualization...")
    img = create_graph_image(grid, width, height, nodes, edges, pois, poi_registry, draw_sizes)
    
    # Save output
    img.save(output_path)
    print(f"  Saved: {output_path}")
    
    print(f"\nDone!")


if __name__ == '__main__':
    main()
