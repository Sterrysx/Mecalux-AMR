#include "StaticBitMap.hh"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Backend {
namespace Layer1 {

    StaticBitMap::StaticBitMap(int w, int h, Backend::Common::Resolution res)
        : AbstractGrid(w, h, res) {
        // Initialize all true (walkable)
        gridData.resize(w * h, true);
    }

    bool StaticBitMap::IsAccessible(Backend::Common::Coordinates coords) const {
        if (!IsWithinBounds(coords)) return false;
        return gridData[coords.y * width + coords.x];
    }

    void StaticBitMap::LoadFromFile(const std::string& filepath) {
        std::cout << "[StaticBitMap] Loading map layout from: " << filepath << std::endl;
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("[StaticBitMap] Failed to open file: " + filepath);
        }

        // Read dimensions from first line
        int fileWidth, fileHeight;
        file >> fileWidth >> fileHeight;
        file.ignore(); // Skip the newline after dimensions

        if (fileWidth != width || fileHeight != height) {
            std::cout << "[StaticBitMap] Warning: File dimensions (" << fileWidth << "x" << fileHeight 
                      << ") differ from initialized dimensions (" << width << "x" << height << ")." << std::endl;
            // Resize to match file
            width = fileWidth;
            height = fileHeight;
            gridData.resize(width * height, true);
        }

        // Read the grid data row by row
        std::string line;
        int y = 0;
        while (std::getline(file, line) && y < height) {
            for (int x = 0; x < width && x < (int)line.size(); ++x) {
                char c = line[x];
                // '.' = walkable (true), '#' = obstacle (false)
                gridData[y * width + x] = (c == '.');
            }
            ++y;
        }

        file.close();
        
        // Count walkable cells for stats
        int walkable = 0;
        for (bool cell : gridData) {
            if (cell) walkable++;
        }
        
        std::cout << "[StaticBitMap] Loaded " << width << "x" << height << " map. "
                  << "Walkable cells: " << walkable << " / " << (width * height) << std::endl;
    }

    const std::vector<bool>& StaticBitMap::GetRawData() const {
        return gridData;
    }

} // namespace Layer1
} // namespace Backend