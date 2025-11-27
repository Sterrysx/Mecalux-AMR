#!/usr/bin/env python3
"""
Warehouse Setup Script for Mecalux AMR

Interactive script to configure a new warehouse from an image:
1. Load warehouse image (PNG/JPG)
2. Convert to binary obstacle map
3. Configure physical dimensions (meters)
4. Generate POI clusters (charging, pickup, dropoff)
5. Generate random tasks
6. Update Layer 2 robot count

Usage:
    python warehouse_setup.py
    python warehouse_setup.py --image warehouse2.png --non-interactive
"""

import argparse
import json
import math
import random
import sys
from pathlib import Path
from typing import List, Tuple, Dict, Optional
from dataclasses import dataclass

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("[WARNING] PIL/Pillow not installed. Install with: pip install Pillow")


@dataclass
class WarehouseConfig:
    """Configuration for warehouse setup."""
    image_path: str = ""
    width_meters: float = 30.0
    height_meters: float = 20.0
    resolution: str = "DECIMETERS"  # DECIMETERS, CENTIMETERS, or MILLIMETERS
    
    # Charging zones
    num_charging: int = 6
    charging_clusters: int = 2
    
    # Pickup zones
    num_pickup: int = 16
    pickup_clusters: int = 4
    
    # Dropoff zones
    num_dropoff: int = 16
    dropoff_clusters: int = 4
    
    # Robots and tasks
    num_robots: int = 3
    num_tasks: int = 50
    
    # Robot parameters
    robot_radius_m: float = 0.3
    
    # Random seed
    seed: Optional[int] = None


def get_resolution_factor(resolution: str) -> float:
    """Get meters per pixel for resolution."""
    if resolution.upper() == "DECIMETERS":
        return 0.1
    elif resolution.upper() == "CENTIMETERS":
        return 0.01
    elif resolution.upper() == "MILLIMETERS":
        return 0.001
    else:
        return 0.1


def image_to_map(image_path: str, output_width: int, output_height: int) -> List[List[bool]]:
    """
    Convert image to binary obstacle map.
    White/light = walkable (True), Black/dark = obstacle (False)
    """
    if not HAS_PIL:
        raise RuntimeError("PIL/Pillow required for image processing")
    
    img = Image.open(image_path)
    
    # Convert to grayscale
    img = img.convert('L')
    
    # Resize to target dimensions
    img = img.resize((output_width, output_height), Image.Resampling.LANCZOS)
    
    # Convert to binary (threshold at 128)
    threshold = 128
    binary_map = []
    
    for y in range(output_height):
        row = []
        for x in range(output_width):
            pixel = img.getpixel((x, y))
            row.append(pixel > threshold)  # True = walkable (white), False = obstacle (black)
        binary_map.append(row)
    
    return binary_map


def save_map_layout(binary_map: List[List[bool]], output_path: str):
    """Save binary map as text file."""
    with open(output_path, 'w') as f:
        for row in binary_map:
            line = ''.join('.' if cell else '#' for cell in row)
            f.write(line + '\n')
    
    height = len(binary_map)
    width = len(binary_map[0]) if binary_map else 0
    walkable = sum(1 for row in binary_map for cell in row if cell)
    total = width * height
    
    print(f"[MapConverter] Saved {width}x{height} map to: {output_path}")
    print(f"[MapConverter] Walkable: {walkable}/{total} ({100*walkable/total:.1f}%)")


def inflate_map(binary_map: List[List[bool]], radius_pixels: int) -> List[List[bool]]:
    """Inflate obstacles by robot radius."""
    height = len(binary_map)
    width = len(binary_map[0]) if binary_map else 0
    
    inflated = [row.copy() for row in binary_map]
    
    for y in range(height):
        for x in range(width):
            if not binary_map[y][x]:  # Obstacle
                for dy in range(-radius_pixels, radius_pixels + 1):
                    for dx in range(-radius_pixels, radius_pixels + 1):
                        if dx*dx + dy*dy <= radius_pixels * radius_pixels:
                            ny, nx = y + dy, x + dx
                            if 0 <= ny < height and 0 <= nx < width:
                                inflated[ny][nx] = False
    
    return inflated


def get_accessible_cells(inflated_map: List[List[bool]], margin: int = 5) -> List[Tuple[int, int]]:
    """Get list of accessible cells (avoiding edges)."""
    height = len(inflated_map)
    width = len(inflated_map[0]) if inflated_map else 0
    
    cells = []
    for y in range(margin, height - margin):
        for x in range(margin, width - margin):
            if inflated_map[y][x]:
                cells.append((x, y))
    
    return cells


