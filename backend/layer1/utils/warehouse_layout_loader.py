#!/usr/bin/env python3
"""
Warehouse Layout Loader for Mecalux AMR

Loads and parses warehouse layout JSON files containing:
- Floor dimensions
- Static objects (shelves, conveyor belts, etc.)
- Prohibited zones (safety buffers, restricted areas)
- Optional robot initial positions

Usage:
    from warehouse_layout_loader import WarehouseLayoutLoader
    
    loader = WarehouseLayoutLoader("warehouse_layout_realistic.json")
    floor_size = loader.get_floor_size()
    objects = loader.get_objects()
    prohibited_zones = loader.get_prohibited_zones()
"""

import json
from pathlib import Path
from typing import List, Dict, Tuple, Optional, Any
from dataclasses import dataclass, field


@dataclass
class Object:
    """Represents a warehouse object (shelf, conveyor, etc.)"""
    model_index: int
    center: Tuple[float, float, float]  # (x, y, z) in meters
    dimensions: Tuple[float, float, float]  # (width, height, depth) in meters
    rotation: float  # degrees
    
    def __repr__(self):
        return (f"Object(model={self.model_index}, "
                f"center={self.center}, "
                f"dims={self.dimensions}, "
                f"rotation={self.rotation}°)")


@dataclass
class ProhibitedZone:
    """Represents a prohibited/restricted zone"""
    zone_id: str
    zone_type: str  # "obstacle_buffer", "restricted_area", etc.
    center: Tuple[float, float]  # (x, y) in meters
    shape: str  # "rectangle", "circle", etc.
    dimensions: Tuple[float, ...] # Depends on shape
    rotation: float  # degrees
    description: str = ""
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def __repr__(self):
        return (f"ProhibitedZone(id={self.zone_id}, "
                f"type={self.zone_type}, "
                f"center={self.center}, "
                f"shape={self.shape})")

@dataclass
class PickingZone:
    """Represents a picking zone for POI generation."""
    poi_type: str  # "PICKUP" or "DROPOFF"
    center: Tuple[float, float, float]  # Center position (x, y, z) in meters
    dimensions: Tuple[float, float, float]  # Width, height, depth in meters
    rotation: float  # Rotation in degrees
    poi_spacing: float  # Distance between POIs in meters (default 2.0)
    description: str = ""

@dataclass
class Robot:
    """Represents a robot initial position"""
    robot_id: int
    x: float
    y: float
    angle: float  # degrees
    
    def __repr__(self):
        return f"Robot(id={self.robot_id}, pos=({self.x}, {self.y}), angle={self.angle}°)"


@dataclass
class WarehouseLayout:
    """Complete warehouse layout structure"""
    floor_size: Tuple[float, float]  # (width, height) in meters
    objects: List[Object]
    prohibited_zones: List[ProhibitedZone] = field(default_factory=list)
    picking_zones: List[PickingZone] = field(default_factory=list)
    robots: List[Robot] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def __repr__(self):
        return (f"WarehouseLayout(floor={self.floor_size}m, "
                f"objects={len(self.objects)}, "
                f"zones={len(self.prohibited_zones)}, "
                f"picking_zones={len(self.picking_zones)}, "
                f"robots={len(self.robots)})")


