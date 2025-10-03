#!/bin/bash
# filepath: /mnt/c/Users/sterr/Desktop/Mecalux-AMR/apps/simulator/list_contents.sh

echo "Directory Structure:"
tree

echo -e "\nFile Contents:"
find . -type f ! -path "*/node_modules/*" ! -path "*/dist/*" -print0 | while IFS= read -r -d '' file; do
    echo -e "\n=== File: $file ==="
    echo "----------------------------------------"
    cat "$file"
    echo "----------------------------------------"
done