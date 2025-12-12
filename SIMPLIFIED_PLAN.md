# Warehouse Layout JSON Integration - SIMPLIFIED Plan

## Goal
**Generate initial warehouse bitmap AND POI (picking/charging stations) from JSON with MINIMAL changes to existing C++ code.**

**Important**: The JSON is used ONLY for initial warehouse setup, not for real-time simulation updates.

## What's Already Done ✅

### Python Utilities (Complete)
1. **`warehouse_layout_loader.py`** - Loads and validates JSON warehouse layouts
2. **`prohibited_zone_generator.py`** - Converts JSON objects to binary grid
3. **`warehouse_setup.py --use-json`** - Command-line tool to generate bitmap from JSON

### Generated Output (Current)
- `backend/layer1/assets/map_layout.txt` - StaticBitMap format (ready to use)
- `backend/layer1/assets/map_layout.png` - Visualization

### What Needs to Be Added
- **POI generation from JSON** - Extract picking/charging stations from object types instead of automatic clustering

## Current Workflow

```bash
# Generate bitmap from JSON
python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout_realistic.json

# Output: 
#   - backend/layer1/assets/map_layout.txt (bitmap)
#   - backend/layer1/assets/map_layout.png (visualization)
# TODO: Also generate poi_config.json from object types
```

## What Needs to Change

### Current POI Generation (Problem)
**Current method**: Automatic clustering algorithm places POIs randomly in accessible areas
- `generate_poi_config()` in `warehouse_setup.py` 
- Uses `generate_clustered_pois()` with random placement
- No connection to actual warehouse objects

**Problem**: POIs don't correspond to actual shelves/charging stations in the warehouse!

### Proposed POI Generation (Solution)
**New method**: Hybrid approach combining explicit zones and automatic detection

**POI Definition Strategy**:
1. **Charging Stations (CHARGING)** - Auto-detected from `modelIndex: 5`
   - Automatically place POI at each charging station object center
   - No manual zone definition needed

2. **Picking Zones (PICKUP/DROPOFF)** - Explicitly defined in JSON
   - Add new `pickingZones` array to JSON (similar to `prohibitedZones`)
   - Each zone defines an area with POI type (PICKUP or DROPOFF)
   - System automatically generates POIs within the zone area

**JSON Structure** (to be added):
```json
{
  "floorSize": [50.0, 40.0],
  "objects": [...],
  "prohibitedZones": [...],
  "pickingZones": [
    {
      "type": "PICKUP",
      "center": [10.0, 0.0, 15.0],
      "dimensions": [8.0, 0.0, 15.0],
      "rotation": 0.0,
      "poiSpacing": 2.0,
      "description": "Main picking area - shelves with inventory"
    },
    {
      "type": "DROPOFF", 
      "center": [30.0, 0.0, 15.0],
      "dimensions": [8.0, 0.0, 15.0],
      "rotation": 0.0,
      "poiSpacing": 2.0,
      "description": "Dropoff area - empty shelves for completed orders"
    }
  ]
}
```

**Model Index Mapping**:
- `modelIndex: 0` - Conveyor belt (not a POI)
- `modelIndex: 1` - Robot (not a POI)
- `modelIndex: 2` - Robot with box (not a POI)
- `modelIndex: 3` - Shelf with boxes (visual only, POI defined by pickingZones)
- `modelIndex: 4` - Shelf without boxes (visual only, POI defined by pickingZones)
- `modelIndex: 5` - **Charging station** → AUTO-GENERATE CHARGING POI

## What Needs to Change (MINIMAL)

### Required Changes

#### 1. Add Picking Zone Support to `warehouse_layout_loader.py`

Add new dataclass for picking zones:

```python
@dataclass
class PickingZone:
    """Represents a picking zone for POI generation."""
    poi_type: str  # "PICKUP" or "DROPOFF"
    center: Tuple[float, float, float]  # Center position (x, y, z) in meters
    dimensions: Tuple[float, float, float]  # Width, height, depth in meters
    rotation: float  # Rotation in degrees
    poi_spacing: float  # Distance between POIs in meters (default 2.0)
    description: str = ""
```

