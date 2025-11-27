#!/usr/bin/env python3
"""
POI Generator for Mecalux AMR Fleet Manager

Generates clustered Points of Interest (charging zones, pickup/dropoff nodes)
for the warehouse map, ensuring:
1. Clusters: Max 3 clusters of charging (max 2 each), max 4 clusters of pickup/dropoff (max 4 each)
2. No overlap with obstacles
3. Robot accessibility (Configuration Space aware)

Usage:
    python poi_generator.py --map ../assets/map_layout.txt --output ../assets/poi_config.json
    python poi_generator.py -m map.txt -o poi.json --charging-clusters 2 --seed 42
"""

import json
import random
import argparse
import math
from pathlib import Path
from typing import List, Dict, Tuple, Set, Optional
from dataclasses import dataclass, field


@dataclass
class POIConfig:
    """Configuration for POI generation."""
    # Cluster settings
    max_charging_clusters: int = 3
    max_charging_per_cluster: int = 2
    max_pickup_clusters: int = 4
    max_pickup_per_cluster: int = 4
    max_dropoff_clusters: int = 4
    max_dropoff_per_cluster: int = 4
    
    # Spacing settings
    cluster_spacing: int = 10       # Min pixels between nodes in same cluster
    inter_cluster_spacing: int = 30  # Min pixels between cluster centers
    
    # Robot dimensions (for accessibility)
    robot_radius_meters: float = 0.3
    resolution_meters_per_pixel: float = 0.1  # DECIMETERS
    
    # Map margins (keep POIs away from edges)
    edge_margin: int = 10


@dataclass
class Cluster:
    """Represents a cluster of POIs."""
    center_x: int
    center_y: int
    poi_type: str  # CHARGING, PICKUP, or DROPOFF
    nodes: List[Tuple[int, int]] = field(default_factory=list)


class MapAccessibility:
    """
    Handles map loading and accessibility checking.
    Implements Configuration Space (C-Space) inflation.
    """
    
    def __init__(self, map_path: str, config: POIConfig):
        self.config = config
        self.width = 0
        self.height = 0
        self.static_map: List[List[bool]] = []  # True = walkable
        self.inflated_map: List[List[bool]] = []  # True = robot can fit
        
        self._load_map(map_path)
        self._inflate_map()
    
    def _load_map(self, map_path: str):
        """Load static bitmap from text file."""
        with open(map_path, 'r') as f:
            lines = f.readlines()
        
        self.height = len(lines)
        self.width = max(len(line.rstrip()) for line in lines) if lines else 0
        
        self.static_map = []
        for line in lines:
            row = []
            line = line.rstrip()
            for i in range(self.width):
                if i < len(line):
                    # '.' or ' ' = walkable, '#' or 'X' = obstacle
                    char = line[i]
                    row.append(char in '.0 ')
                else:
                    row.append(True)  # Pad with walkable
            self.static_map.append(row)
        
        walkable = sum(1 for row in self.static_map for cell in row if cell)
        print(f"[MapAccessibility] Loaded {self.width}x{self.height} map")
        print(f"[MapAccessibility] Walkable cells: {walkable}/{self.width * self.height}")
    
    def _inflate_map(self):
        """
        Inflate obstacles by robot radius to create Configuration Space.
        A cell is accessible only if the robot centered there doesn't collide.
        """
        # Calculate inflation radius in pixels
        radius_pixels = int(math.ceil(
            self.config.robot_radius_meters / self.config.resolution_meters_per_pixel
        ))
        
        print(f"[MapAccessibility] Robot radius: {self.config.robot_radius_meters}m = {radius_pixels}px")
        
        # Create inflated map (start as copy of static)
        self.inflated_map = [row.copy() for row in self.static_map]
        
        # For each obstacle cell, mark surrounding cells as inaccessible
        closed_count = 0
        for y in range(self.height):
            for x in range(self.width):
                if not self.static_map[y][x]:  # This is an obstacle
                    # Close all cells within radius
                    for dy in range(-radius_pixels, radius_pixels + 1):
                        for dx in range(-radius_pixels, radius_pixels + 1):
                            # Check circular distance
                            if dx*dx + dy*dy <= radius_pixels * radius_pixels:
                                ny, nx = y + dy, x + dx
                                if 0 <= ny < self.height and 0 <= nx < self.width:
                                    if self.inflated_map[ny][nx]:
                                        self.inflated_map[ny][nx] = False
                                        closed_count += 1
        
        # Also close cells near map edges
        margin = self.config.edge_margin
        for y in range(self.height):
            for x in range(self.width):
                if x < margin or x >= self.width - margin or \
                   y < margin or y >= self.height - margin:
                    if self.inflated_map[y][x]:
                        self.inflated_map[y][x] = False
                        closed_count += 1
        
        accessible = sum(1 for row in self.inflated_map for cell in row if cell)
        print(f"[MapAccessibility] Cells closed by inflation: {closed_count}")
        print(f"[MapAccessibility] Accessible cells: {accessible}")
    
    def is_accessible(self, x: int, y: int) -> bool:
        """Check if a position is accessible by the robot."""
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.inflated_map[y][x]
        return False
    
    def get_accessible_cells(self) -> List[Tuple[int, int]]:
        """Get list of all accessible (x, y) positions."""
        cells = []
        for y in range(self.height):
            for x in range(self.width):
                if self.inflated_map[y][x]:
                    cells.append((x, y))
        return cells
    
    def find_accessible_region(self, center_x: int, center_y: int, radius: int) -> List[Tuple[int, int]]:
        """Find all accessible cells within a radius of the center."""
        cells = []
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                if dx*dx + dy*dy <= radius * radius:
                    x, y = center_x + dx, center_y + dy
                    if self.is_accessible(x, y):
                        cells.append((x, y))
        return cells


