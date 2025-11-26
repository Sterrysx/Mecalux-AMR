#include "InflatedBitMap.hh"
#include "Resolution.hh"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>

namespace Backend {
namespace Layer1 {

    InflatedBitMap::InflatedBitMap(const StaticBitMap& source, float robotRadiusMeters)
        : AbstractGrid(source.GetDimensions().first, 
                       source.GetDimensions().second, 
                       source.GetResolution()),
          sourceMap(&source) {
        
        // Calculate inflation radius in pixels based on resolution
        double metersPerPixel = Backend::Common::GetConversionFactorToMeters(resolution);
        
        // Convert robot radius from meters to pixels
        // We use ceiling to be conservative (round up for safety)
        inflationRadiusPixels = static_cast<int>(std::ceil(robotRadiusMeters / metersPerPixel));
        
        std::cout << "[InflatedBitMap] Robot radius: " << robotRadiusMeters << "m" << std::endl;
        std::cout << "[InflatedBitMap] Resolution: " << metersPerPixel << " meters/pixel" << std::endl;
        std::cout << "[InflatedBitMap] Inflation radius: " << inflationRadiusPixels << " pixels" << std::endl;

        // Initialize inflated grid - start as copy of source
        const std::vector<bool>& sourceData = source.GetRawData();
        inflatedGrid = sourceData;

        // =========================================================================
        // INFLATION ALGORITHM (Minkowski Sum via Brushfire)
        // =========================================================================
        // For each obstacle cell, mark all cells within inflationRadius as blocked.
        // This is equivalent to performing a Minkowski sum of the obstacles with
        // a circular structuring element of radius = inflationRadiusPixels.
        //
        // We use a simple but effective approach:
        // 1. Iterate through all cells
        // 2. For each obstacle cell, paint a circle of blocked cells around it
        // =========================================================================

        int inflationCount = 0;
        
        // Pre-compute the circular mask offsets for efficiency
        // All (dx, dy) pairs where dx^2 + dy^2 <= inflationRadius^2
        std::vector<std::pair<int, int>> circleOffsets;
        for (int dy = -inflationRadiusPixels; dy <= inflationRadiusPixels; ++dy) {
            for (int dx = -inflationRadiusPixels; dx <= inflationRadiusPixels; ++dx) {
                // Use squared distance to avoid sqrt
                int distSq = dx * dx + dy * dy;
                int radiusSq = inflationRadiusPixels * inflationRadiusPixels;
                
                if (distSq <= radiusSq) {
                    circleOffsets.push_back({dx, dy});
                }
            }
        }
        
        std::cout << "[InflatedBitMap] Circle mask size: " << circleOffsets.size() << " pixels" << std::endl;

        // Scan for obstacle cells and inflate
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Check if this is an obstacle in the original map
                if (!sourceData[y * width + x]) {
                    // This is an obstacle - inflate around it
                    for (const auto& offset : circleOffsets) {
                        int nx = x + offset.first;
                        int ny = y + offset.second;
                        
                        // Bounds check
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int idx = ny * width + nx;
                            // Only count if it was previously walkable
                            if (inflatedGrid[idx]) {
                                inflatedGrid[idx] = false;
                                ++inflationCount;
                            }
                        }
                    }
                }
            }
        }

        // =========================================================================
        // BOUNDARY INFLATION
        // =========================================================================
        // Also inflate around the map edges - robot can't get too close to boundaries
        // =========================================================================
        
        int boundaryInflation = 0;
        
        // Top and bottom edges
        for (int y = 0; y < inflationRadiusPixels && y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                if (inflatedGrid[idx]) {
                    inflatedGrid[idx] = false;
                    ++boundaryInflation;
                }
                
                // Bottom edge
                int bottomY = height - 1 - y;
                if (bottomY >= 0 && bottomY != y) {
                    int bottomIdx = bottomY * width + x;
                    if (inflatedGrid[bottomIdx]) {
                        inflatedGrid[bottomIdx] = false;
                        ++boundaryInflation;
                    }
                }
            }
        }
        
        // Left and right edges (excluding corners already done)
        for (int x = 0; x < inflationRadiusPixels && x < width; ++x) {
            for (int y = inflationRadiusPixels; y < height - inflationRadiusPixels; ++y) {
                int idx = y * width + x;
                if (inflatedGrid[idx]) {
                    inflatedGrid[idx] = false;
                    ++boundaryInflation;
                }
                
                // Right edge
                int rightX = width - 1 - x;
                if (rightX >= 0 && rightX != x) {
                    int rightIdx = y * width + rightX;
                    if (inflatedGrid[rightIdx]) {
                        inflatedGrid[rightIdx] = false;
                        ++boundaryInflation;
                    }
                }
            }
        }

        // Calculate and print statistics
        int originalWalkable = 0;
        int inflatedWalkable = 0;
        for (int i = 0; i < width * height; ++i) {
            if (sourceData[i]) ++originalWalkable;
            if (inflatedGrid[i]) ++inflatedWalkable;
        }

        std::cout << "[InflatedBitMap] Original walkable cells: " << originalWalkable << std::endl;
        std::cout << "[InflatedBitMap] Cells closed by obstacle inflation: " << inflationCount << std::endl;
        std::cout << "[InflatedBitMap] Cells closed by boundary inflation: " << boundaryInflation << std::endl;
        std::cout << "[InflatedBitMap] Remaining walkable cells: " << inflatedWalkable << std::endl;
        std::cout << "[InflatedBitMap] Reduction: " 
                  << (100.0 * (originalWalkable - inflatedWalkable) / originalWalkable) 
                  << "%" << std::endl;
    }

    bool InflatedBitMap::IsAccessible(Backend::Common::Coordinates coords) const {
        if (!IsWithinBounds(coords)) return false;
        return inflatedGrid[coords.y * width + coords.x];
    }

    int InflatedBitMap::GetInflationRadiusPixels() const {
        return inflationRadiusPixels;
    }

    const std::vector<bool>& InflatedBitMap::GetRawData() const {
        return inflatedGrid;
    }

    const StaticBitMap* InflatedBitMap::GetSourceMap() const {
        return sourceMap;
    }

    void InflatedBitMap::ExportToFile(const std::string& filepath) const {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[InflatedBitMap] Failed to open file for writing: " << filepath << std::endl;
            return;
        }

        // Write dimensions on first line
        file << width << " " << height << "\n";

        // Write grid data
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                file << (inflatedGrid[y * width + x] ? '.' : '#');
            }
            file << "\n";
        }

        file.close();
        std::cout << "[InflatedBitMap] Exported inflated map to: " << filepath << std::endl;
    }

    void InflatedBitMap::GetInflationStats(int& originalWalkable, int& inflatedWalkable, int& closedCells) const {
        originalWalkable = 0;
        inflatedWalkable = 0;
        
        const std::vector<bool>& sourceData = sourceMap->GetRawData();
        
        for (int i = 0; i < width * height; ++i) {
            if (sourceData[i]) ++originalWalkable;
            if (inflatedGrid[i]) ++inflatedWalkable;
        }
        
        closedCells = originalWalkable - inflatedWalkable;
    }

} // namespace Layer1
} // namespace Backend