Update `WarehouseLayout` dataclass:

```python
@dataclass
class WarehouseLayout:
    floor_size: Tuple[float, float]  # Width, height in meters
    objects: List[Object]
    prohibited_zones: List[ProhibitedZone]
    picking_zones: List[PickingZone]  # NEW
    robots: List[Robot]
```

Add parsing method to `WarehouseLayoutLoader`:

```python
def _parse_picking_zones(self, zones_data: List[Dict]) -> List[PickingZone]:
    """Parse picking zones from JSON."""
    picking_zones = []
    
    for i, zone_data in enumerate(zones_data):
        try:
            poi_type = zone_data.get("type", "PICKUP")
            if poi_type not in ["PICKUP", "DROPOFF"]:
                self.warnings.append(f"Picking zone {i}: Invalid type '{poi_type}', defaulting to PICKUP")
                poi_type = "PICKUP"
            
            picking_zones.append(PickingZone(
                poi_type=poi_type,
                center=tuple(zone_data["center"]),
                dimensions=tuple(zone_data["dimensions"]),
                rotation=zone_data.get("rotation", 0.0),
                poi_spacing=zone_data.get("poiSpacing", 2.0),
                description=zone_data.get("description", "")
            ))
        except (KeyError, TypeError, ValueError) as e:
            self.errors.append(f"Picking zone {i}: Invalid format - {e}")
    
    return picking_zones
```

#### 2. Add POI Extraction to `warehouse_setup.py`

#### 2. Add POI Extraction to `warehouse_setup.py`

Create new function `extract_pois_from_json()`:

```python
def extract_pois_from_json(
    layout: WarehouseLayout,
    resolution_m_per_px: float,
    grid: List[List[bool]]
) -> Dict:
    """
    Extract POI locations from warehouse layout.
    
    Strategy:
    1. Charging stations: Auto-detect from modelIndex=5 objects
    2. Picking zones: Generate POIs from pickingZones definitions
    """
    charging_pois = []
    pickup_pois = []
    dropoff_pois = []
    
    # 1. Auto-detect charging stations from objects
    for i, obj in enumerate(layout.objects):
        if obj.model_index == 5:  # Charging station
            x_m = obj.center[0]
            z_m = obj.center[2]
            
            x_px = int(x_m / resolution_m_per_px)
            y_px = int(z_m / resolution_m_per_px)
            
            charging_pois.append({
                "id": f"C{len(charging_pois)}",
                "type": "CHARGING",
                "x": x_px,
                "y": y_px,
                "active": True,
                "metadata": {
                    "source": "auto_detected",
                    "model_index": 5,
                    "object_index": i,
                    "center_m": [x_m, z_m]
                }
            })
    
    # 2. Generate POIs from picking zones
    for zone_idx, zone in enumerate(layout.picking_zones):
        # Calculate zone bounds
        x_min_m = zone.center[0] - zone.dimensions[0] / 2
        x_max_m = zone.center[0] + zone.dimensions[0] / 2
        z_min_m = zone.center[2] - zone.dimensions[2] / 2
        z_max_m = zone.center[2] + zone.dimensions[2] / 2
        
        # Generate POIs with spacing
        poi_list = pickup_pois if zone.poi_type == "PICKUP" else dropoff_pois
        prefix = "PU" if zone.poi_type == "PICKUP" else "DO"
        
        # Grid-based POI placement within zone
        x_m = x_min_m + zone.poi_spacing / 2
        while x_m < x_max_m:
            z_m = z_min_m + zone.poi_spacing / 2
            while z_m < z_max_m:
                x_px = int(x_m / resolution_m_per_px)
                y_px = int(z_m / resolution_m_per_px)
                
                # Check if position is walkable
                if (0 <= y_px < len(grid) and 
                    0 <= x_px < len(grid[0]) and 
                    grid[y_px][x_px]):
                    
                    poi_list.append({
                        "id": f"{prefix}{len(poi_list)}",
                        "type": zone.poi_type,
                        "x": x_px,
                        "y": y_px,
                        "active": True,
                        "metadata": {
                            "source": "picking_zone",
                            "zone_index": zone_idx,
                            "zone_description": zone.description,
                            "center_m": [x_m, z_m]
                        }
                    })
                
                z_m += zone.poi_spacing
            x_m += zone.poi_spacing
    
    print(f"[POIExtractor] Generated {len(charging_pois)} CHARGING POIs (auto-detected)")
    print(f"[POIExtractor] Generated {len(pickup_pois)} PICKUP POIs from {len([z for z in layout.picking_zones if z.poi_type == 'PICKUP'])} zones")
    print(f"[POIExtractor] Generated {len(dropoff_pois)} DROPOFF POIs from {len([z for z in layout.picking_zones if z.poi_type == 'DROPOFF'])} zones")
    
    return {
        "description": "POI configuration: charging auto-detected, picking from zones",
        "version": "3.1",
        "coordinate_system": f"pixels ({resolution_m_per_px}m/pixel)",
        "physical_dimensions": {
            "width_m": layout.floor_size[0],
            "height_m": layout.floor_size[1]
        },
        "poi_types": {
            "CHARGING": "Auto-detected from modelIndex=5 objects",
            "PICKUP": "Generated from pickingZones with type=PICKUP",
            "DROPOFF": "Generated from pickingZones with type=DROPOFF"
        },
        "poi": charging_pois + pickup_pois + dropoff_pois
    }
```