class POIGenerator:
    """
    Generates clustered POIs for the warehouse map.
    """
    
    def __init__(self, map_access: MapAccessibility, config: POIConfig, seed: int = None):
        self.map_access = map_access
        self.config = config
        self.clusters: List[Cluster] = []
        self.used_positions: Set[Tuple[int, int]] = set()
        
        if seed is not None:
            random.seed(seed)
    
    def _distance(self, p1: Tuple[int, int], p2: Tuple[int, int]) -> float:
        """Calculate Euclidean distance between two points."""
        return math.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2)
    
    def _is_valid_cluster_center(self, x: int, y: int, min_dist: int) -> bool:
        """Check if position is valid for a new cluster center."""
        if not self.map_access.is_accessible(x, y):
            return False
        
        # Check distance to existing cluster centers
        for cluster in self.clusters:
            if self._distance((x, y), (cluster.center_x, cluster.center_y)) < min_dist:
                return False
        
        return True
    
    def _is_valid_poi_position(self, x: int, y: int, min_dist: int) -> bool:
        """Check if position is valid for a new POI."""
        if not self.map_access.is_accessible(x, y):
            return False
        
        # Check distance to used positions
        for pos in self.used_positions:
            if self._distance((x, y), pos) < min_dist:
                return False
        
        return True
    
    def _find_cluster_center(self, poi_type: str) -> Optional[Tuple[int, int]]:
        """Find a valid position for a new cluster center."""
        accessible = self.map_access.get_accessible_cells()
        random.shuffle(accessible)
        
        for x, y in accessible:
            if self._is_valid_cluster_center(x, y, self.config.inter_cluster_spacing):
                return (x, y)
        
        return None
    
    def _populate_cluster(self, cluster: Cluster, max_nodes: int):
        """Add POI nodes to a cluster."""
        # Find accessible cells near cluster center
        search_radius = self.config.inter_cluster_spacing // 2
        candidates = self.map_access.find_accessible_region(
            cluster.center_x, cluster.center_y, search_radius
        )
        random.shuffle(candidates)
        
        for x, y in candidates:
            if len(cluster.nodes) >= max_nodes:
                break
            
            if self._is_valid_poi_position(x, y, self.config.cluster_spacing):
                cluster.nodes.append((x, y))
                self.used_positions.add((x, y))
    
    def generate_charging_clusters(self, num_clusters: int = None, nodes_per_cluster: int = None):
        """Generate charging station clusters."""
        if num_clusters is None:
            num_clusters = random.randint(1, self.config.max_charging_clusters)
        if nodes_per_cluster is None:
            nodes_per_cluster = random.randint(1, self.config.max_charging_per_cluster)
        
        print(f"[POIGenerator] Generating {num_clusters} charging clusters (up to {nodes_per_cluster} nodes each)")
        
        for _ in range(num_clusters):
            center = self._find_cluster_center("CHARGING")
            if center is None:
                print(f"[POIGenerator] WARNING: Could not find valid charging cluster center")
                continue
            
            cluster = Cluster(center[0], center[1], "CHARGING")
            self._populate_cluster(cluster, nodes_per_cluster)
            
            if cluster.nodes:
                self.clusters.append(cluster)
                print(f"[POIGenerator] Charging cluster at ({center[0]}, {center[1]}): {len(cluster.nodes)} nodes")
    
    def generate_pickup_clusters(self, num_clusters: int = None, nodes_per_cluster: int = None):
        """Generate pickup zone clusters."""
        if num_clusters is None:
            num_clusters = random.randint(1, self.config.max_pickup_clusters)
        if nodes_per_cluster is None:
            nodes_per_cluster = random.randint(1, self.config.max_pickup_per_cluster)
        
        print(f"[POIGenerator] Generating {num_clusters} pickup clusters (up to {nodes_per_cluster} nodes each)")
        
        for _ in range(num_clusters):
            center = self._find_cluster_center("PICKUP")
            if center is None:
                print(f"[POIGenerator] WARNING: Could not find valid pickup cluster center")
                continue
            
            cluster = Cluster(center[0], center[1], "PICKUP")
            self._populate_cluster(cluster, nodes_per_cluster)
            
            if cluster.nodes:
                self.clusters.append(cluster)
                print(f"[POIGenerator] Pickup cluster at ({center[0]}, {center[1]}): {len(cluster.nodes)} nodes")
    
    def generate_dropoff_clusters(self, num_clusters: int = None, nodes_per_cluster: int = None):
        """Generate dropoff zone clusters."""
        if num_clusters is None:
            num_clusters = random.randint(1, self.config.max_dropoff_clusters)
        if nodes_per_cluster is None:
            nodes_per_cluster = random.randint(1, self.config.max_dropoff_per_cluster)
        
        print(f"[POIGenerator] Generating {num_clusters} dropoff clusters (up to {nodes_per_cluster} nodes each)")
        
        for _ in range(num_clusters):
            center = self._find_cluster_center("DROPOFF")
            if center is None:
                print(f"[POIGenerator] WARNING: Could not find valid dropoff cluster center")
                continue
            
            cluster = Cluster(center[0], center[1], "DROPOFF")
            self._populate_cluster(cluster, nodes_per_cluster)
            
            if cluster.nodes:
                self.clusters.append(cluster)
                print(f"[POIGenerator] Dropoff cluster at ({center[0]}, {center[1]}): {len(cluster.nodes)} nodes")
    
    def generate_all(self):
        """Generate all POI clusters."""
        self.generate_charging_clusters()
        self.generate_pickup_clusters()
        self.generate_dropoff_clusters()
    
    def to_poi_config(self) -> Dict:
        """Convert generated clusters to POI configuration format."""
        pois = []
        
        # Counter for IDs
        charging_count = 0
        pickup_count = 0
        dropoff_count = 0
        
        for cluster in self.clusters:
            for x, y in cluster.nodes:
                if cluster.poi_type == "CHARGING":
                    poi_id = f"C{charging_count}"
                    charging_count += 1
                elif cluster.poi_type == "PICKUP":
                    poi_id = f"P{pickup_count * 2}"  # Even numbers for pickup
                    pickup_count += 1
                else:  # DROPOFF
                    poi_id = f"P{dropoff_count * 2 + 1}"  # Odd numbers for dropoff
                    dropoff_count += 1
                
                pois.append({
                    "id": poi_id,
                    "type": cluster.poi_type,
                    "x": x,
                    "y": y,
                    "active": True,
                    "metadata": {
                        "cluster_center": f"({cluster.center_x}, {cluster.center_y})",
                        "generated": True
                    }
                })
        
        return {
            "description": "Auto-generated Points of Interest for the warehouse",
            "version": "3.0",
            "coordinate_system": f"pixels ({self.config.resolution_meters_per_pixel}m/pixel)",
            "generation_config": {
                "robot_radius_m": self.config.robot_radius_meters,
                "cluster_spacing_px": self.config.cluster_spacing,
                "inter_cluster_spacing_px": self.config.inter_cluster_spacing
            },
            "poi_types": {
                "CHARGING": "Battery charging stations (IDs: C0, C1, C2...)",
                "PICKUP": "Locations where robot picks up packages (even P-numbers)",
                "DROPOFF": "Locations where robot drops off packages (odd P-numbers)"
            },
            "poi": pois
        }
    
    def print_summary(self):
        """Print generation summary."""
        print("\n" + "=" * 60)
        print("           POI GENERATOR SUMMARY")
        print("=" * 60)
        
        charging = [c for c in self.clusters if c.poi_type == "CHARGING"]
        pickup = [c for c in self.clusters if c.poi_type == "PICKUP"]
        dropoff = [c for c in self.clusters if c.poi_type == "DROPOFF"]
        
        print(f"\nClusters Generated:")
        print(f"  Charging clusters: {len(charging)}")
        for i, c in enumerate(charging):
            print(f"    Cluster {i}: center=({c.center_x}, {c.center_y}), nodes={len(c.nodes)}")
        
        print(f"  Pickup clusters:   {len(pickup)}")
        for i, c in enumerate(pickup):
            print(f"    Cluster {i}: center=({c.center_x}, {c.center_y}), nodes={len(c.nodes)}")
        
        print(f"  Dropoff clusters:  {len(dropoff)}")
        for i, c in enumerate(dropoff):
            print(f"    Cluster {i}: center=({c.center_x}, {c.center_y}), nodes={len(c.nodes)}")
        
        total_charging = sum(len(c.nodes) for c in charging)
        total_pickup = sum(len(c.nodes) for c in pickup)
        total_dropoff = sum(len(c.nodes) for c in dropoff)
        
        print(f"\nTotal POIs:")
        print(f"  Charging: {total_charging}")
        print(f"  Pickup:   {total_pickup}")
        print(f"  Dropoff:  {total_dropoff}")
        print(f"  TOTAL:    {total_charging + total_pickup + total_dropoff}")
        print("=" * 60 + "\n")


