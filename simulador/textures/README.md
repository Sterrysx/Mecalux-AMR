# Warehouse Floor Texture

This directory contains the textures used for the warehouse floor in the simulator.

## Files

- `warehouse_floor.png` - Standard resolution (512x512) warehouse floor texture
- `warehouse_floor_hires.png` - High resolution (1024x1024) warehouse floor texture
- `generate_floor_texture.py` - Python script to regenerate the textures

## Texture Features

The warehouse floor texture includes:
- Concrete-like appearance with random noise
- Grid lines representing floor tiles (every 64 pixels)
- Section markers with thicker lines (every 256 pixels)
- Random wear and tear spots for realism

## Regenerating Textures

To regenerate the textures with different parameters:

```bash
python3 generate_floor_texture.py
```

You can modify the script parameters:
- `size`: Image size in pixels (default: 512 or 1024)
- `tile_size`: Size of each floor tile (default: 64 or 128)

## Usage in Simulator

The simulator will automatically load `warehouse_floor.png` from this directory when it starts. If the texture file is not found, a procedural texture will be generated at runtime instead.

The texture is applied to the floor with proper lighting and repeats across the warehouse floor for a seamless appearance.