def generate_clustered_pois(
    accessible_cells: List[Tuple[int, int]],
    num_pois: int,
    num_clusters: int,
    poi_type: str,
    id_prefix: str,
    min_cluster_dist: int = 30,
    min_poi_dist: int = 8,
    used_positions: set = None,
    seed: int = None
) -> List[Dict]:
    """Generate clustered POIs."""
    if seed is not None:
        random.seed(seed)
    
    if used_positions is None:
        used_positions = set()
    
    pois_per_cluster = max(1, num_pois // num_clusters)
    remaining = num_pois - (pois_per_cluster * num_clusters)
    
    pois = []
    cluster_centers = []
    poi_count = 0
    
    # Shuffle accessible cells
    cells = accessible_cells.copy()
    random.shuffle(cells)
    
    def distance(p1, p2):
        return math.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2)
    
    def is_valid_cluster_center(pos):
        for center in cluster_centers:
            if distance(pos, center) < min_cluster_dist:
                return False
        return True
    
    def is_valid_poi_pos(pos):
        if pos in used_positions:
            return False
        for existing in used_positions:
            if distance(pos, existing) < min_poi_dist:
                return False
        return True
    
    # Find cluster centers
    for cell in cells:
        if len(cluster_centers) >= num_clusters:
            break
        if is_valid_cluster_center(cell):
            cluster_centers.append(cell)
    
    if len(cluster_centers) < num_clusters:
        print(f"[WARNING] Could only create {len(cluster_centers)}/{num_clusters} clusters for {poi_type}")
    
    # Populate each cluster
    for i, center in enumerate(cluster_centers):
        cluster_pois = pois_per_cluster
        if i < remaining:
            cluster_pois += 1
        
        # Find cells near this cluster center
        nearby = [(x, y) for (x, y) in cells 
                  if distance((x, y), center) < min_cluster_dist // 2 + 10]
        random.shuffle(nearby)
        
        added = 0
        for pos in nearby:
            if added >= cluster_pois:
                break
            if is_valid_poi_pos(pos):
                pois.append({
                    "id": f"{id_prefix}{poi_count}",
                    "type": poi_type,
                    "x": pos[0],
                    "y": pos[1],
                    "active": True,
                    "metadata": {
                        "cluster_center": f"({center[0]}, {center[1]})",
                        "cluster_id": i,
                        "generated": True
                    }
                })
                used_positions.add(pos)
                poi_count += 1
                added += 1
    
    print(f"[POIGenerator] Created {len(pois)} {poi_type} POIs in {len(cluster_centers)} clusters")
    return pois


def generate_poi_config(
    config: WarehouseConfig,
    accessible_cells: List[Tuple[int, int]]
) -> Dict:
    """Generate complete POI configuration."""
    used_positions = set()
    
    # Seed for reproducibility
    seed_base = config.seed if config.seed is not None else random.randint(0, 10000)
    
    # Generate charging zones
    charging_pois = generate_clustered_pois(
        accessible_cells,
        config.num_charging,
        config.charging_clusters,
        "CHARGING",
        "C",
        min_cluster_dist=40,
        min_poi_dist=10,
        used_positions=used_positions,
        seed=seed_base
    )
    
    # Generate pickup zones
    pickup_pois = generate_clustered_pois(
        accessible_cells,
        config.num_pickup,
        config.pickup_clusters,
        "PICKUP",
        "PU",
        min_cluster_dist=35,
        min_poi_dist=8,
        used_positions=used_positions,
        seed=seed_base + 1
    )
    
    # Generate dropoff zones
    dropoff_pois = generate_clustered_pois(
        accessible_cells,
        config.num_dropoff,
        config.dropoff_clusters,
        "DROPOFF",
        "DO",
        min_cluster_dist=35,
        min_poi_dist=8,
        used_positions=used_positions,
        seed=seed_base + 2
    )
    
    resolution_factor = get_resolution_factor(config.resolution)
    
    return {
        "description": f"Auto-generated POI configuration for {Path(config.image_path).name}",
        "version": "3.1",
        "coordinate_system": f"pixels ({resolution_factor}m/pixel, {config.resolution})",
        "physical_dimensions": {
            "width_m": config.width_meters,
            "height_m": config.height_meters
        },
        "generation_config": {
            "robot_radius_m": config.robot_radius_m,
            "seed": config.seed
        },
        "poi_types": {
            "CHARGING": "Battery charging stations (IDs: C0, C1, C2...)",
            "PICKUP": "Locations where robot picks up packages (PU0, PU1...)",
            "DROPOFF": "Locations where robot drops off packages (DO0, DO1...)"
        },
        "poi": charging_pois + pickup_pois + dropoff_pois
    }


def generate_tasks(poi_config: Dict, num_tasks: int, seed: int = None) -> Dict:
    """Generate random tasks from POI config."""
    if seed is not None:
        random.seed(seed)
    
    pickup_ids = [p["id"] for p in poi_config["poi"] if p["type"] == "PICKUP"]
    dropoff_ids = [p["id"] for p in poi_config["poi"] if p["type"] == "DROPOFF"]
    
    if not pickup_ids or not dropoff_ids:
        raise ValueError("Need at least one pickup and one dropoff POI")
    
    tasks = []
    for i in range(1, num_tasks + 1):
        tasks.append({
            "id": i,
            "source": random.choice(pickup_ids),
            "destination": random.choice(dropoff_ids)
        })
    
    print(f"[TaskGenerator] Generated {len(tasks)} tasks")
    return {"tasks": tasks}


def update_layer2_robots(config: WarehouseConfig, poi_config: Dict, layer2_main_path: str):
    """Update Layer 2 main.cc with robot count."""
    # Get charging node count
    charging_count = len([p for p in poi_config["poi"] if p["type"] == "CHARGING"])
    actual_robots = min(config.num_robots, charging_count)
    
    if actual_robots < config.num_robots:
        print(f"[WARNING] Only {charging_count} charging stations, reducing robots to {actual_robots}")
    
    try:
        with open(layer2_main_path, 'r') as f:
            content = f.read()
        
        # Find and replace numRobots
        import re
        new_content = re.sub(
            r'int numRobots = \d+;',
            f'int numRobots = {actual_robots};',
            content
        )
        
        with open(layer2_main_path, 'w') as f:
            f.write(new_content)
        
        print(f"[Layer2] Updated robot count to {actual_robots}")
    except Exception as e:
        print(f"[WARNING] Could not update Layer 2 main.cc: {e}")


def interactive_setup() -> WarehouseConfig:
    """Interactive configuration wizard."""
    config = WarehouseConfig()
    
    print("\n" + "=" * 70)
    print("          MECALUX AMR - WAREHOUSE SETUP WIZARD")
    print("=" * 70)
    
    # Image path
    default_image = "warehouse2.PNG"
    image_input = input(f"\n[1] Warehouse image file [{default_image}]: ").strip()
    config.image_path = image_input if image_input else default_image
    
    # Dimensions
    print("\n[2] Physical dimensions (plant size in meters)")
    dim_input = input("    Enter as WIDTHxHEIGHT (e.g., 160x60 for 160m x 60m) [30x20]: ").strip()
    if dim_input:
        try:
            parts = dim_input.lower().replace(' ', '').split('x')
            config.width_meters = float(parts[0])
            config.height_meters = float(parts[1])
        except:
            print("    [Using default 30x20]")
    
    # Resolution
    print("\n[3] Resolution (pixel size)")
    print("    1. DECIMETERS (0.1m/pixel) - for large warehouses")
    print("    2. CENTIMETERS (0.01m/pixel) - for detailed maps")
    print("    3. MILLIMETERS (0.001m/pixel) - for very detailed maps")
    res_input = input("    Choose resolution [1]: ").strip()
    if res_input == "2":
        config.resolution = "CENTIMETERS"
    elif res_input == "3":
        config.resolution = "MILLIMETERS"
    else:
        config.resolution = "DECIMETERS"
    
    # Charging zones
    print("\n[4] Charging Zones")
    num_input = input("    How many charging zones? [6]: ").strip()
    config.num_charging = int(num_input) if num_input else 6
    
    cluster_input = input(f"    In how many clusters? [2]: ").strip()
    config.charging_clusters = int(cluster_input) if cluster_input else 2
    
    # Pickup zones
    print("\n[5] Pickup Zones")
    num_input = input("    How many pickup zones? [16]: ").strip()
    config.num_pickup = int(num_input) if num_input else 16
    
    cluster_input = input(f"    In how many clusters? [4]: ").strip()
    config.pickup_clusters = int(cluster_input) if cluster_input else 4
    
    # Dropoff zones
    print("\n[6] Dropoff Zones")
    num_input = input("    How many dropoff zones? [16]: ").strip()
    config.num_dropoff = int(num_input) if num_input else 16
    
    cluster_input = input(f"    In how many clusters? [4]: ").strip()
    config.dropoff_clusters = int(cluster_input) if cluster_input else 4
    
    # Robots
    print("\n[7] Fleet Configuration")
    num_input = input("    How many robots? [3]: ").strip()
    config.num_robots = int(num_input) if num_input else 3
    
    # Tasks
    num_input = input("    How many tasks to generate? [50]: ").strip()
    config.num_tasks = int(num_input) if num_input else 50
    
    # Robot radius
    radius_input = input("    Robot radius in meters? [0.3]: ").strip()
    config.robot_radius_m = float(radius_input) if radius_input else 0.3
    
    # Seed
    seed_input = input("\n[8] Random seed (leave empty for random): ").strip()
    config.seed = int(seed_input) if seed_input else None
    
    return config


def print_summary(config: WarehouseConfig):
    """Print configuration summary."""
    resolution_factor = get_resolution_factor(config.resolution)
    px_width = int(config.width_meters / resolution_factor)
    px_height = int(config.height_meters / resolution_factor)
    
    print("\n" + "=" * 70)
    print("                    CONFIGURATION SUMMARY")
    print("=" * 70)
    print(f"  Image:           {config.image_path}")
    print(f"  Physical Size:   {config.width_meters}m x {config.height_meters}m")
    print(f"  Resolution:      {config.resolution} ({resolution_factor}m/pixel)")
    print(f"  Pixel Size:      {px_width} x {px_height} pixels")
    print(f"  Robot Radius:    {config.robot_radius_m}m")
    print()
    print(f"  Charging Zones:  {config.num_charging} in {config.charging_clusters} clusters")
    print(f"  Pickup Zones:    {config.num_pickup} in {config.pickup_clusters} clusters")
    print(f"  Dropoff Zones:   {config.num_dropoff} in {config.dropoff_clusters} clusters")
    print()
    print(f"  Robots:          {config.num_robots}")
    print(f"  Tasks:           {config.num_tasks}")
    print(f"  Seed:            {config.seed if config.seed else 'random'}")
    print("=" * 70)


def main():
    parser = argparse.ArgumentParser(description='Warehouse Setup Wizard')
    parser.add_argument('--image', type=str, help='Warehouse image file')
    parser.add_argument('--width', type=float, help='Width in meters')
    parser.add_argument('--height', type=float, help='Height in meters')
    parser.add_argument('--resolution', choices=['DECIMETERS', 'CENTIMETERS', 'MILLIMETERS'])
    parser.add_argument('--charging', type=int, help='Number of charging zones')
    parser.add_argument('--charging-clusters', type=int, help='Number of charging clusters')
    parser.add_argument('--pickup', type=int, help='Number of pickup zones')
    parser.add_argument('--pickup-clusters', type=int, help='Number of pickup clusters')
    parser.add_argument('--dropoff', type=int, help='Number of dropoff zones')
    parser.add_argument('--dropoff-clusters', type=int, help='Number of dropoff clusters')
    parser.add_argument('--robots', type=int, help='Number of robots')
    parser.add_argument('--tasks', type=int, help='Number of tasks')
    parser.add_argument('--seed', type=int, help='Random seed')
    parser.add_argument('--non-interactive', action='store_true', help='Skip interactive prompts')
    
    args = parser.parse_args()
    
    script_dir = Path(__file__).parent
    assets_dir = script_dir / ".." / "assets"
    
    # Interactive or command-line configuration
    if args.non_interactive:
        config = WarehouseConfig()
        if args.image: config.image_path = args.image
        if args.width: config.width_meters = args.width
        if args.height: config.height_meters = args.height
        if args.resolution: config.resolution = args.resolution
        if args.charging: config.num_charging = args.charging
        if args.charging_clusters: config.charging_clusters = args.charging_clusters
        if args.pickup: config.num_pickup = args.pickup
        if args.pickup_clusters: config.pickup_clusters = args.pickup_clusters
        if args.dropoff: config.num_dropoff = args.dropoff
        if args.dropoff_clusters: config.dropoff_clusters = args.dropoff_clusters
        if args.robots: config.num_robots = args.robots
        if args.tasks: config.num_tasks = args.tasks
        if args.seed: config.seed = args.seed
    else:
        config = interactive_setup()
    
    print_summary(config)
    
    if not args.non_interactive:
        confirm = input("\nProceed with this configuration? [Y/n]: ").strip().lower()
        if confirm == 'n':
            print("Aborted.")
            return 1
    
    # Check for PIL
    if not HAS_PIL:
        print("\n[ERROR] PIL/Pillow is required. Install with: pip install Pillow")
        return 1
    
    # Resolve image path
    image_path = Path(config.image_path)
    if not image_path.is_absolute():
        image_path = script_dir / config.image_path
    
    if not image_path.exists():
        print(f"\n[ERROR] Image not found: {image_path}")
        return 1
    
    print("\n" + "-" * 70)
    print("  PROCESSING...")
    print("-" * 70)
    
    # Calculate pixel dimensions
    resolution_factor = get_resolution_factor(config.resolution)
    px_width = int(config.width_meters / resolution_factor)
    px_height = int(config.height_meters / resolution_factor)
    
    print(f"\n[Step 1/5] Converting image to {px_width}x{px_height} map...")
    binary_map = image_to_map(str(image_path), px_width, px_height)
    
    # Save map layout
    map_output_path = assets_dir / "map_layout.txt"
    save_map_layout(binary_map, str(map_output_path))
    
    # Inflate map for robot accessibility
    print(f"\n[Step 2/5] Computing robot accessibility (radius={config.robot_radius_m}m)...")
    radius_pixels = int(math.ceil(config.robot_radius_m / resolution_factor))
    inflated_map = inflate_map(binary_map, radius_pixels)
    
    accessible_cells = get_accessible_cells(inflated_map)
    print(f"[Accessibility] {len(accessible_cells)} accessible cells")
    
    # Generate POIs
    print(f"\n[Step 3/5] Generating POI clusters...")
    poi_config = generate_poi_config(config, accessible_cells)
    
    poi_output_path = assets_dir / "poi_config.json"
    with open(poi_output_path, 'w') as f:
        json.dump(poi_config, f, indent=4)
    print(f"[POIGenerator] Saved to: {poi_output_path}")
    
    # Generate tasks
    print(f"\n[Step 4/5] Generating {config.num_tasks} tasks...")
    seed_tasks = config.seed + 100 if config.seed else None
    tasks = generate_tasks(poi_config, config.num_tasks, seed_tasks)
    
    tasks_output_path = script_dir / ".." / ".." / ".." / "api" / "set_of_tasks.json"
    tasks_output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(tasks_output_path, 'w') as f:
        json.dump(tasks, f, indent=2)
    print(f"[TaskGenerator] Saved to: {tasks_output_path}")
    
    # Update Layer 2 robot count
    print(f"\n[Step 5/5] Updating Layer 2 configuration...")
    layer2_main = script_dir / ".." / ".." / "layer2" / "main.cc"
    update_layer2_robots(config, poi_config, str(layer2_main))
    
    # Final summary
    print("\n" + "=" * 70)
    print("                    SETUP COMPLETE!")
    print("=" * 70)
    print(f"\n  Generated files:")
    print(f"    Map:      {map_output_path}")
    print(f"    POIs:     {poi_output_path}")
    print(f"    Tasks:    {tasks_output_path}")
    print(f"\n  POI Summary:")
    charging = len([p for p in poi_config["poi"] if p["type"] == "CHARGING"])
    pickup = len([p for p in poi_config["poi"] if p["type"] == "PICKUP"])
    dropoff = len([p for p in poi_config["poi"] if p["type"] == "DROPOFF"])
    print(f"    Charging: {charging}")
    print(f"    Pickup:   {pickup}")
    print(f"    Dropoff:  {dropoff}")
    print(f"    Total:    {charging + pickup + dropoff}")
    print(f"\n  Fleet:")
    print(f"    Robots:   {min(config.num_robots, charging)}")
    print(f"    Tasks:    {config.num_tasks}")
    
    print(f"\n  Next steps:")
    print(f"    1. cd ../../layer2")
    print(f"    2. make clean && make")
    print(f"    3. ./build/test_layer2")
    print("=" * 70 + "\n")
    
    return 0


if __name__ == '__main__':
    exit(main())