#### 3. Modify `setup_warehouse_from_json()`

#### 3. Modify `setup_warehouse_from_json()`

Replace the POI generation section:

```python
# After generating grid...

# Extract POIs from layout (charging auto-detected + picking zones)
print(f"\n[Step 3/3] Extracting POIs from layout...")
poi_config = extract_pois_from_json(layout, resolution_factor, grid)

poi_output_path = assets_dir / "poi_config.json"
with open(poi_output_path, 'w') as f:
    json.dump(poi_config, f, indent=4)
print(f"[POIExtractor] Saved to: {poi_output_path}")

# Generate tasks
print(f"\n[Step 4/4] Generating tasks...")
# ... existing task generation code
```

#### 4. Update JSON Files

Add `pickingZones` section to warehouse layout files:

**Example**: `simulador/warehouse_layout_realistic.json`
```json
{
  "floorSize": [50.0, 40.0],
  "objects": [
    ...existing objects...
  ],
  "prohibitedZones": [
    ...existing zones...
  ],
  "pickingZones": [
    {
      "type": "PICKUP",
      "center": [10.0, 0.0, 15.0],
      "dimensions": [8.0, 0.0, 20.0],
      "rotation": 0.0,
      "poiSpacing": 2.5,
      "description": "Left shelf area - inventory pickup"
    },
    {
      "type": "DROPOFF",
      "center": [20.0, 0.0, 15.0],
      "dimensions": [8.0, 0.0, 20.0],
      "rotation": 0.0,
      "poiSpacing": 2.5,
      "description": "Right shelf area - order dropoff"
    },
    {
      "type": "PICKUP",
      "center": [35.0, 0.0, 20.0],
      "dimensions": [5.0, 0.0, 10.0],
      "rotation": 45.0,
      "poiSpacing": 2.0,
      "description": "Additional pickup area"
    }
  ]
}
```

### Implementation Steps

1. **Add `PickingZone` dataclass** to `warehouse_layout_loader.py` (~20 lines)
2. **Add `_parse_picking_zones()` method** to `WarehouseLayoutLoader` (~30 lines)
3. **Update `_parse_layout()` method** to load picking zones (~5 lines)
4. **Add `extract_pois_from_json()` function** to `warehouse_setup.py` (~80 lines)
5. **Modify `setup_warehouse_from_json()`** to call new function (~3 line change)
6. **Add `pickingZones` to JSON files** (manual editing)
7. **Test with both warehouse layouts**:
   ```bash
   python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout_realistic.json
   python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout.json
   ```

