# Modification Plan: Object-Based Prohibited Zones from JSON

## Goal
Convert the system from using **map image files** to define prohibited zones, to using **JSON object configurations** (from the simulator) to define prohibited zones. This allows the backend to communicate directly with the simulator without relying on image processing.

---

## Architecture Overview

### Current Flow (Image-Based)
```
Warehouse Image (.png/.jpg)
    ↓
image_to_map() [warehouse_setup.py]
    ↓
StaticBitMap (binary grid)
    ↓
InflatedBitMap (add robot radius safety margin)
    ↓
NavMeshGenerator (creates navigation graph)
    ↓
POI Generator (generates picking/charging zones)
```

### Target Flow (JSON-Based)
```
warehouse_layout_realistic.json (objects array)
    ↓
WarehouseLayoutLoader (new)
    ↓
ProhibitedZoneGenerator (new - converts objects to binary grid)
    ↓
StaticBitMap (binary grid)
    ↓
InflatedBitMap (add robot radius safety margin)
    ↓
NavMeshGenerator (creates navigation graph)
    ↓
POI Loader from JSON [existing poi_config.json]
```

---

## Files to Modify/Create

### 1. **Python Utilities** (Layer 1 Utils)

#### 1.1 Create: `backend/layer1/utils/warehouse_layout_loader.py`
**Purpose**: Load `warehouse_layout_realistic.json` and convert objects to binary grid

**Functions to implement**:
```python
class WarehouseLayoutLoader:
    def __init__(self, json_path: str, floor_width: float, floor_height: float, resolution: float)
    def load_layout(self) -> Dict
    def get_objects(self) -> List[Dict]
    def validate_objects(self) -> bool
    
class ProhibitedZoneGenerator:
    def __init__(self, layout_data: Dict, resolution: float, robot_radius: float)
    def convert_objects_to_grid(self) -> Tuple[List[List[bool]], Tuple[int, int]]
    def inflate_object_zones(self, safety_margin: float = None) -> List[List[bool]]
    def export_to_bitmap_file(self, filepath: str) -> None
    def visualize_zones(self, output_path: str = None) -> None
```

**Key features**:
- Load floor size from `floorSize` field
- Load objects array with `center`, `dimensions`, `rotation`
- Convert 3D object coordinates to 2D grid cells
- Support rotation of rectangular objects
- Generate binary grid (True = walkable, False = obstacle)
- Export as `.txt` format compatible with `StaticBitMap::LoadFromFile()`

#### 1.2 Modify: `backend/layer1/utils/warehouse_setup.py`
**Changes**:
- Add command-line option `--use-json` to use JSON instead of image
- Add function `setup_warehouse_from_json()` that:
  - Calls `WarehouseLayoutLoader` 
  - Calls `ProhibitedZoneGenerator`
  - Generates POIs using existing `poi_generator.py`
  - Creates all necessary output files

**New functions**:
```python
def setup_warehouse_from_json(json_path: str, config: WarehouseConfig) -> None
def load_warehouse_layout_from_json(json_path: str) -> Dict
```

#### 1.3 Create: `backend/layer1/utils/json_to_bitmap.py`
**Purpose**: Command-line utility to convert JSON layout to bitmap

**Script entry point**:
```bash
python json_to_bitmap.py --json warehouse_layout_realistic.json \
                         --width 50 --height 40 \
                         --resolution 0.1 \
                         --output assets/map_layout.txt
```

**Functions**:
```python
def main(json_path: str, width: float, height: float, resolution: float, output: str)
```

---

### 2. **C++ Backend Layer 1** (Map/Mesh Generation)

#### 2.1 Create: `backend/layer1/include/WarehouseLayoutLoader.hh`
**Purpose**: C++ wrapper to load and parse warehouse JSON layout

