#!/usr/bin/env python3
"""Test script to verify picking zones are loaded correctly."""

import sys
sys.path.append('backend/layer1/utils')

from warehouse_layout_loader import WarehouseLayoutLoader

# Load the warehouse layout
loader = WarehouseLayoutLoader("simulador/warehouse_layout.json")
layout = loader.get_layout()

print("="*60)
print("WAREHOUSE LAYOUT TEST")
print("="*60)

print(f"\nFloor size: {layout.floor_size[0]}m × {layout.floor_size[1]}m")
print(f"Objects: {len(layout.objects)}")
print(f"Prohibited zones: {len(layout.prohibited_zones)}")
print(f"Picking zones: {len(layout.picking_zones)}")
print(f"Robots: {len(layout.robots)}")

if layout.picking_zones:
    print(f"\n{'='*60}")
    print(f"PICKING ZONES ({len(layout.picking_zones)})")
    print(f"{'='*60}")
    for i, zone in enumerate(layout.picking_zones, 1):
        print(f"\nZone {i}:")
        print(f"  Type: {zone.poi_type}")
        print(f"  Center: {zone.center}")
        print(f"  Dimensions: {zone.dimensions}")
        print(f"  Rotation: {zone.rotation}°")
        print(f"  POI Spacing: {zone.poi_spacing}m")
        if zone.description:
            print(f"  Description: {zone.description}")

print(f"\n{'='*60}")
if loader.warnings:
    print(f"\nWarnings ({len(loader.warnings)}):")
    for warning in loader.warnings:
        print(f"  - {warning}")
else:
    print("\n✓ No warnings")

if loader.errors:
    print(f"\nErrors ({len(loader.errors)}):")
    for error in loader.errors:
        print(f"  - {error}")
else:
    print("✓ No errors")

print(f"\n{'='*60}")
print("TEST COMPLETE")
print("="*60)