**Total estimated effort**: 1-2 hours

### Expected Output

After modification, the script will generate:
- `backend/layer1/assets/map_layout.txt` - Bitmap (unchanged)
- `backend/layer1/assets/map_layout.png` - Visualization (unchanged)
- `backend/layer1/assets/poi_config.json` - **POIs: charging auto-detected + picking from zones** (NEW!)
- `api/set_of_tasks.json` - Random tasks between pickup/dropoff POIs

**Example POI output**:
```json
{
  "description": "POI configuration: charging auto-detected, picking from zones",
  "poi": [
    {"id": "C0", "type": "CHARGING", "x": 50, "y": 100, "metadata": {"source": "auto_detected"}},
    {"id": "C1", "type": "CHARGING", "x": 50, "y": 300, "metadata": {"source": "auto_detected"}},
    {"id": "PU0", "type": "PICKUP", "x": 102, "y": 52, "metadata": {"source": "picking_zone", "zone_description": "Left shelf area"}},
    {"id": "PU1", "type": "PICKUP", "x": 105, "y": 52, "metadata": {"source": "picking_zone"}},
    {"id": "DO0", "type": "DROPOFF", "x": 202, "y": 52, "metadata": {"source": "picking_zone", "zone_description": "Right shelf area"}}
  ]
}
```

### C++ Backend Changes

**None required!** The C++ backend already reads `poi_config.json` in the current format.

---

## Updated Workflow

### One-Time Setup (Initial Warehouse Configuration)
1. **Design warehouse** in simulator GUI → saves to `warehouse_layout_realistic.json`
   - Place charging stations (modelIndex=5)
   - Place shelves with boxes (modelIndex=3) for pickup
   - Place empty shelves (modelIndex=4) for dropoff
2. **Generate bitmap and POIs** (one-time):
   ```bash
   python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout_realistic.json
   ```
3. **Build backend** with this initial configuration:
   ```bash
   cd backend/layer2
   make clean && make
   ```
4. **Run system** - Backend uses POIs from actual warehouse objects

### When Warehouse Design Changes
1. **Modify warehouse** in simulator GUI → update JSON
   - Add/remove charging stations, shelves, etc.
2. **Regenerate bitmap and POIs**:
   ```bash
   python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout_realistic.json
   ```
3. **Rebuild backend**:
   ```bash
   cd backend/layer2
   make clean && make
   ```

**Note**: During runtime, the warehouse layout and POIs are FIXED. JSON is only used to initialize the system, not for dynamic updates.

---

## Comparison: Old vs New POI Generation

### Old Method (Current - Random Clustering)
```python
# Automatic clustering - no connection to actual objects
poi_config = generate_poi_config(config, accessible_cells)
# Result: POIs placed randomly in walkable areas
```

**Problems:**
- ❌ POIs don't match actual warehouse objects
- ❌ No control over POI placement
- ❌ Charging stations in JSON ignored for POI generation
- ❌ Manual configuration needed (--charging 6 --pickup 16 --dropoff 16)

### New Method (Proposed - Hybrid: Auto + Zones)
```python
# Charging: Auto-detect from objects
# Picking: From explicit zone definitions
poi_config = extract_pois_from_json(layout, resolution_factor, grid)
# Result: Charging at actual stations, picking in defined zones
```

**Benefits:**
- ✅ Charging POIs match actual charging station objects (modelIndex=5)
- ✅ Picking zones give full control over POI placement
- ✅ Similar to prohibitedZones - familiar pattern
- ✅ poiSpacing parameter controls POI density
- ✅ Multiple zones supported (multiple pickup/dropoff areas)
- ✅ Only walkable cells get POIs (automatic filtering)
- ✅ No manual counting needed
- ✅ Flexible zone shapes with rotation support