**Class**:
```cpp
class WarehouseLayoutLoader {
private:
    struct Object {
        int modelIndex;
        Coordinates center;  // [x, y, z]
        std::tuple<float, float, float> dimensions;  // [w, h, d]
        float rotation;
    };
    
    std::vector<Object> objects;
    std::pair<float, float> floorSize;  // [width, height]
    
public:
    WarehouseLayoutLoader(const std::string& jsonPath);
    
    // Parse and validate JSON
    bool LoadFromJSON(const std::string& jsonPath);
    
    // Accessors
    const std::vector<Object>& GetObjects() const;
    std::pair<float, float> GetFloorSize() const;
    
    // Validation
    bool ValidateObjectCoordinates() const;
};
```

#### 2.2 Create: `backend/layer1/include/ProhibitedZoneGenerator.hh`
**Purpose**: Convert objects to binary grid (C++ implementation)

**Class**:
```cpp
class ProhibitedZoneGenerator {
private:
    const WarehouseLayoutLoader& loader;
    Resolution resolution;
    float robotRadiusMeters;
    
    // Grid dimensions in pixels
    int gridWidth;
    int gridHeight;
    
    // Binary grid (true = walkable, false = obstacle)
    std::vector<bool> gridData;
    
    // Helper methods
    bool IsPointInObstacle(float worldX, float worldY) const;
    void MarkRectangleObstacle(float centerX, float centerY, 
                               float width, float height, float rotationDegrees);
    void MarkCircleObstacle(float centerX, float centerY, float radius);
    
public:
    ProhibitedZoneGenerator(const WarehouseLayoutLoader& loader,
                           Resolution res, float robotRadius);
    
    // Main conversion
    void GenerateGrid();
    
    // Export
    void ExportToBitmapFile(const std::string& filepath) const;
    void ExportToCSV(const std::string& filepath) const;
    
    // Statistics
    int GetAccessibleCells() const;
    int GetObstacleCells() const;
    
    // Get raw grid for StaticBitMap
    const std::vector<bool>& GetGridData() const;
    std::pair<int, int> GetGridDimensions() const;
};
```

#### 2.3 Modify: `backend/layer1/include/StaticBitMap.hh`
**Changes**:
- Add static factory method to create from object list instead of file

**New functions**:
```cpp
// Load from pre-computed grid (instead of file)
void LoadFromGrid(const std::vector<bool>& gridData, int width, int height);

// Factory: Create from JSON warehouse layout
static StaticBitMap CreateFromLayout(const std::string& jsonPath,
                                     Resolution res, float robotRadiusMeters);
```

#### 2.4 Modify: `backend/layer1/src/StaticBitMap.cc`
**Changes**:
- Implement `LoadFromGrid()` method
- Implement `CreateFromLayout()` factory
- These will use the new `WarehouseLayoutLoader` and `ProhibitedZoneGenerator`

**Implementation logic**:
```cpp
StaticBitMap StaticBitMap::CreateFromLayout(const std::string& jsonPath,
                                            Resolution res, 
                                            float robotRadiusMeters) {
    // 1. Load warehouse layout from JSON
    WarehouseLayoutLoader loader(jsonPath);
    
    // 2. Calculate grid dimensions from floor size
    auto floorSize = loader.GetFloorSize();
    int pixelsPerMeter = (res == Resolution::DECIMETERS) ? 10 : 100;
    int width = floorSize.first * pixelsPerMeter;
    int height = floorSize.second * pixelsPerMeter;
    
    // 3. Create StaticBitMap with calculated dimensions
    StaticBitMap map(width, height, res);
    
    // 4. Generate prohibited zones from objects
    ProhibitedZoneGenerator generator(loader, res, robotRadiusMeters);
    generator.GenerateGrid();
    
    // 5. Load grid data
    map.LoadFromGrid(generator.GetGridData(), 
                     generator.GetGridDimensions().first,
                     generator.GetGridDimensions().second);
    
    return map;
}
```

---

### 3. **C++ Backend Layer 1 - Main Integration**

#### 3.1 Modify: `backend/layer1/main.cc`
**Changes**:
- Add new test case to validate JSON-based loading
- Add command-line argument handling for JSON input
- Keep existing image-based test for backwards compatibility

