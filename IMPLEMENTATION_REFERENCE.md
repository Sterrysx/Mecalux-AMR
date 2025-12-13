# Summary: Files & Functions to Modify for JSON-Based Warehouse Loading

## Executive Summary

To enable the backend to generate prohibited zones from a JSON warehouse layout (instead of from a map image), you need to modify/create **15 files** across Python utilities and C++ backend code.

The key insight: **Replace image-based obstacle detection with object-based grid generation**, while keeping all downstream systems identical.

---

## ONE-PAGE REFERENCE

### 🟢 NEW Python Files (in `backend/layer1/utils/`)

| File | Purpose | Key Classes |
|------|---------|-------------|
| `warehouse_layout_loader.py` | Parse JSON warehouse layout | `WarehouseLayoutLoader` |
| `prohibited_zone_generator.py` | Convert objects to binary grid | `ProhibitedZoneGenerator` |
| `json_to_bitmap.py` | CLI utility for quick testing | `main()` entry point |

### 🔵 MODIFIED Python Files (in `backend/layer1/utils/`)

| File | Changes |
|------|---------|
| `warehouse_setup.py` | Add `setup_warehouse_from_json()` function<br>Add `--use-json` CLI flag<br>Integrate with POI generation |

### 🟢 NEW C++ Files (in `backend/layer1/`)

| File | Purpose | Key Classes |
|------|---------|-------------|
| `include/WarehouseLayoutLoader.hh` | Declare C++ JSON loader | `WarehouseLayoutLoader` |
| `src/WarehouseLayoutLoader.cc` | Implement JSON parsing | Parse, validate layout |
| `include/ProhibitedZoneGenerator.hh` | Declare C++ grid generator | `ProhibitedZoneGenerator` |
| `src/ProhibitedZoneGenerator.cc` | Implement grid generation | Object→pixel conversion |

### 🔵 MODIFIED C++ Files (in `backend/layer1/`)

| File | Changes |
|------|---------|
| `include/StaticBitMap.hh` | Add 2 new methods:<br>• `void LoadFromGrid(...)`<br>• `static StaticBitMap CreateFromLayout(...)` |
| `src/StaticBitMap.cc` | Implement the 2 new methods |
| `main.cc` | Add TEST 1B for JSON loading<br>Add argument parsing |
| `Makefile` | Add 2 new .cc files to compilation |

### 🟢 NEW Config Files (in `backend/`)

| File | Purpose |
|------|---------|
| `layer1/assets/warehouse_layout_schema.json` | JSON Schema for validation |

### 🔵 MODIFIED Config Files (in `backend/`)

| File | Changes |
|------|---------|
| `system_config.json` | Add `warehouse_source` field: "json" or "image"<br>Add `warehouse_json_path` field |

---

## DETAILED FUNCTION LISTS

### Python Functions to Create

#### `warehouse_layout_loader.py`
```python
class WarehouseLayoutLoader:
    def __init__(self, json_path: str)
    def load_layout(self) -> Dict
    def get_objects(self) -> List[Dict]
    def get_floor_size(self) -> Tuple[float, float]
    def validate_objects(self) -> bool
```

#### `prohibited_zone_generator.py`
```python
class ProhibitedZoneGenerator:
    def __init__(self, layout_data: Dict, resolution_m_per_px: float, robot_radius_m: float)
    def generate_grid(self) -> Tuple[List[List[bool]], Tuple[int, int]]
    def convert_objects_to_grid(self)
    def export_to_bitmap_file(self, filepath: str)
    def get_grid_dimensions(self) -> Tuple[int, int]
    def get_accessible_cells(self) -> int
    def get_obstacle_cells(self) -> int
    
    def _object_to_pixels(self, obj_center, obj_dims, rotation) -> List[Tuple[int, int]]
    def _rasterize_rectangle(self, cx, cy, w, h, rotation)
    def _rotate_point(self, x, y, cx, cy, angle_deg) -> Tuple[float, float]
    def _point_in_polygon(self, x, y, polygon) -> bool
```

#### `json_to_bitmap.py`
```python
def main(args)
def parse_arguments()
```

#### `warehouse_setup.py` - New Functions
```python
def setup_warehouse_from_json(json_path: str, config: WarehouseConfig)
def load_warehouse_layout_from_json(json_path: str) -> Dict
```

### C++ Functions to Create