---

## File Changes Summary

### ✅ Completed (No Further Changes)
- `backend/layer1/utils/prohibited_zone_generator.py` (538 lines)
  - ✅ **Updated**: Charging stations (modelIndex=5) are NOT marked as obstacles
  - Robots can access charging stations freely

### 📝 Needs Modification
- `backend/layer1/utils/warehouse_layout_loader.py`:
  - Add `PickingZone` dataclass (~15 lines)
  - Add `_parse_picking_zones()` method (~30 lines)
  - Update `_parse_layout()` to load picking zones (~5 lines)
  - Update `WarehouseLayout` dataclass (~2 lines)
  - **Subtotal**: ~50 lines
  
- `backend/layer1/utils/warehouse_setup.py`:
  - Add `extract_pois_from_json()` function (~80 lines)
  - Modify `setup_warehouse_from_json()` (~5 line change)
  - **Subtotal**: ~85 lines

- `simulador/*.json` files:
  - Add `pickingZones` section (manual JSON editing)
  - **Estimated effort per file**: 10 minutes

**Total estimated effort**: 1-2 hours coding + JSON editing

### ❌ No Changes Needed
- `backend/layer1/src/StaticBitMap.cc` - Already loads from file
- `backend/layer1/src/InflatedBitMap.cc` - Already inflates bitmap
- `backend/layer1/src/NavMeshGenerator.cc` - Already builds from bitmap
- `backend/layer2/main.cc` - Already reads poi_config.json
- All other C++ files - No changes required

---

## Testing Strategy

### Test 1: Verify POI Extraction
```bash
# Generate from JSON
python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout_realistic.json

# Check POI config
cat backend/layer1/assets/poi_config.json | jq '.poi[] | {id, type, x, y}'

# Verify counts match JSON object counts:
# - Count modelIndex=5 → should equal CHARGING POIs
# - Count modelIndex=3 → should equal PICKUP POIs  
# - Count modelIndex=4 → should equal DROPOFF POIs
```

### Test 2: Backend Integration
```bash
cd backend/layer2
make clean && make
./build/test_layer2

# Verify:
# - Backend loads POI config successfully
# - Robots can reach all POI locations
# - Tasks generated between valid pickup/dropoff pairs
```

### Test 3: Visual Validation
```bash
# Open PNG to verify POI locations visually
xdg-open backend/layer1/assets/map_layout.png

# POIs should be at center of corresponding objects
```

---

## Benefits of This Approach

### ✅ Advantages
1. **Zero C++ changes** - Uses existing infrastructure
2. **Consistent POI placement** - POIs match actual warehouse objects
3. **Automatic POI generation** - No manual counting/clustering needed
4. **Model-based mapping** - Clear correspondence between simulator and backend
5. **Visual validation** - PNG output shows both obstacles and POI locations
6. **Static warehouse** - No runtime complexity, warehouse layout is fixed
7. **Simple workflow** - One JSON defines everything (obstacles + POIs)

### 📊 Use Cases

| Scenario | Process |
|----------|---------|
| Initial setup | Design in simulator → Generate bitmap + POIs → Deploy backend |
| Testing new layout | Modify JSON (add/remove shelves) → Regenerate → Test backend |
| Production deployment | Final JSON → Generate all files → Build & deploy |
| Runtime operation | **No JSON involved** - Backend uses fixed bitmap + POIs |

### 📋 POI Extraction Example

Given this JSON:
```json
{
  "objects": [
    {"modelIndex": 5, "center": [5.0, 0.0, 10.0], ...},  // Charging station
    {"modelIndex": 5, "center": [5.0, 0.0, 30.0], ...},  // Charging station
    {"modelIndex": 3, "center": [10.0, 0.0, 5.0], ...},  // Shelf (visual)
    {"modelIndex": 4, "center": [20.0, 0.0, 5.0], ...}   // Shelf (visual)
  ],
  "pickingZones": [
    {
      "type": "PICKUP",
      "center": [10.0, 0.0, 15.0],
      "dimensions": [8.0, 0.0, 20.0],
      "poiSpacing": 2.5
    },
    {
      "type": "DROPOFF",
      "center": [20.0, 0.0, 15.0],
      "dimensions": [8.0, 0.0, 20.0],
      "poiSpacing": 2.5
    }
  ]
}
```