def main():
    parser = argparse.ArgumentParser(
        description='Generate clustered POIs for warehouse map'
    )
    parser.add_argument(
        '-m', '--map',
        type=str,
        default='../assets/map_layout.txt',
        help='Path to map layout file'
    )
    parser.add_argument(
        '-o', '--output',
        type=str,
        default='../assets/poi_config_generated.json',
        help='Output path for generated POI config'
    )
    parser.add_argument(
        '--charging-clusters',
        type=int,
        default=None,
        help='Number of charging clusters (default: random 1-3)'
    )
    parser.add_argument(
        '--charging-per-cluster',
        type=int,
        default=None,
        help='Max charging nodes per cluster (default: random 1-2)'
    )
    parser.add_argument(
        '--pickup-clusters',
        type=int,
        default=None,
        help='Number of pickup clusters (default: random 1-4)'
    )
    parser.add_argument(
        '--pickup-per-cluster',
        type=int,
        default=None,
        help='Max pickup nodes per cluster (default: random 1-4)'
    )
    parser.add_argument(
        '--dropoff-clusters',
        type=int,
        default=None,
        help='Number of dropoff clusters (default: random 1-4)'
    )
    parser.add_argument(
        '--dropoff-per-cluster',
        type=int,
        default=None,
        help='Max dropoff nodes per cluster (default: random 1-4)'
    )
    parser.add_argument(
        '--robot-radius',
        type=float,
        default=0.3,
        help='Robot radius in meters (default: 0.3)'
    )
    parser.add_argument(
        '--resolution',
        type=float,
        default=0.1,
        help='Map resolution in meters/pixel (default: 0.1 = DECIMETERS)'
    )
    parser.add_argument(
        '-s', '--seed',
        type=int,
        default=None,
        help='Random seed for reproducibility'
    )
    parser.add_argument(
        '-q', '--quiet',
        action='store_true',
        help='Suppress summary output'
    )
    
    args = parser.parse_args()
    
    # Resolve paths relative to script location
    script_dir = Path(__file__).parent
    map_path = script_dir / args.map
    output_path = script_dir / args.output
    
    # Create configuration
    config = POIConfig(
        robot_radius_meters=args.robot_radius,
        resolution_meters_per_pixel=args.resolution
    )
    
    print(f"[POIGenerator] Loading map from: {map_path}")
    
    # Load map and create accessibility checker
    try:
        map_access = MapAccessibility(str(map_path), config)
    except FileNotFoundError:
        print(f"[POIGenerator] ERROR: Map file not found: {map_path}")
        return 1
    
    # Create generator
    generator = POIGenerator(map_access, config, args.seed)
    
    # Generate clusters
    generator.generate_charging_clusters(args.charging_clusters, args.charging_per_cluster)
    generator.generate_pickup_clusters(args.pickup_clusters, args.pickup_per_cluster)
    generator.generate_dropoff_clusters(args.dropoff_clusters, args.dropoff_per_cluster)
    
    # Convert to POI config
    poi_config = generator.to_poi_config()
    
    # Save to file
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'w') as f:
        json.dump(poi_config, f, indent=4)
    print(f"[POIGenerator] Saved POI config to: {output_path}")
    
    # Print summary
    if not args.quiet:
        generator.print_summary()
    
    return 0


if __name__ == '__main__':
    exit(main())