#### `WarehouseLayoutLoader.hh/cc`
```cpp
class WarehouseLayoutLoader {
private:
    struct Object { int modelIndex; Coordinates center; Dimensions dims; float rotation; };
    std::vector<Object> objects;
    std::pair<float, float> floorSize;

public:
    WarehouseLayoutLoader(const std::string& jsonPath);
    bool LoadFromJSON(const std::string& jsonPath);
    const std::vector<Object>& GetObjects() const;
    std::pair<float, float> GetFloorSize() const;
    bool ValidateObjectCoordinates() const;
    
private:
    std::string extractStringValue(const std::string& json, const std::string& key);
    int extractIntValue(const std::string& json, const std::string& key);
    float extractFloatValue(const std::string& json, const std::string& key);
};
```

#### `ProhibitedZoneGenerator.hh/cc`
```cpp
class ProhibitedZoneGenerator {
private:
    const WarehouseLayoutLoader& loader;
    Resolution resolution;
    float robotRadiusMeters;
    int gridWidth, gridHeight;
    std::vector<bool> gridData;

public:
    ProhibitedZoneGenerator(const WarehouseLayoutLoader& loader, 
                           Resolution res, float robotRadius);
    
    void GenerateGrid();
    void ExportToBitmapFile(const std::string& filepath) const;
    void ExportToCSV(const std::string& filepath) const;
    
    const std::vector<bool>& GetGridData() const;
    std::pair<int, int> GetGridDimensions() const;
    int GetAccessibleCells() const;
    int GetObstacleCells() const;

private:
    bool IsPointInObstacle(float worldX, float worldY) const;
    void MarkRectangleObstacle(float cx, float cy, float w, float h, float rotation);
    void MarkCircleObstacle(float cx, float cy, float radius);
    void RasterizeRotatedRectangle(float cx, float cy, float w, float h, float rotation);
    
    std::pair<int, int> WorldToPixel(float worldX, float worldY) const;
    std::vector<std::pair<float, float>> GetRectangleVertices(float cx, float cy, 
                                                               float w, float h, 
                                                               float rotation) const;
    bool PointInPolygon(float x, float y, 
                       const std::vector<std::pair<float, float>>& polygon) const;
};
```

### C++ Functions to Add to StaticBitMap

#### `StaticBitMap.hh` - New Method Declarations
```cpp
// Load pre-computed grid instead of from file
void LoadFromGrid(const std::vector<bool>& gridData, int width, int height);

// Factory method: Create from warehouse JSON layout
static StaticBitMap CreateFromLayout(const std::string& jsonPath,
                                     Resolution res, 
                                     float robotRadiusMeters);
```

#### `StaticBitMap.cc` - New Method Implementations
```cpp
void StaticBitMap::LoadFromGrid(const std::vector<bool>& gridData, 
                               int width, int height) {
    // 1. Set width/height from parameters
    // 2. Copy gridData to internal storage
    // 3. Validate grid integrity
}

StaticBitMap StaticBitMap::CreateFromLayout(const std::string& jsonPath,
                                           Resolution res,
                                           float robotRadiusMeters) {
    // 1. Load warehouse layout from JSON
    WarehouseLayoutLoader loader(jsonPath);
    
    // 2. Calculate grid dimensions from floor size
    auto floorSize = loader.GetFloorSize();
    int pixelsPerMeter = GetPixelsPerMeter(res);
    int width = std::ceil(floorSize.first * pixelsPerMeter);
    int height = std::ceil(floorSize.second * pixelsPerMeter);
    
    // 3. Create StaticBitMap instance
    StaticBitMap map(width, height, res);
    
    // 4. Generate prohibited zones
    ProhibitedZoneGenerator generator(loader, res, robotRadiusMeters);
    generator.GenerateGrid();
    
    // 5. Load grid data
    map.LoadFromGrid(generator.GetGridData(), 
                     generator.GetGridDimensions().first,
                     generator.GetGridDimensions().second);
    
    return map;
}
```

### C++ Functions to Modify in main.cc