Generated POI config:
```json
{
  "poi": [
    {"id": "C0", "type": "CHARGING", "x": 50, "y": 100, "metadata": {"source": "auto_detected", "model_index": 5}},
    {"id": "C1", "type": "CHARGING", "x": 50, "y": 300, "metadata": {"source": "auto_detected", "model_index": 5}},
    {"id": "PU0", "type": "PICKUP", "x": 65, "y": 55, "metadata": {"source": "picking_zone"}},
    {"id": "PU1", "type": "PICKUP", "x": 90, "y": 55, "metadata": {"source": "picking_zone"}},
    {"id": "PU2", "type": "PICKUP", "x": 65, "y": 80, "metadata": {"source": "picking_zone"}},
    {"id": "DO0", "type": "DROPOFF", "x": 165, "y": 55, "metadata": {"source": "picking_zone"}},
    {"id": "DO1", "type": "DROPOFF", "x": 190, "y": 55, "metadata": {"source": "picking_zone"}}
  ]
}
```

**Note**: Number of POIs in each zone depends on zone dimensions and `poiSpacing`.

### 📊 Comparison Table

| Aspect | Current (Clustering) | New (Hybrid) |
|--------|---------------------|-------------|
| Charging POI source | Random placement | ✅ Auto from modelIndex=5 |
| Picking POI source | Random placement | ✅ From pickingZones |
| Consistency | ❌ Different each run | ✅ Always matches JSON |
| Configuration | Manual parameters | ✅ Zone definitions in JSON |
| Simulator sync | ❌ Not aligned | ✅ Perfectly aligned |
| Control | Limited | ✅ Full control via zones |
| Pattern | New concept | ✅ Same as prohibitedZones |
| Code changes | 0 (already done) | ~140 lines Python |

---

## Migration Path (If Needed Later)

If you eventually want real-time updates or dynamic warehouse changes:

1. **Phase 1** (Current): JSON → Python → Bitmap → C++ (static layout) ✅
2. **Phase 2** (Future): Add runtime API to modify obstacles
3. **Phase 3** (Future): Add dynamic obstacle updates from simulator
4. **Phase 4** (Optional): Full real-time synchronization

**But Phase 1 (static layout) is the recommended approach for most warehouses!**

---

## Documentation Updates Needed

### Update README files:
- `backend/README.md` - Add JSON bitmap generation section
- `backend/layer1/README.md` - Document warehouse_setup.py usage
- `simulador/README.md` - Explain JSON export for backend

### Example Documentation:

```markdown
## Generating Initial Map from Simulator JSON

The warehouse layout is defined at system startup using a JSON file from the simulator.

**One-time setup process:**

1. Design warehouse layout in simulator GUI
2. Export layout to `warehouse_layout_realistic.json`
3. Generate bitmap for backend:
   ```bash
   python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout_realistic.json
   ```
4. Build and run backend:
   ```bash
   cd backend/layer2
   make clean && make
   ./build/test_layer2
   ```

**Important**: The JSON is only used for initial setup. During runtime, the warehouse 
layout is fixed and stored in `map_layout.txt`. To change the warehouse, regenerate 
the bitmap and rebuild the backend.
```

---

## Troubleshooting

### Issue: "No POIs generated"
**Solution**: Check that JSON has:
- Charging stations (modelIndex=5)
- pickingZones section
```bash
# Check charging stations
jq '.objects[] | select(.modelIndex == 5)' simulador/warehouse_layout_realistic.json

# Check picking zones
jq '.pickingZones' simulador/warehouse_layout_realistic.json
```