**New code**:
```cpp
// TEST 1B: Load StaticBitMap from JSON warehouse layout
PrintHeader("TEST 1B: Static Map Loading from JSON Layout");

try {
    StaticBitMap staticMapFromJSON = StaticBitMap::CreateFromLayout(
        "assets/warehouse_layout_realistic.json",
        Resolution::DECIMETERS,
        0.3f  // Robot radius
    );
    
    PrintPass("StaticBitMap loaded from warehouse_layout_realistic.json");
    passedTests++;
    
    // Export for visualization
    // (add export capability to StaticBitMap)
    
} catch (const std::exception& e) {
    PrintFail("Failed to load from JSON: " + std::string(e.what()));
}
totalTests++;
```

#### 3.2 Create: `backend/layer1/Makefile` (or update existing)
**Changes**:
- Add new source files to compilation:
  ```makefile
  SOURCES += src/WarehouseLayoutLoader.cc
  SOURCES += src/ProhibitedZoneGenerator.cc
  ```

---

### 4. **Python Utilities - Existing Files to Update**

#### 4.1 Modify: `backend/layer1/utils/scenario_generator.py`
**Changes**:
- Update to use JSON layout if available instead of image
- Add parameter `--layout-json` for JSON input

#### 4.2 Modify: `backend/layer1/utils/map_baker.py` (if exists)
**Changes**:
- Update to document JSON layout as input option

---

### 5. **Configuration & Setup Files**

#### 5.1 Create: `backend/layer1/assets/warehouse_layout_schema.json`
**Purpose**: JSON Schema for validating warehouse layouts

