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

    std::pair<int, int> StaticBitMap::GetFileDimensions(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("[StaticBitMap] Failed to open file: " + filepath);
        }

        std::string line;
        int fileHeight = 0;
        int fileWidth = 0;
        
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            // Width is the length of the first non-empty line
            if (fileWidth == 0) {
                fileWidth = static_cast<int>(line.size());
            }
            fileHeight++;
        }
        
        file.close();
        return {fileWidth, fileHeight};
    }

    StaticBitMap StaticBitMap::CreateFromFile(const std::string& filepath, 
                                               Backend::Common::Resolution res) {
        auto [w, h] = GetFileDimensions(filepath);
        std::cout << "[StaticBitMap] Detected map dimensions: " << w << "x" << h << std::endl;
        
        StaticBitMap map(w, h, res);
        map.LoadFromFile(filepath);
        return map;
    }

    void StaticBitMap::LoadFromFile(const std::string& filepath) {
        std::cout << "[StaticBitMap] Loading map layout from: " << filepath << std::endl;
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("[StaticBitMap] Failed to open file: " + filepath);
        }

        // Read the grid data row by row (no header line - pure grid format)
        std::string line;
        int y = 0;
        int walkable = 0;
        int obstacles = 0;
        
        while (std::getline(file, line) && y < height) {
            if (line.empty()) continue;  // Skip empty lines
            
            for (int x = 0; x < width && x < (int)line.size(); ++x) {
                char c = line[x];
                // '.' = walkable (true), '#' = obstacle (false)
                bool isWalkable = (c == '.');
                gridData[y * width + x] = isWalkable;
                if (isWalkable) walkable++;
                else obstacles++;
            }
            ++y;
        }

        file.close();
        
        std::cout << "[StaticBitMap] Loaded " << width << "x" << height << " map. "
                  << "Walkable: " << walkable << ", Obstacles: " << obstacles << std::endl;
    }

    const std::vector<bool>& StaticBitMap::GetRawData() const {
        return gridData;
    }

} // namespace Layer1
} // namespace Backend