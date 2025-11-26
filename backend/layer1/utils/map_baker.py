from PIL import Image
import sys
import os

def parse_dimensions(dim_str):
    """Parses a string like '30x20' into tuple (30.0, 20.0)."""
    try:
        # Normalize to lowercase 'x'
        parts = dim_str.lower().split('x')
        if len(parts) != 2:
            raise ValueError
        width = float(parts[0])
        height = float(parts[1])
        return width, height
    except ValueError:
        print(f"Error: Invalid dimension format '{dim_str}'. Expected format: WIDTHxHEIGHT (e.g., 30x20)")
        sys.exit(1)

def convert_png_to_grid(input_path, output_path, physical_dims, threshold=128):
    try:
        # 1. Load Image
        if not os.path.exists(input_path):
            print(f"Error: File {input_path} not found.")
            return

        original_img = Image.open(input_path)
        original_img = original_img.convert('L') # Grayscale
        
        real_w_meters, real_h_meters = physical_dims

        print(f"\n--- MAP CONFIGURATION ---")
        print(f"Input Image:      {input_path}")
        print(f"Physical Size:    {real_w_meters}m (Width) x {real_h_meters}m (Height)")
        print(f"Physical Area:    {real_w_meters * real_h_meters} m^2")

        # 2. Define Options based on Backend::Common::Resolution
        # Multiplier = Cells per meter
        options = {
            1: ("METERS", 1.0),         # 1 cell = 1m
            2: ("DECIMETERS", 10.0),    # 1 cell = 0.1m (10 cells/m)
            3: ("CENTIMETERS", 100.0),  # 1 cell = 0.01m
            4: ("MILLIMETERS", 1000.0)  # 1 cell = 0.001m
        }

        print("\n--- SELECT RESOLUTION ---")
        # Header
        print(f"{'Opt':<5} {'Name':<15} {'Grid Size (WxH)':<20} {'Total Cells'}")
        print("-" * 60)

        # Calculate and display grid sizes for each option
        for key, (name, multiplier) in options.items():
            grid_w = int(real_w_meters * multiplier)
            grid_h = int(real_h_meters * multiplier)
            total = grid_w * grid_h
            
            warning = " (!! HEAVY !!)" if total > 10000000 else ""
            print(f"{key:<5} {name:<15} {grid_w}x{grid_h:<15} {total}{warning}")

        # 3. Get User Selection
        try:
            choice_str = input("\nSelect Option (1-4): ")
            choice = int(choice_str)
            if choice not in options:
                print("Invalid option.")
                return
        except ValueError:
            print("Invalid input.")
            return

        selected_name, multiplier = options[choice]
        
        # Final Grid Calculation
        target_w = int(real_w_meters * multiplier)
        target_h = int(real_h_meters * multiplier)

        print(f"\nProcessing as {selected_name}...")
        print(f"Resizing image to grid dimensions: {target_w}x{target_h}...")

        # 4. Resize Image (LANCZOS for quality)
        # This implicitly handles aspect ratio distortion if your PNG doesn't match your meters.
        # e.g. If PNG is square but meters are 30x10, the image will stretch to fit 30x10.
        
        resized_img = original_img.resize((target_w, target_h), Image.Resampling.LANCZOS)
        
        pixels = list(resized_img.getdata())

        # 5. Write to Text File
        with open(output_path, 'w') as f:
            # Header: Width Height
            f.write(f"{target_w} {target_h}\n")
            
            # Grid Data
            for y in range(target_h):
                row_str = ""
                for x in range(target_w):
                    pixel_val = pixels[y * target_w + x]
                    
                    # LOGIC: Dark = Wall (#), Light = Floor (.)
                    if pixel_val < threshold:
                        row_str += "#"
                    else:
                        row_str += "."
                
                f.write(row_str + "\n")
                
        print(f"\nSUCCESS! Map saved to: {output_path}")
        print(f"Grid Dimensions: {target_w} x {target_h}")

    except Exception as e:
        print(f"Critical Error: {e}")

if __name__ == "__main__":
    # Usage Check
    if len(sys.argv) < 4:
        print("Usage: python3 map_baker.py <input_png> <output_txt> <WxH_meters>")
        print("Example: python3 utils/map_baker.py warehouse.png map.txt 30x20")
    else:
        input_file = sys.argv[1]
        output_file = sys.argv[2]
        dimensions = parse_dimensions(sys.argv[3])
        
        convert_png_to_grid(input_file, output_file, dimensions)