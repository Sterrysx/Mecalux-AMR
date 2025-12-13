# Quick Reference: Files to Modify

## PYTHON UTILITIES (Layer 1)
Located: `backend/layer1/utils/`

### NEW FILES TO CREATE:
1. ✅ `warehouse_layout_loader.py`
   - Class: `WarehouseLayoutLoader`
   - Loads JSON warehouse layout
   - Parses objects array
   
2. ✅ `prohibited_zone_generator.py`
   - Class: `ProhibitedZoneGenerator`
   - Converts objects to binary grid
   - Exports to bitmap file
   
3. ✅ `json_to_bitmap.py`
   - CLI script: `python json_to_bitmap.py --json <path> --output <path>`
   - Quick utility for testing

### FILES TO MODIFY:
4. ✅ `warehouse_setup.py`
   - Add function: `setup_warehouse_from_json()`
   - Add CLI flag: `--use-json`
   - Integrate with existing POI generation

---

## C++ BACKEND (Layer 1)
Located: `backend/layer1/`

### NEW FILES TO CREATE:

5. ✅ `include/WarehouseLayoutLoader.hh`
   - Class: `WarehouseLayoutLoader`
   - Parse JSON layout in C++
   - Validate object coordinates

6. ✅ `src/WarehouseLayoutLoader.cc`
   - Implementation of above

7. ✅ `include/ProhibitedZoneGenerator.hh`
   - Class: `ProhibitedZoneGenerator`
   - Convert objects to binary grid
   - Methods for object-to-pixel mapping

8. ✅ `src/ProhibitedZoneGenerator.cc`
   - Implementation of above

### FILES TO MODIFY:

9. ✅ `include/StaticBitMap.hh`
   - Add: `void LoadFromGrid(const std::vector<bool>& gridData, int w, int h)`
   - Add: `static StaticBitMap CreateFromLayout(const std::string& jsonPath, Resolution res, float robotRadius)`

10. ✅ `src/StaticBitMap.cc`
    - Implement `LoadFromGrid()` - loads pre-computed grid
    - Implement `CreateFromLayout()` - orchestrates loading and grid generation

11. ✅ `main.cc`
    - Add TEST 1B for JSON-based loading
    - Add command-line argument parsing
    - Verify JSON output matches expected format

12. ✅ `Makefile`
    - Add `src/WarehouseLayoutLoader.cc` to `SOURCES`
    - Add `src/ProhibitedZoneGenerator.cc` to `SOURCES`
    - Add `include/` flags if needed

---

## CONFIGURATION FILES

Located: `backend/`

### NEW FILES TO CREATE:

13. ✅ `layer1/assets/warehouse_layout_schema.json`
    - JSON Schema for validating warehouse layouts
    - Defines required fields and types

### FILES TO MODIFY:

14. ✅ `system_config.json`
    - Add field: `"warehouse_source": "json"` or `"image"`
    - Add field: `"warehouse_json_path"`
    - Keep backward compatibility with image path

---

## SUMMARY TABLE

| Category | Count | Action |
|----------|-------|--------|
| Python NEW | 3 | Create warehouse_layout_loader.py, prohibited_zone_generator.py, json_to_bitmap.py |
| Python MODIFY | 1 | Update warehouse_setup.py |
| C++ Headers NEW | 2 | Create WarehouseLayoutLoader.hh, ProhibitedZoneGenerator.hh |
| C++ Source NEW | 2 | Create WarehouseLayoutLoader.cc, ProhibitedZoneGenerator.cc |
| C++ Headers MODIFY | 1 | Update StaticBitMap.hh |
| C++ Source MODIFY | 4 | Update StaticBitMap.cc, main.cc, Makefile, and potentially other files |
| Config NEW | 1 | Create warehouse_layout_schema.json |
| Config MODIFY | 1 | Update system_config.json |
| **TOTAL** | **15 FILES** | **8 NEW + 7 MODIFY** |

---

## RECOMMENDED IMPLEMENTATION ORDER

### Phase 1: Python (Fastest feedback loop)
1. Create `warehouse_layout_loader.py`
2. Create `prohibited_zone_generator.py`
3. Create `json_to_bitmap.py` for testing
4. Test with `warehouse_layout_realistic.json`
5. Update `warehouse_setup.py` to integrate

### Phase 2: C++ Core
6. Create `WarehouseLayoutLoader.hh/cc`
7. Create `ProhibitedZoneGenerator.hh/cc`
8. Update `StaticBitMap.hh/cc` to add new methods
9. Update `Makefile` to compile new files

### Phase 3: Integration & Testing
10. Update `layer1/main.cc` with JSON test case
11. Verify NavMesh generation works
12. Verify POI loading still works
13. Create `warehouse_layout_schema.json`
14. Update `system_config.json`

---

## KEY DEPENDENCIES

```
warehouse_layout_realistic.json
    ↓
WarehouseLayoutLoader (C++ or Python)
    ↓
ProhibitedZoneGenerator (C++ or Python)
    ↓
Binary Grid (vector<bool>)
    ↓
StaticBitMap::LoadFromGrid()
    ↓
InflatedBitMap (existing - no changes needed)
    ↓
NavMeshGenerator (existing - no changes needed)
    ↓
POIRegistry (existing - no changes needed)
```

---

## MINIMAL VIABLE PRODUCT (MVP)

To get a working prototype quickly, implement only:

**Python**:
- ✅ `warehouse_layout_loader.py` (minimal)
- ✅ `prohibited_zone_generator.py` (minimal)
- ✅ `json_to_bitmap.py` script

**C++**:
- ✅ `StaticBitMap::LoadFromGrid()` method (simple)
- ✅ Update `main.cc` to test grid loading

This gives you a working end-to-end pipeline for JSON → binary grid → NavMesh.

Then iterate with:
- Full C++ JSON parsing classes
- Validation and error handling
- Rotation and complex shapes
- Configuration integration

---

## TESTING CHECKLIST

- [ ] Python loads warehouse_layout_realistic.json
- [ ] Python generates correct grid dimensions
- [ ] Python exports grid in StaticBitMap format
- [ ] Python output matches image-based approach
- [ ] C++ loads grid from Python-generated file
- [ ] C++ NavMesh generation works on new grid
- [ ] C++ POI loading still works
- [ ] End-to-end: JSON → NavMesh → POIs
- [ ] Rotation handling works correctly
- [ ] Edge cases: objects touching boundaries
- [ ] Backward compatibility: image-based still works