### Issue: "Too few POIs in zone"
**Solution**: Increase zone dimensions or reduce poiSpacing
```json
{
  "poiSpacing": 1.5,  // Reduce from 2.5 to get more POIs
  "dimensions": [10.0, 0.0, 25.0]  // Increase zone size
}
```

### Issue: "POIs placed on obstacles"
**Solution**: The algorithm automatically filters non-walkable cells. If this happens:
- Check grid generation is correct
- Verify zone doesn't overlap with obstacles
- Adjust zone center/dimensions

### Issue: "Charging POIs missing"
**Solution**: Ensure objects have modelIndex=5
```bash
# Count charging stations
jq '[.objects[] | select(.modelIndex == 5)] | length' simulador/warehouse_layout_realistic.json
```

---

## Next Steps

### Immediate (Required for POI extraction)
- [ ] Add `PickingZone` dataclass to `warehouse_layout_loader.py`
- [ ] Add `_parse_picking_zones()` method to `WarehouseLayoutLoader`
- [ ] Update `WarehouseLayout` to include `picking_zones` field
- [ ] Implement `extract_pois_from_json()` function in `warehouse_setup.py`
- [ ] Modify `setup_warehouse_from_json()` to use new POI extraction
- [ ] Add `pickingZones` section to `warehouse_layout_realistic.json`
- [ ] Add `pickingZones` section to `warehouse_layout.json`
- [ ] Test with both warehouse JSON files
- [ ] Verify charging POI count matches modelIndex=5 count
- [ ] Verify picking POIs are within zone bounds and walkable

### Optional Improvements
- [ ] Add zone rotation support for diagonal picking areas
- [ ] Add POI visualization to PNG output (colored dots per type)
- [ ] Create validation script to check zone coverage
- [ ] Support multiple POIs per shelf (front/back access)

---

## Conclusion

**Current status**: Bitmap generation from JSON is working ✅

**Next step**: Add hybrid POI extraction (auto-charging + picking zones) - estimated 1-2 hours

**Typical workflow after implementation:**
1. **Design warehouse** in simulator
   - Place charging stations (modelIndex=5) - POIs auto-generated
   - Place shelves (any modelIndex) for visual representation
   - Define pickingZones for PICKUP/DROPOFF areas (like prohibitedZones)
   
2. **Example JSON structure:**
```json
{
  "floorSize": [50.0, 40.0],
  "objects": [
    {"modelIndex": 5, "center": [5, 0, 10], ...},  // Auto → CHARGING POI
    {"modelIndex": 3, "center": [10, 0, 15], ...}  // Visual only
  ],
  "prohibitedZones": [...],  // Areas robots can't enter
  "pickingZones": [          // Areas where to place PICKUP/DROPOFF POIs
    {"type": "PICKUP", "center": [10, 0, 15], "dimensions": [8, 0, 20], "poiSpacing": 2.5},
    {"type": "DROPOFF", "center": [30, 0, 15], "dimensions": [8, 0, 20], "poiSpacing": 2.5}
  ]
}
```

3. **Generate bitmap + POIs** from JSON (one time)
   ```bash
   python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout_realistic.json
   ```
   
4. **Run backend** with warehouse layout and POIs perfectly synchronized with simulator

The JSON is only for **initial warehouse configuration**, not runtime updates. Once generated, both the warehouse layout and POI locations are static during operation.

**Benefits of this approach:**
- ✅ **Charging stations**: Automatically detected (no manual definition)
- ✅ **Picking zones**: Explicit control (like prohibitedZones)
- ✅ **Flexible**: Multiple zones, custom spacing, rotation support
- ✅ **Consistent**: Same pattern as prohibitedZones (familiar)
- ✅ **Safe**: Only walkable cells get POIs (automatic filtering)

**Command to remember:**
```bash
python3 backend/layer1/utils/warehouse_setup.py --use-json simulador/warehouse_layout_realistic.json
```

Perfect synchronization between simulator and backend! 🎉