**Schema**:
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "floorSize": {
      "type": "array",
      "items": {"type": "number"},
      "minItems": 2,
      "maxItems": 2,
      "description": "Floor width and height in meters"
    },
    "objects": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "modelIndex": {"type": "integer"},
          "center": {
            "type": "array",
            "items": {"type": "number"},
            "minItems": 3,
            "maxItems": 3
          },
          "dimensions": {
            "type": "array",
            "items": {"type": "number"},
            "minItems": 3,
            "maxItems": 3
          },
          "rotation": {"type": "number"}
        },
        "required": ["modelIndex", "center", "dimensions"]
      }
    }
  },
  "required": ["floorSize", "objects"]
}
```

#### 5.2 Update: `backend/system_config.json`
**Changes**:
- Add new field to specify warehouse input source:
  ```json
  {
    "warehouse_source": "json",  // "image" or "json"
    "warehouse_json_path": "assets/warehouse_layout_realistic.json",
    "warehouse_image_path": "assets/warehouse2.png"
  }
  ```

---

## Detailed Implementation Steps

### Phase 1: Python Implementation (Recommended to start here)

1. **Create `warehouse_layout_loader.py`**
   - Parse JSON structure
   - Validate object coordinates against floor size
   - Handle 3D-to-2D projection (use center x,y, ignore z)

2. **Create `prohibited_zone_generator.py`**
   - Convert objects to obstacle grid
   - Handle rectangular obstacles with rotation (use `shapely` library or custom rotation)
   - Generate binary grid format compatible with C++ `StaticBitMap`

3. **Update `warehouse_setup.py`**
   - Add `--use-json` flag
   - Create new pipeline for JSON-based setup

4. **Create `json_to_bitmap.py`**
   - CLI wrapper for quick conversion
   - Useful for testing

### Phase 2: C++ Implementation

1. **Create `WarehouseLayoutLoader.hh/cc`**
   - Simple JSON parsing (use manual parsing like POIRegistry)
   - Validate floor size and object coordinates

2. **Create `ProhibitedZoneGenerator.hh/cc`**
   - Core logic to convert objects to grid
   - Handle rotation and projection

3. **Modify `StaticBitMap.hh/cc`**
   - Add `LoadFromGrid()` method
   - Add `CreateFromLayout()` factory method

4. **Update `layer1/main.cc`**
   - Add test case for JSON-based loading
   - Verify output matches expected grid

### Phase 3: Integration & Validation

1. Test with `warehouse_layout_realistic.json`
2. Compare generated grids with image-based approach
3. Verify NavMesh generation works identically
4. Verify POI loading still works
5. Update documentation

---

## Key Design Decisions

### 1. **2D Projection**
- Use only `center.x` and `center.y` from warehouse layout JSON
- Ignore `z` coordinate (objects are on ground)
- Convert to grid pixel coordinates: `pixel = world_coord / resolution_meters_per_pixel`

### 2. **Object Shape Handling**
- Rectangles: Use `dimensions[0]` (width) and `dimensions[1]` (height)
- Ignore 3D depth (`dimensions[2]`), assume flat on ground
- Rotation: Apply 2D rotation to rectangle vertices before rasterizing

### 3. **Grid Generation**
- Binary grid: `true` = walkable, `false` = obstacle
- Each pixel = 0.1m (DECIMETERS) resolution
- Grid size: `ceil(floor_width / 0.1) x ceil(floor_height / 0.1)`

### 4. **Compatibility**
- Output format must match existing `StaticBitMap` binary format
- Must work seamlessly with existing `NavMeshGenerator`
- Must work seamlessly with existing `InflatedBitMap`

---

## Data Flow Example

### Input (from Simulator)
```json
{
  "floorSize": [50.0, 40.0],
  "objects": [
    {
      "modelIndex": 0,
      "center": [2.0, 0.0, 5.0],
      "dimensions": [3.0, 0.8, 20.0],
      "rotation": 0.0
    }
  ]
}
```

### Processing
1. Floor size: 50m × 40m
2. Grid size: 500 × 400 pixels (at 0.1m/pixel)
3. Object 1: Rectangle at (20, 50) px, size 30×200 px, rotation 0°
4. Mark those pixels as obstacles
5. Output binary grid

### Output (StaticBitMap format)
```
500 400
.................X.................
.................X.................
...
(binary grid representation)
```

---

## Testing Strategy

### Unit Tests
- Test JSON parsing with valid/invalid layouts
- Test object-to-grid conversion for various shapes
- Test rotation logic
- Test grid export/import

### Integration Tests
- Load JSON layout → generate grid → create NavMesh
- Compare with image-based approach
- Verify POI generation still works
- Verify pathfinding works end-to-end

### Validation Tests
- Verify object coordinates are within floor bounds
- Verify grid connectivity (no isolated walkable regions)
- Verify inflation still works correctly on JSON-generated grid

---

## Files Summary Table

| File | Type | Action | Purpose |
|------|------|--------|---------|
| `warehouse_layout_loader.py` | Python | CREATE | Load and parse JSON warehouse layout |
| `prohibited_zone_generator.py` | Python | CREATE | Convert objects to binary grid |
| `json_to_bitmap.py` | Python | CREATE | CLI utility for quick conversion |
| `warehouse_setup.py` | Python | MODIFY | Add JSON pipeline support |
| `WarehouseLayoutLoader.hh` | C++ Header | CREATE | C++ JSON loader |
| `WarehouseLayoutLoader.cc` | C++ Source | CREATE | C++ JSON loader implementation |
| `ProhibitedZoneGenerator.hh` | C++ Header | CREATE | C++ grid generator |
| `ProhibitedZoneGenerator.cc` | C++ Source | CREATE | C++ grid generator implementation |
| `StaticBitMap.hh` | C++ Header | MODIFY | Add `LoadFromGrid()` and factory |
| `StaticBitMap.cc` | C++ Source | MODIFY | Implement new methods |
| `layer1/main.cc` | C++ Source | MODIFY | Add JSON test case |
| `layer1/Makefile` | Makefile | MODIFY | Add new source files |
| `warehouse_layout_schema.json` | JSON Schema | CREATE | Validation schema |
| `system_config.json` | Config | MODIFY | Add warehouse source config |

---

## Summary

Total modifications needed: **17 files** (11 new, 6 modified)

The implementation preserves backwards compatibility with the image-based approach while adding the ability to use JSON object definitions from the simulator. Both approaches ultimately produce identical `StaticBitMap` and `NavMesh` outputs.