#### Add new test case
```cpp
// TEST 1B: Load StaticBitMap from JSON warehouse layout
PrintHeader("TEST 1B: Static Map Loading from JSON Layout");

try {
    StaticBitMap staticMapFromJSON = StaticBitMap::CreateFromLayout(
        "assets/warehouse_layout_realistic.json",
        Resolution::DECIMETERS,
        0.3f
    );
    
    PrintPass("StaticBitMap loaded from JSON");
    passedTests++;
    
    // Verify it has valid dimensions
    auto dims = staticMapFromJSON.GetDimensions();
    std::cout << "  Grid dimensions: " << dims.first << " x " << dims.second << std::endl;
    
} catch (const std::exception& e) {
    PrintFail("Failed to load from JSON: " + std::string(e.what()));
}
totalTests++;
```

---

## Implementation Checklist

### Phase 1: Python Implementation
- [ ] Create `warehouse_layout_loader.py`
  - [ ] JSON parsing
  - [ ] Object extraction
  - [ ] Validation
  
- [ ] Create `prohibited_zone_generator.py`
  - [ ] Grid initialization
  - [ ] Object-to-grid conversion
  - [ ] Rectangle rasterization
  - [ ] Rotation handling
  - [ ] Bitmap export
  
- [ ] Create `json_to_bitmap.py`
  - [ ] CLI argument parsing
  - [ ] Integration with above classes
  
- [ ] Modify `warehouse_setup.py`
  - [ ] Add `--use-json` flag
  - [ ] Add `setup_warehouse_from_json()` function
  - [ ] Test with existing POI generation

### Phase 2: C++ Header Files
- [ ] Create `WarehouseLayoutLoader.hh`
  - [ ] Class declaration
  - [ ] Method signatures
  
- [ ] Create `ProhibitedZoneGenerator.hh`
  - [ ] Class declaration
  - [ ] Method signatures
  
- [ ] Modify `StaticBitMap.hh`
  - [ ] Add `LoadFromGrid()` method
  - [ ] Add `CreateFromLayout()` factory

### Phase 3: C++ Implementation
- [ ] Implement `WarehouseLayoutLoader.cc`
  - [ ] JSON parsing (manual, no external deps)
  - [ ] Object validation
  
- [ ] Implement `ProhibitedZoneGenerator.cc`
  - [ ] Grid generation logic
  - [ ] Rasterization algorithm
  - [ ] Rotation calculations
  
- [ ] Modify `StaticBitMap.cc`
  - [ ] Implement `LoadFromGrid()`
  - [ ] Implement `CreateFromLayout()`

### Phase 4: Build & Integration
- [ ] Modify `Makefile`
  - [ ] Add new source files
  - [ ] Update include paths if needed
  
- [ ] Modify `main.cc`
  - [ ] Add TEST 1B
  - [ ] Add argument parsing
  
- [ ] Compile and verify no errors

### Phase 5: Testing
- [ ] Test Python pipeline
  - [ ] Load warehouse_layout_realistic.json
  - [ ] Generate grid
  - [ ] Export to bitmap
  
- [ ] Test C++ pipeline
  - [ ] Load from JSON
  - [ ] Create NavMesh
  - [ ] Verify POI loading works
  
- [ ] Integration test
  - [ ] End-to-end: JSON → NavMesh → Layer 2

### Phase 6: Configuration
- [ ] Create `warehouse_layout_schema.json`
- [ ] Update `system_config.json`
- [ ] Update documentation

---

## Key Implementation Notes

### 1. **No Breaking Changes**
- All existing image-based code remains functional
- New code is purely additive
- Both pipelines produce identical StaticBitMap/NavMesh outputs

### 2. **JSON Parsing in C++**
- Use manual string parsing (like POIRegistry does)
- No external JSON libraries required
- Minimize dependencies

### 3. **Coordinate Systems**
- Input: meters (from warehouse_layout_realistic.json)
- Internal: pixels (0.1m per pixel for DECIMETERS)
- Output: pixel grid

### 4. **Grid Generation**
- Binary grid: `true` = walkable, `false` = obstacle
- Same format as existing `StaticBitMap` file format
- Fully compatible with `InflatedBitMap` and `NavMeshGenerator`

### 5. **Error Handling**
- Throw exceptions for invalid JSON
- Log warnings for out-of-bounds objects
- Validate floor size before grid allocation

---

## Success Criteria

✅ JSON warehouse layout loads without errors
✅ Grid generated matches expected dimensions
✅ Grid export format compatible with StaticBitMap
✅ NavMesh generation works on new grid
✅ POI loading still functional
✅ End-to-end test: JSON → NavMesh → POIs → Layer 2
✅ Backwards compatibility maintained (image-based still works)

