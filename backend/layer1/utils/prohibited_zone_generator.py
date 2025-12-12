#!/usr/bin/env python3
"""
Prohibited Zone Generator for Mecalux AMR

Converts warehouse objects and prohibited zones from JSON layout
into a binary grid (obstacle map) for the backend mapping system.

The generator:
1. Loads warehouse layout from JSON
2. Converts 3D objects to 2D grid cells
3. Handles rotation of rectangular objects
4. Generates binary grid (True = walkable, False = obstacle)
5. Exports to StaticBitMap-compatible format

Usage:
    from prohibited_zone_generator import ProhibitedZoneGenerator
    from warehouse_layout_loader import WarehouseLayoutLoader
    
    loader = WarehouseLayoutLoader("warehouse_layout.json")
    generator = ProhibitedZoneGenerator(
        loader=loader,
        resolution_m_per_px=0.1,  # DECIMETERS
        robot_radius_m=0.3
    )
    
    grid = generator.generate_grid()
    generator.export_to_bitmap_file("map_layout.txt")
"""

import math
from typing import List, Tuple, Set, Optional
from warehouse_layout_loader import WarehouseLayoutLoader, Object, ProhibitedZone


class ProhibitedZoneGenerator:
    """
    Converts warehouse objects and zones into a binary grid.
    
    The grid uses Configuration Space concept: cells are marked as obstacles
    to ensure robots (with their physical radius) avoid collisions.
    """
    
    def __init__(self, 
                 loader: WarehouseLayoutLoader,
                 resolution_m_per_px: float = 0.1,
                 robot_radius_m: float = 0.3):
        """
        Initialize the prohibited zone generator.
        
        Args:
            loader: Loaded WarehouseLayoutLoader instance
            resolution_m_per_px: Physical size per pixel (default 0.1m = DECIMETERS)
            robot_radius_m: Robot radius for safety buffer (default 0.3m)
        """
        self.loader = loader
        self.resolution_m_per_px = resolution_m_per_px
        self.robot_radius_m = robot_radius_m
        
        # Get floor dimensions
        floor_width, floor_height = loader.get_floor_size()
        
        # Calculate grid dimensions
        self.grid_width = int(math.ceil(floor_width / resolution_m_per_px))
        self.grid_height = int(math.ceil(floor_height / resolution_m_per_px))
        
        # Initialize grid: True = walkable, False = obstacle
        self.grid: List[List[bool]] = [
            [True for _ in range(self.grid_width)] 
            for _ in range(self.grid_height)
        ]
        
        # Track statistics
        self.obstacle_cells = 0
        self.walkable_cells = self.grid_width * self.grid_height
        
        print(f"[ProhibitedZoneGenerator] Initialized")
        print(f"  Floor size: {floor_width}m × {floor_height}m")
        print(f"  Grid dimensions: {self.grid_width} × {self.grid_height} pixels")
        print(f"  Resolution: {resolution_m_per_px}m/pixel")
        print(f"  Robot radius: {robot_radius_m}m")
    
    def generate_grid(self) -> List[List[bool]]:
        """
        Generate the complete grid by marking all obstacles.
        
        Returns:
            2D grid where True = walkable, False = obstacle
        """
        print("\n[ProhibitedZoneGenerator] Generating grid...")
        
        # Process objects from warehouse
        print(f"[ProhibitedZoneGenerator] Processing {len(self.loader.get_objects())} objects")
        for i, obj in enumerate(self.loader.get_objects()):
            self._mark_object_obstacle(obj)
        
        # Process prohibited zones
        zones = self.loader.get_prohibited_zones()
        if zones:
            print(f"[ProhibitedZoneGenerator] Processing {len(zones)} prohibited zones")
            for zone in zones:
                self._mark_zone_obstacle(zone)
        
        # Update statistics
        self.obstacle_cells = sum(1 for row in self.grid for cell in row if not cell)
        self.walkable_cells = self.grid_width * self.grid_height - self.obstacle_cells
        
        print(f"[ProhibitedZoneGenerator] Grid generation complete")
        print(f"  Obstacle cells: {self.obstacle_cells}")
        print(f"  Walkable cells: {self.walkable_cells}")
        print(f"  Obstacle ratio: {100 * self.obstacle_cells / (self.grid_width * self.grid_height):.1f}%")
        
        return self.grid
    
    def _mark_object_obstacle(self, obj: Object) -> None:
        """
        Mark obstacle region for a warehouse object.
        
        Converts 3D object (x, y, z, w, h, d) to 2D grid cells.
        
        Special handling:
        - modelIndex=5 (charging stations): NOT marked as obstacles (robots need to access them)
        - All other objects: Marked as obstacles
        
        JSON coordinate system:
        - center[0] = x (left-right in meters)
        - center[1] = y (up-down, typically 0 for floor level)
        - center[2] = z (forward-backward in meters)
        
        Grid coordinate system (2D top-down view):
        - x axis = JSON center[0] (left-right)
        - y axis = JSON center[2] (forward-backward)
        
        Args:
            obj: Object from warehouse layout (dimensions in meters)
        """
        # Skip charging stations - robots need to access them
        if obj.model_index == 5:
            return
        
        # Extract 2D position from 3D center
        # Use x and z from JSON (z becomes y in 2D grid)
        center_x = obj.center[0]  # meters
        center_y = obj.center[2]  # meters (z in 3D becomes y in 2D)
        
        # Extract 2D dimensions (width and depth for top-down view, ignore height)
        # dimensions[0] = width (x-axis), dimensions[2] = depth (z-axis in 3D, y-axis in 2D)
        width = obj.dimensions[0]    # meters
        height = obj.dimensions[2]   # meters (depth in 3D, height in 2D grid)
        rotation = obj.rotation
        
        # Mark obstacle region
        if abs(rotation) < 0.01 or abs(rotation - 180) < 0.01:
            # No significant rotation: use simple AABB
            self._mark_rectangle_obstacle(center_x, center_y, width, height)
        else:
            # Rotated rectangle: use polygon-based approach
            self._mark_rotated_rectangle_obstacle(center_x, center_y, width, height, rotation)
    
    def _mark_zone_obstacle(self, zone: ProhibitedZone) -> None:
        """
        Mark obstacle region for a prohibited zone.
        
        ProhibitedZone coordinates are already in 2D:
        - center[0] = x (left-right in meters)
        - center[1] = y (forward-backward in meters)
        
        Args:
            zone: ProhibitedZone from warehouse layout (already in 2D, in meters)
        """
        center_x = zone.center[0]  # meters, already 2D x
        center_y = zone.center[1]  # meters, already 2D y
        rotation = zone.rotation
        
        if zone.shape.lower() == "rectangle":
            # Rectangular zone
            if len(zone.dimensions) < 2:
                return
            
            width = zone.dimensions[0]   # meters
            height = zone.dimensions[1]  # meters
            
            if abs(rotation) < 0.01 or abs(rotation - 180) < 0.01:
                self._mark_rectangle_obstacle(center_x, center_y, width, height)
            else:
                self._mark_rotated_rectangle_obstacle(center_x, center_y, width, height, rotation)
        
        elif zone.shape.lower() == "circle":
            # Circular zone
            if len(zone.dimensions) < 1:
                return
            radius = zone.dimensions[0]  # meters
            self._mark_circle_obstacle(center_x, center_y, radius)
    
    def _mark_rectangle_obstacle(self, center_x: float, center_y: float, 
                                 width: float, height: float) -> None:
        """
        Mark a rectangular obstacle without rotation (AABB).
        
        Args:
            center_x, center_y: Center position in meters
            width, height: Rectangle dimensions in meters
        """
        # Convert to grid coordinates
        x1_px = int(math.floor((center_x - width / 2) / self.resolution_m_per_px))
        x2_px = int(math.ceil((center_x + width / 2) / self.resolution_m_per_px))
        y1_px = int(math.floor((center_y - height / 2) / self.resolution_m_per_px))
        y2_px = int(math.ceil((center_y + height / 2) / self.resolution_m_per_px))
        
        # Clamp to grid bounds
        x1_px = max(0, min(x1_px, self.grid_width - 1))
        x2_px = max(0, min(x2_px, self.grid_width - 1))
        y1_px = max(0, min(y1_px, self.grid_height - 1))
        y2_px = max(0, min(y2_px, self.grid_height - 1))
        
        # Mark cells as obstacles
        for y in range(y1_px, y2_px + 1):
            for x in range(x1_px, x2_px + 1):
                if 0 <= y < self.grid_height and 0 <= x < self.grid_width:
                    self.grid[y][x] = False
    
    def _mark_rotated_rectangle_obstacle(self, center_x: float, center_y: float,
                                        width: float, height: float,
                                        rotation_deg: float) -> None:
        """
        Mark a rotated rectangular obstacle using polygon rasterization.
        
        Converts rectangle to world-space polygon, then rasterizes to grid.
        
        Args:
            center_x, center_y: Center position in meters
            width, height: Rectangle dimensions in meters
            rotation_deg: Rotation angle in degrees
        """
        # Get rectangle corners in local coordinates
        half_w = width / 2
        half_h = height / 2
        
        corners_local = [
            (-half_w, -half_h),
            (half_w, -half_h),
            (half_w, half_h),
            (-half_w, half_h)
        ]
        
        # Apply rotation
        rotation_rad = math.radians(rotation_deg)
        cos_r = math.cos(rotation_rad)
        sin_r = math.sin(rotation_rad)
        
        corners_world = []
        for lx, ly in corners_local:
            # Rotate
            rx = lx * cos_r - ly * sin_r
            ry = lx * sin_r + ly * cos_r
            # Translate to center
            wx = center_x + rx
            wy = center_y + ry
            corners_world.append((wx, wy))
        
        # Rasterize polygon
        self._rasterize_polygon(corners_world)
    
    def _mark_circle_obstacle(self, center_x: float, center_y: float,
                             radius: float) -> None:
        """
        Mark a circular obstacle.
        
        Args:
            center_x, center_y: Center position in meters
            radius: Radius in meters
        """
        # Convert to grid coordinates
        center_px_x = center_x / self.resolution_m_per_px
        center_px_y = center_y / self.resolution_m_per_px
        radius_px = radius / self.resolution_m_per_px
        
        # Scan bounding box
        x_min = max(0, int(math.floor(center_px_x - radius_px)))
        x_max = min(self.grid_width, int(math.ceil(center_px_x + radius_px)))
        y_min = max(0, int(math.floor(center_px_y - radius_px)))
        y_max = min(self.grid_height, int(math.ceil(center_px_y + radius_px)))
        
        # Mark cells within radius
        for y in range(y_min, y_max):
            for x in range(x_min, x_max):
                dx = x - center_px_x
                dy = y - center_px_y
                distance = math.sqrt(dx * dx + dy * dy)
                
                if distance <= radius_px:
                    self.grid[y][x] = False
    
    def _rasterize_polygon(self, vertices: List[Tuple[float, float]]) -> None:
        """
        Rasterize a polygon (in world coordinates) to the grid.
        
        Uses scan-line algorithm to find cells inside polygon.
        
        Args:
            vertices: List of (x, y) vertices in world coordinates (meters)
        """
        if len(vertices) < 3:
            return
        
        # Convert vertices to pixel coordinates
        vertices_px = [
            (v[0] / self.resolution_m_per_px, v[1] / self.resolution_m_per_px)
            for v in vertices
        ]
        
        # Find bounding box
        xs = [v[0] for v in vertices_px]
        ys = [v[1] for v in vertices_px]
        
        x_min = max(0, int(math.floor(min(xs))))
        x_max = min(self.grid_width, int(math.ceil(max(xs))))
        y_min = max(0, int(math.floor(min(ys))))
        y_max = min(self.grid_height, int(math.ceil(max(ys))))
        
        # Check each cell in bounding box
        for y in range(y_min, y_max):
            for x in range(x_min, x_max):
                # Use point-in-polygon test
                if self._point_in_polygon(float(x), float(y), vertices_px):
                    self.grid[y][x] = False
    
    def _point_in_polygon(self, x: float, y: float,
                         vertices: List[Tuple[float, float]]) -> bool:
        """
        Check if a point is inside a polygon using ray-casting algorithm.
        
        Args:
            x, y: Point coordinates
            vertices: List of polygon vertices
            
        Returns:
            True if point is inside polygon, False otherwise
        """
        n = len(vertices)
        inside = False
        
        p1x, p1y = vertices[0]
        for i in range(1, n + 1):
            p2x, p2y = vertices[i % n]
            
            if y > min(p1y, p2y):
                if y <= max(p1y, p2y):
                    if x <= max(p1x, p2x):
                        if p1y != p2y:
                            xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y) + p1x
                        if p1x == p2x or x <= xinters:
                            inside = not inside
            
            p1x, p1y = p2x, p2y
        
        return inside
    
    def export_to_bitmap_file(self, output_path: str) -> None:
        """
        Export grid to StaticBitMap-compatible text file.
        
        Format:
        - Line 1: "width height"
        - Lines 2+: Grid data
          - '.' = walkable (True)
          - '#' = obstacle (False)
        
        Args:
            output_path: Path to output file
        """
        print(f"\n[ProhibitedZoneGenerator] Exporting to {output_path}")
        
        try:
            with open(output_path, 'w') as f:
                # Write header with dimensions
                f.write(f"{self.grid_width} {self.grid_height}\n")
                
                # Write grid data
                for row in self.grid:
                    line = "".join("." if cell else "#" for cell in row)
                    f.write(line + "\n")
            
            print(f"[ProhibitedZoneGenerator] Export complete")
            print(f"  File: {output_path}")
            print(f"  Dimensions: {self.grid_width} × {self.grid_height}")
        
        except IOError as e:
            print(f"[ProhibitedZoneGenerator] ERROR: Failed to write file: {e}")
            raise
    
    def export_to_csv(self, output_path: str) -> None:
        """
        Export grid to CSV format for analysis/debugging.
        
        Args:
            output_path: Path to output file
        """
        print(f"\n[ProhibitedZoneGenerator] Exporting to CSV: {output_path}")
        
        try:
            with open(output_path, 'w') as f:
                for row in self.grid:
                    line = ",".join("1" if cell else "0" for cell in row)
                    f.write(line + "\n")
            
            print(f"[ProhibitedZoneGenerator] CSV export complete")
        
        except IOError as e:
            print(f"[ProhibitedZoneGenerator] ERROR: Failed to write file: {e}")
            raise
    
    def export_to_png(self, output_path: str) -> None:
        """
        Export grid to PNG image for visualization.
        
        Requires PIL/Pillow library.
        
        Args:
            output_path: Path to output PNG file
        """
        try:
            from PIL import Image
        except ImportError:
            print("[ProhibitedZoneGenerator] WARNING: PIL/Pillow not installed, skipping PNG export")
            print("  Install with: pip install Pillow")
            return
        
        print(f"\n[ProhibitedZoneGenerator] Exporting to PNG: {output_path}")
        
        # Create image: white = walkable, black = obstacle
        pixels = []
        for row in self.grid:
            pixel_row = []
            for cell in row:
                # Convert bool to RGB: True (white) = (255, 255, 255), False (black) = (0, 0, 0)
                color = (255, 255, 255) if cell else (0, 0, 0)
                pixel_row.append(color)
            pixels.append(pixel_row)
        
        # Create PIL image
        img = Image.new('RGB', (self.grid_width, self.grid_height))
        
        # Set pixel data
        for y, row in enumerate(pixels):
            for x, color in enumerate(row):
                img.putpixel((x, y), color)
        
        # Save
        try:
            img.save(output_path)
            print(f"[ProhibitedZoneGenerator] PNG export complete")
        except IOError as e:
            print(f"[ProhibitedZoneGenerator] ERROR: Failed to write PNG: {e}")
            raise
    
    def get_grid(self) -> List[List[bool]]:
        """Get the generated grid."""
        return self.grid
    
    def get_grid_dimensions(self) -> Tuple[int, int]:
        """Get grid dimensions (width, height) in pixels."""
        return (self.grid_width, self.grid_height)
    
    def get_statistics(self) -> dict:
        """Get grid generation statistics."""
        total = self.grid_width * self.grid_height
        obstacle_ratio = self.obstacle_cells / total if total > 0 else 0
        
        return {
            'grid_width': self.grid_width,
            'grid_height': self.grid_height,
            'total_cells': total,
            'obstacle_cells': self.obstacle_cells,
            'walkable_cells': self.walkable_cells,
            'obstacle_ratio': obstacle_ratio,
            'resolution_m_per_px': self.resolution_m_per_px,
            'robot_radius_m': self.robot_radius_m
        }
    
    def print_statistics(self) -> None:
        """Print generation statistics."""
        stats = self.get_statistics()
        
        print(f"\n{'='*60}")
        print(f"Grid Generation Statistics")
        print(f"{'='*60}")
        print(f"Grid dimensions: {stats['grid_width']} × {stats['grid_height']} pixels")
        print(f"Total cells: {stats['total_cells']}")
        print(f"Obstacle cells: {stats['obstacle_cells']}")
        print(f"Walkable cells: {stats['walkable_cells']}")
        print(f"Obstacle ratio: {100 * stats['obstacle_ratio']:.1f}%")
        print(f"Resolution: {stats['resolution_m_per_px']}m/pixel")
        print(f"Robot radius: {stats['robot_radius_m']}m")
        print(f"{'='*60}\n")


# Example usage
if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python prohibited_zone_generator.py <warehouse_layout.json> [output_map.txt]")
        sys.exit(1)
    
    json_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else "map_layout.txt"
    
    try:
        print("=" * 60)
        print("Prohibited Zone Generator")
        print("=" * 60)
        
        # Load layout
        print(f"\nLoading warehouse layout from: {json_path}")
        loader = WarehouseLayoutLoader(json_path)
        loader.print_summary()
        
        # Generate grid
        print("\nGenerating grid...")
        generator = ProhibitedZoneGenerator(
            loader=loader,
            resolution_m_per_px=0.1,  # DECIMETERS
            robot_radius_m=0.3
        )
        
        grid = generator.generate_grid()
        
        # Export
        generator.export_to_bitmap_file(output_path)
        generator.print_statistics()
        
        # Try to export PNG if PIL available
        try:
            png_path = output_path.replace('.txt', '.png')
            generator.export_to_png(png_path)
        except:
            pass
        
        print("✓ Grid generation complete!")
        
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