class WarehouseLayoutLoader:
    """
    Loads and parses warehouse layout from JSON configuration.
    
    Validates:
    - Floor size is positive
    - Object coordinates are within bounds (with tolerance)
    - Object dimensions are positive
    - Prohibited zones don't overlap excessively
    - All required fields are present
    """
    
    def __init__(self, json_path: str):
        """
        Initialize loader with path to JSON file.
        
        Args:
            json_path: Path to warehouse_layout_*.json file
            
        Raises:
            FileNotFoundError: If JSON file doesn't exist
            json.JSONDecodeError: If JSON is invalid
        """
        self.json_path = Path(json_path)
        self.layout: Optional[WarehouseLayout] = None
        self.errors: List[str] = []
        self.warnings: List[str] = []
        
        if not self.json_path.exists():
            raise FileNotFoundError(f"Warehouse layout file not found: {json_path}")
        
        # Load and parse immediately
        self._load_json()
    
    def _load_json(self) -> None:
        """Load and parse JSON file."""
        try:
            with open(self.json_path, 'r') as f:
                data = json.load(f)
        except json.JSONDecodeError as e:
            raise json.JSONDecodeError(
                f"Invalid JSON in {self.json_path}: {e.msg}",
                e.doc, e.pos
            )
        
        self._parse_layout(data)
    
    def _parse_layout(self, data: Dict) -> None:
        """
        Parse layout data from JSON dictionary.
        
        Args:
            data: Dictionary parsed from JSON
        """
        # Extract floor size
        if "floorSize" not in data:
            raise ValueError("Missing required field: 'floorSize'")
        
        floor_size = self._parse_floor_size(data["floorSize"])
        
        # Extract objects
        if "objects" not in data:
            raise ValueError("Missing required field: 'objects'")
        
        objects = self._parse_objects(data["objects"])
        
        # Extract prohibited zones (optional)
        prohibited_zones = []
        if "prohibitedZones" in data:
            prohibited_zones = self._parse_prohibited_zones(data["prohibitedZones"])
        
        # Extract picking zones (optional)
        picking_zones = []
        if "pickingZones" in data:
            picking_zones = self._parse_picking_zones(data["pickingZones"])
        
        # Extract robots (optional)
        robots = []
        if "robots" in data:
            robots = self._parse_robots(data["robots"])
        
        # Extract metadata (optional)
        metadata = data.get("metadata", {})
        
        # Create layout object
        self.layout = WarehouseLayout(
            floor_size=floor_size,
            objects=objects,
            prohibited_zones=prohibited_zones,
            picking_zones=picking_zones,
            robots=robots,
            metadata=metadata
        )
        
        # Validate after parsing
        self._validate_layout()
    
    def _parse_floor_size(self, floor_size_data: Any) -> Tuple[float, float]:
        """
        Parse floor size from JSON.
        
        Args:
            floor_size_data: Should be [width, height]
            
        Returns:
            Tuple of (width, height) in meters
            
        Raises:
            ValueError: If invalid format
        """
        try:
            if not isinstance(floor_size_data, list) or len(floor_size_data) != 2:
                raise ValueError("floorSize must be [width, height]")
            
            width, height = float(floor_size_data[0]), float(floor_size_data[1])
            
            if width <= 0 or height <= 0:
                raise ValueError("Floor dimensions must be positive")
            
            return (width, height)
        except (TypeError, ValueError) as e:
            raise ValueError(f"Invalid floorSize: {e}")
    
    def _parse_objects(self, objects_data: Any) -> List[Object]:
        """
        Parse objects array from JSON.
        
        Args:
            objects_data: Array of object dictionaries
            
        Returns:
            List of Object instances
        """
        objects = []
        
        if not isinstance(objects_data, list):
            raise ValueError("'objects' must be an array")
        
        for i, obj_data in enumerate(objects_data):
            try:
                obj = self._parse_single_object(obj_data, i)
                objects.append(obj)
            except ValueError as e:
                self.errors.append(f"Error parsing object {i}: {e}")
                continue
        
        if not objects:
            raise ValueError("No valid objects found in layout")
        
        return objects
    
    def _parse_single_object(self, obj_data: Dict, index: int) -> Object:
        """
        Parse a single object from JSON.
        
        Args:
            obj_data: Object dictionary
            index: Index in array (for error messages)
            
        Returns:
            Object instance
            
        Raises:
            ValueError: If required fields missing or invalid
        """
        # Parse model index
        if "modelIndex" not in obj_data:
            raise ValueError(f"Missing 'modelIndex'")
        
        try:
            model_index = int(obj_data["modelIndex"])
        except (TypeError, ValueError):
            raise ValueError(f"Invalid 'modelIndex': must be integer")
        
        # Parse center coordinates
        if "center" not in obj_data:
            raise ValueError(f"Missing 'center'")
        
        center_data = obj_data["center"]
        if not isinstance(center_data, list) or len(center_data) != 3:
            raise ValueError(f"'center' must be [x, y, z]")
        
        try:
            center = (float(center_data[0]), float(center_data[1]), float(center_data[2]))
        except (TypeError, ValueError):
            raise ValueError(f"'center' values must be numeric")
        
        # Parse dimensions
        if "dimensions" not in obj_data:
            raise ValueError(f"Missing 'dimensions'")
        
        dims_data = obj_data["dimensions"]
        if not isinstance(dims_data, list) or len(dims_data) != 3:
            raise ValueError(f"'dimensions' must be [width, height, depth]")
        
        try:
            dimensions = (float(dims_data[0]), float(dims_data[1]), float(dims_data[2]))
            if any(d <= 0 for d in dimensions):
                raise ValueError("All dimensions must be positive")
        except (TypeError, ValueError) as e:
            raise ValueError(f"'dimensions' error: {e}")
        
        # Parse rotation (optional, default 0)
        rotation = float(obj_data.get("rotation", 0.0))
        
        return Object(
            model_index=model_index,
            center=center,
            dimensions=dimensions,
            rotation=rotation
        )
    
    def _parse_prohibited_zones(self, zones_data: Any) -> List[ProhibitedZone]:
        """
        Parse prohibited zones array from JSON.
        
        Args:
            zones_data: Array of zone dictionaries
            
        Returns:
            List of ProhibitedZone instances
        """
        zones = []
        
        if not isinstance(zones_data, list):
            self.warnings.append("'prohibitedZones' must be an array, skipping")
            return zones
        
        for i, zone_data in enumerate(zones_data):
            try:
                zone = self._parse_single_zone(zone_data, i)
                zones.append(zone)
            except ValueError as e:
                self.warnings.append(f"Warning parsing prohibited zone {i}: {e}")
                continue
        
        return zones
    
    def _parse_single_zone(self, zone_data: Dict, index: int) -> ProhibitedZone:
        """
        Parse a single prohibited zone from JSON.
        
        Args:
            zone_data: Zone dictionary
            index: Index in array
            
        Returns:
            ProhibitedZone instance
        """
        # Parse zone ID
        zone_id = str(zone_data.get("id", f"zone_{index}"))
        
        # Parse zone type
        zone_type = str(zone_data.get("type", "obstacle_buffer"))
        
        # Parse center coordinates
        if "center" not in zone_data:
            raise ValueError(f"Missing 'center'")
        
        center_data = zone_data["center"]
        if not isinstance(center_data, list) or len(center_data) != 2:
            raise ValueError(f"'center' must be [x, y] (2D)")
        
        try:
            center = (float(center_data[0]), float(center_data[1]))
        except (TypeError, ValueError):
            raise ValueError(f"'center' values must be numeric")
        
        # Parse shape
        shape = str(zone_data.get("shape", "rectangle"))
        
        # Parse dimensions
        if "dimensions" not in zone_data:
            raise ValueError(f"Missing 'dimensions'")
        
        dims_data = zone_data["dimensions"]
        try:
            dimensions = tuple(float(d) for d in dims_data)
            if any(d <= 0 for d in dimensions):
                raise ValueError("All dimensions must be positive")
        except (TypeError, ValueError) as e:
            raise ValueError(f"'dimensions' error: {e}")
        
        # Parse rotation (optional)
        rotation = float(zone_data.get("rotation", 0.0))
        
        # Parse description (optional)
        description = str(zone_data.get("description", ""))
        
        # Parse metadata (optional)
        metadata = zone_data.get("metadata", {})
        
        return ProhibitedZone(
            zone_id=zone_id,
            zone_type=zone_type,
            center=center,
            shape=shape,
            dimensions=dimensions,
            rotation=rotation,
            description=description,
            metadata=metadata
        )
    
    def _parse_picking_zones(self, zones_data: Any) -> List[PickingZone]:
        """
        Parse picking zones array from JSON.
        
        Args:
            zones_data: Array of picking zone dictionaries
            
        Returns:
            List of PickingZone instances
        """
        zones = []
        
        if not isinstance(zones_data, list):
            self.warnings.append("'pickingZones' must be an array, skipping")
            return zones
        
        for i, zone_data in enumerate(zones_data):
            try:
                zone = self._parse_single_picking_zone(zone_data, i)
                zones.append(zone)
            except ValueError as e:
                self.warnings.append(f"Warning parsing picking zone {i}: {e}")
                continue
        
        return zones
    
    def _parse_single_picking_zone(self, zone_data: Dict, index: int) -> PickingZone:
        """
        Parse a single picking zone from JSON.
        
        Args:
            zone_data: Picking zone dictionary
            index: Index in array
            
        Returns:
            PickingZone instance
        """
        # Parse POI type (PICKUP or DROPOFF)
        poi_type = str(zone_data.get("type", "PICKUP")).upper()
        if poi_type not in ["PICKUP", "DROPOFF"]:
            raise ValueError(f"'type' must be 'PICKUP' or 'DROPOFF', got '{poi_type}'")
        
        # Parse center coordinates (3D)
        if "center" not in zone_data:
            raise ValueError(f"Missing 'center'")
        
        center_data = zone_data["center"]
        if not isinstance(center_data, list) or len(center_data) != 3:
            raise ValueError(f"'center' must be [x, y, z] (3D)")
        
        try:
            center = (float(center_data[0]), float(center_data[1]), float(center_data[2]))
        except (TypeError, ValueError):
            raise ValueError(f"'center' values must be numeric")
        
        # Parse dimensions (3D)
        if "dimensions" not in zone_data:
            raise ValueError(f"Missing 'dimensions'")
        
        dims_data = zone_data["dimensions"]
        if not isinstance(dims_data, list) or len(dims_data) != 3:
            raise ValueError(f"'dimensions' must be [width, height, depth] (3D)")
        
        try:
            dimensions = (float(dims_data[0]), float(dims_data[1]), float(dims_data[2]))
            if dimensions[0] <= 0 or dimensions[2] <= 0:  # Width and depth must be positive
                raise ValueError("Width and depth dimensions must be positive")
        except (TypeError, ValueError) as e:
            raise ValueError(f"'dimensions' error: {e}")
        
        # Parse rotation (optional)
        rotation = float(zone_data.get("rotation", 0.0))
        
        # Parse POI spacing (optional, default 2.0 meters)
        poi_spacing = float(zone_data.get("poiSpacing", 2.0))
        if poi_spacing <= 0:
            raise ValueError("'poiSpacing' must be positive")
        
        # Parse description (optional)
        description = str(zone_data.get("description", ""))
        
        return PickingZone(
            poi_type=poi_type,
            center=center,
            dimensions=dimensions,
            rotation=rotation,
            poi_spacing=poi_spacing,
            description=description
        )
    
    def _parse_robots(self, robots_data: Any) -> List[Robot]:
        """
        Parse robots array from JSON.
        
        Args:
            robots_data: Array of robot dictionaries
            
        Returns:
            List of Robot instances
        """
        robots = []
        
        if not isinstance(robots_data, list):
            self.warnings.append("'robots' must be an array, skipping")
            return robots
        
        for i, robot_data in enumerate(robots_data):
            try:
                robot = self._parse_single_robot(robot_data, i)
                robots.append(robot)
            except ValueError as e:
                self.warnings.append(f"Warning parsing robot {i}: {e}")
                continue
        
        return robots
    
    def _parse_single_robot(self, robot_data: Dict, index: int) -> Robot:
        """
        Parse a single robot from JSON.
        
        Args:
            robot_data: Robot dictionary
            index: Index in array
            
        Returns:
            Robot instance
        """
        # Parse robot ID
        if "id" not in robot_data:
            robot_id = index
        else:
            try:
                robot_id = int(robot_data["id"])
            except (TypeError, ValueError):
                raise ValueError("'id' must be an integer")
        
        # Parse position
        if "x" not in robot_data or "y" not in robot_data:
            raise ValueError("Missing 'x' or 'y'")
        
        try:
            x = float(robot_data["x"])
            y = float(robot_data["y"])
        except (TypeError, ValueError):
            raise ValueError("'x' and 'y' must be numeric")
        
        # Parse angle (optional)
        angle = float(robot_data.get("angle", 0.0))
        
        return Robot(robot_id=robot_id, x=x, y=y, angle=angle)
    
    def _validate_layout(self) -> None:
        """Validate the parsed layout for consistency."""
        if self.layout is None:
            raise ValueError("Layout not parsed")
        
        floor_width, floor_height = self.layout.floor_size
        
        # Check if objects are within reasonable bounds
        # (Allow some tolerance for objects that extend beyond bounds)
        tolerance = 2.0  # meters
        
        for obj in self.layout.objects:
            x, y, z = obj.center
            
            # Warn if object center is far outside bounds
            if x < -tolerance or x > floor_width + tolerance:
                self.warnings.append(
                    f"Object model {obj.model_index} center x={x} "
                    f"is outside floor bounds [0, {floor_width}]"
                )
            
            if y < -tolerance or y > floor_height + tolerance:
                self.warnings.append(
                    f"Object model {obj.model_index} center y={y} "
                    f"is outside floor bounds [0, {floor_height}]"
                )
        
        # Check if prohibited zones are within bounds
        for zone in self.layout.prohibited_zones:
            x, y = zone.center
            
            if x < -tolerance or x > floor_width + tolerance:
                self.warnings.append(
                    f"Prohibited zone '{zone.zone_id}' center x={x} "
                    f"is outside floor bounds [0, {floor_width}]"
                )
            
            if y < -tolerance or y > floor_height + tolerance:
                self.warnings.append(
                    f"Prohibited zone '{zone.zone_id}' center y={y} "
                    f"is outside floor bounds [0, {floor_height}]"
                )
    
    # Public API
    
    def get_layout(self) -> WarehouseLayout:
        """Get the parsed layout."""
        if self.layout is None:
            raise RuntimeError("Layout not loaded")
        return self.layout
    
    def get_floor_size(self) -> Tuple[float, float]:
        """Get floor dimensions (width, height) in meters."""
        if self.layout is None:
            raise RuntimeError("Layout not loaded")
        return self.layout.floor_size
    
    def get_objects(self) -> List[Object]:
        """Get list of warehouse objects."""
        if self.layout is None:
            raise RuntimeError("Layout not loaded")
        return self.layout.objects
    
    def get_prohibited_zones(self) -> List[ProhibitedZone]:
        """Get list of prohibited zones."""
        if self.layout is None:
            raise RuntimeError("Layout not loaded")
        return self.layout.prohibited_zones
    
    def get_picking_zones(self) -> List[PickingZone]:
        """Get list of picking zones."""
        if self.layout is None:
            raise RuntimeError("Layout not loaded")
        return self.layout.picking_zones
    
    def get_robots(self) -> List[Robot]:
        """Get list of robot initial positions."""
        if self.layout is None:
            raise RuntimeError("Layout not loaded")
        return self.layout.robots
    
    def get_errors(self) -> List[str]:
        """Get list of errors encountered during parsing."""
        return self.errors
    
    def get_warnings(self) -> List[str]:
        """Get list of warnings encountered during parsing."""
        return self.warnings
    
    def validate(self) -> bool:
        """
        Validate the loaded layout.
        
        Returns:
            True if valid (no critical errors), False otherwise
        """
        return len(self.errors) == 0
    
    def print_summary(self) -> None:
        """Print a summary of the loaded layout."""
        if self.layout is None:
            print("No layout loaded")
            return
        
        print(f"\n{'='*60}")
        print(f"Warehouse Layout Summary: {self.json_path.name}")
        print(f"{'='*60}")
        
        print(f"\nFloor Size: {self.layout.floor_size[0]} × {self.layout.floor_size[1]} m")
        
        print(f"\nObjects ({len(self.layout.objects)}):")
        for obj in self.layout.objects:
            print(f"  - Model {obj.model_index}: center={obj.center}, "
                  f"dims={obj.dimensions}, rot={obj.rotation}°")
        
        if self.layout.prohibited_zones:
            print(f"\nProhibited Zones ({len(self.layout.prohibited_zones)}):")
            for zone in self.layout.prohibited_zones:
                print(f"  - {zone.zone_id} ({zone.zone_type}): "
                      f"center={zone.center}, {zone.shape}")
        
        if self.layout.picking_zones:
            print(f"\nPicking Zones ({len(self.layout.picking_zones)}):")
            for zone in self.layout.picking_zones:
                print(f"  - {zone.poi_type}: center={zone.center}, "
                      f"dims={zone.dimensions}, spacing={zone.poi_spacing}m")
                if zone.description:
                    print(f"    Description: {zone.description}")
        
        if self.layout.robots:
            print(f"\nRobots ({len(self.layout.robots)}):")
            for robot in self.layout.robots:
                print(f"  - Robot {robot.robot_id}: pos=({robot.x}, {robot.y}), "
                      f"angle={robot.angle}°")
        
        if self.warnings:
            print(f"\nWarnings ({len(self.warnings)}):")
            for warning in self.warnings:
                print(f"  ⚠ {warning}")
        
        if self.errors:
            print(f"\nErrors ({len(self.errors)}):")
            for error in self.errors:
                print(f"  ✗ {error}")
        
        print(f"{'='*60}\n")


# Example usage
if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python warehouse_layout_loader.py <warehouse_layout.json>")
        sys.exit(1)
    
    json_path = sys.argv[1]
    
    try:
        loader = WarehouseLayoutLoader(json_path)
        loader.print_summary()
        
        if not loader.validate():
            print("ERROR: Layout validation failed!")
            sys.exit(1)
        
        print("✓ Layout loaded and validated successfully")
        
    except Exception as e:
        print(f"ERROR: {e}")
        sys.exit(1)
