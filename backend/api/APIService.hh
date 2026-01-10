/**
 * @file APIService.hh
 * @brief File-based API service for robot telemetry and fleet visualization
 * 
 * This header-only service writes JSON files to disk for consumption by
 * the frontend simulator. It acts as a "Frame Recorder" that dumps:
 * - Robot telemetry (position, velocity, status) at 20Hz
 * - Dynamic obstacle data at 1Hz
 * 
 * Uses C++17 <filesystem> for directory management.
 */

#ifndef BACKEND_API_SERVICE_HH
#define BACKEND_API_SERVICE_HH

#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <chrono>

#include "../common/include/Coordinates.hh"
#include "../layer3/include/Vector2.hh"

namespace Backend {
namespace API {

// =============================================================================
// DATA STRUCTURES
// =============================================================================

/**
 * @brief Task statistics for API output.
 */
struct TasksInfo {
    int active = 0;       ///< Currently in-progress tasks
    int completed = 0;    ///< Completed tasks count
    int pending = 0;      ///< Pending tasks in queue
};

/**
 * @brief Charging station status for API output.
 */
struct ChargingStationStatus {
    int id;                 ///< Station ID (0-indexed)
    int x;                  ///< Station X coordinate
    int y;                  ///< Station Y coordinate
    std::string status;     ///< "AVAILABLE" or "OCCUPIED"
    int robotId;            ///< Robot ID if occupied, -1 otherwise
};

/**
 * @brief Robot telemetry data for API broadcast.
 */
struct RobotTelemetry {
    int id;                         ///< Robot ID
    Common::Coordinates pos;        ///< Current pixel position
    Layer3::Vector2 velocity;       ///< Current velocity vector
    std::string status;             ///< Status string (IDLE, MOVING, etc.)
    std::string driverState;        ///< Driver state (IDLE, MOVING, COMPUTING_PATH, etc.)
    float battery;                  ///< Battery level (0.0 - 1.0)
    int currentNodeId;              ///< Current nearest NavMesh node
    int targetNodeId;               ///< Target node (or -1 if none)
    int remainingWaypoints;         ///< Remaining waypoints in itinerary
    bool hasPackage;                ///< Whether robot is carrying a package
};

/**
 * @brief Dynamic obstacle data for API broadcast.
 */
struct ObstacleInfo {
    Common::Coordinates topLeft;    ///< Top-left corner of obstacle
    int width;                      ///< Width in pixels
    int height;                     ///< Height in pixels
    std::string type;               ///< Obstacle type (e.g., "forklift", "pallet")
};

/**
 * @brief Path segment for visualization.
 */
struct PathSegment {
    int robotId;
    std::vector<Common::Coordinates> waypoints;
};

// =============================================================================
// API SERVICE CLASS
// =============================================================================

/**
 * @brief File-based API service for fleet visualization.
 * 
 * Writes JSON files to disk for the frontend to consume:
 * - orca/orca_tick_{N}.json - Robot telemetry at 20Hz
 * - fleet/fleet_tick_{N}.json - Dynamic obstacles at 1Hz
 * - paths/paths_tick_{N}.json - Robot paths (on request)
 * 
 * Thread-safe: Uses internal mutex for file operations.
 */
class APIService {
private:
    mutable std::mutex apiMutex_;   ///< Protects file I/O
    std::string basePath_;          ///< Base directory for output
    long long tickPhysics_ = 0;     ///< Physics tick counter
    long long tickMap_ = 0;         ///< Map tick counter
    long long tickPaths_ = 0;       ///< Paths tick counter
    
    bool enabled_ = true;           ///< Enable/disable file output
    int keepLastNFiles_ = 100;      ///< Keep only last N files per folder
    
    // =========================================================================
    // HELPERS
    // =========================================================================
    
    /**
     * @brief Ensure directory exists, create if not.
     */
    void EnsureDirectory(const std::string& dir) const {
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        }
    }
    
    /**
     * @brief Write content to a file.
     */
    void WriteFile(const std::string& folder, const std::string& filename, 
                   const std::string& content) const {
        std::string dir = basePath_ + "/" + folder;
        EnsureDirectory(dir);
        
        std::ofstream f(dir + "/" + filename);
        if (f.is_open()) {
            f << content;
            f.close();
        }
    }
    
    /**
     * @brief Clean old files, keeping only the last N.
     */
    void CleanOldFiles(const std::string& folder, const std::string& prefix, 
                       long long currentTick) const {
        if (currentTick <= keepLastNFiles_) return;
        
        std::string dir = basePath_ + "/" + folder;
        long long deleteBelow = currentTick - keepLastNFiles_;
        
        std::string oldFile = dir + "/" + prefix + std::to_string(deleteBelow) + ".json";
        if (std::filesystem::exists(oldFile)) {
            std::filesystem::remove(oldFile);
        }
    }
    
    /**
     * @brief Get current timestamp as ISO string.
     */
    std::string GetTimestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

public:
    // =========================================================================
    // CONSTRUCTOR
    // =========================================================================
    
    /**
     * @brief Construct APIService with output path.
     * 
     * @param path Base directory for output files (default: ../api)
     */
    explicit APIService(const std::string& path = "../api") 
        : basePath_(path) 
    {
        // Ensure base directories exist on startup
        EnsureDirectory(basePath_ + "/orca");
        EnsureDirectory(basePath_ + "/fleet");
        EnsureDirectory(basePath_ + "/paths");
    }
    
    // =========================================================================
    // CONFIGURATION
    // =========================================================================
    
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }
    
    void SetKeepLastNFiles(int n) { keepLastNFiles_ = n; }
    
    void SetBasePath(const std::string& path) { 
        basePath_ = path;
        EnsureDirectory(basePath_ + "/orca");
        EnsureDirectory(basePath_ + "/fleet");
        EnsureDirectory(basePath_ + "/paths");
    }
    
    // =========================================================================
    // BROADCAST METHODS
    // =========================================================================
    
    /**
     * @brief Broadcast robot telemetry (20 Hz).
     * 
     * Writes to: orca/orca_tick_{N}.json
     * 
     * @param data Vector of robot telemetry data
     */
    void BroadcastTelemetry(const std::vector<RobotTelemetry>& data) {
        if (!enabled_) return;
        
        std::lock_guard<std::mutex> lock(apiMutex_);
        
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"tick\": " << tickPhysics_ << ",\n";
        ss << "  \"timestamp\": \"" << GetTimestamp() << "\",\n";
        ss << "  \"robots\": [\n";
        
        for (size_t i = 0; i < data.size(); ++i) {
            const auto& r = data[i];
            ss << "    {\n";
            ss << "      \"id\": " << r.id << ",\n";
            ss << "      \"x\": " << r.pos.x << ",\n";
            ss << "      \"y\": " << r.pos.y << ",\n";
            ss << "      \"vx\": " << std::fixed << std::setprecision(4) << r.velocity.x << ",\n";
            ss << "      \"vy\": " << std::fixed << std::setprecision(4) << r.velocity.y << ",\n";
            ss << "      \"status\": \"" << r.status << "\",\n";
            ss << "      \"driverState\": \"" << r.driverState << "\",\n";
            ss << "      \"battery\": " << r.battery << ",\n";
            ss << "      \"currentNodeId\": " << r.currentNodeId << ",\n";
            ss << "      \"targetNodeId\": " << r.targetNodeId << ",\n";
            ss << "      \"remainingWaypoints\": " << r.remainingWaypoints << ",\n";
            ss << "      \"hasPackage\": " << (r.hasPackage ? "true" : "false") << "\n";
            ss << "    }";
            if (i < data.size() - 1) ss << ",";
            ss << "\n";
        }
        
        ss << "  ]\n";
        ss << "}\n";
        
        WriteFile("orca", "orca_tick_" + std::to_string(tickPhysics_) + ".json", ss.str());
        CleanOldFiles("orca", "orca_tick_", tickPhysics_);
        tickPhysics_++;
    }
    
    /**
     * @brief Broadcast dynamic obstacles (1 Hz).
     * 
     * Writes to: fleet/fleet_tick_{N}.json
     * 
     * @param obstacles Vector of obstacle data
     */
    void BroadcastObstacles(const std::vector<ObstacleInfo>& obstacles) {
        if (!enabled_) return;
        
        std::lock_guard<std::mutex> lock(apiMutex_);
        
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"tick\": " << tickMap_ << ",\n";
        ss << "  \"timestamp\": \"" << GetTimestamp() << "\",\n";
        ss << "  \"obstacles\": [\n";
        
        for (size_t i = 0; i < obstacles.size(); ++i) {
            const auto& o = obstacles[i];
            ss << "    {\n";
            ss << "      \"x\": " << o.topLeft.x << ",\n";
            ss << "      \"y\": " << o.topLeft.y << ",\n";
            ss << "      \"width\": " << o.width << ",\n";
            ss << "      \"height\": " << o.height << ",\n";
            ss << "      \"type\": \"" << o.type << "\"\n";
            ss << "    }";
            if (i < obstacles.size() - 1) ss << ",";
            ss << "\n";
        }
        
        ss << "  ]\n";
        ss << "}\n";
        
        WriteFile("fleet", "fleet_tick_" + std::to_string(tickMap_) + ".json", ss.str());
        CleanOldFiles("fleet", "fleet_tick_", tickMap_);
        tickMap_++;
    }
    
    /**
     * @brief Broadcast robot paths (on demand).
     * 
     * Writes to: paths/paths_tick_{N}.json
     * 
     * @param paths Vector of path segments per robot
     */
    void BroadcastPaths(const std::vector<PathSegment>& paths) {
        if (!enabled_) return;
        
        std::lock_guard<std::mutex> lock(apiMutex_);
        
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"tick\": " << tickPaths_ << ",\n";
        ss << "  \"timestamp\": \"" << GetTimestamp() << "\",\n";
        ss << "  \"paths\": [\n";
        
        for (size_t i = 0; i < paths.size(); ++i) {
            const auto& p = paths[i];
            ss << "    {\n";
            ss << "      \"robotId\": " << p.robotId << ",\n";
            ss << "      \"waypoints\": [\n";
            
            for (size_t j = 0; j < p.waypoints.size(); ++j) {
                ss << "        {\"x\": " << p.waypoints[j].x 
                   << ", \"y\": " << p.waypoints[j].y << "}";
                if (j < p.waypoints.size() - 1) ss << ",";
                ss << "\n";
            }
            
            ss << "      ]\n";
            ss << "    }";
            if (i < paths.size() - 1) ss << ",";
            ss << "\n";
        }
        
        ss << "  ]\n";
        ss << "}\n";
        
        WriteFile("paths", "paths_tick_" + std::to_string(tickPaths_) + ".json", ss.str());
        CleanOldFiles("paths", "paths_tick_", tickPaths_);
        tickPaths_++;
    }
    
    // =========================================================================
    // SINGLE FILE WRITE (for special events)
    // =========================================================================
    
    /**
     * @brief Write a single JSON file with VRP solution.
     */
    void WriteSolution(const std::string& content) {
        if (!enabled_) return;
        
        std::lock_guard<std::mutex> lock(apiMutex_);
        WriteFile(".", "solution.json", content);
    }
    
    /**
     * @brief Write aggregated robots.json for frontend consumption.
     * 
     * Uses atomic temp-file-rename to ensure safe concurrent reads.
     * This file is written periodically (every ~20 ticks = 1 Hz).
     * 
     * @param robots Robot telemetry data
     * @param tasks Task statistics
     * @param stations Charging station statuses
     */
    void WriteRobotsJSON(
        const std::vector<RobotTelemetry>& robots,
        const TasksInfo& tasks,
        const std::vector<ChargingStationStatus>& stations
    ) {
        if (!enabled_) return;
        
        std::lock_guard<std::mutex> lock(apiMutex_);
        
        // Get current timestamp in milliseconds
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"timestamp\": " << ms << ",\n";
        
        // Robots array
        ss << "  \"robots\": [\n";
        for (size_t i = 0; i < robots.size(); ++i) {
            const auto& r = robots[i];
            ss << "    {\n";
            ss << "      \"id\": " << r.id << ",\n";
            ss << "      \"x\": " << r.pos.x << ",\n";
            ss << "      \"y\": " << r.pos.y << ",\n";
            ss << "      \"vx\": " << std::fixed << std::setprecision(2) << r.velocity.x << ",\n";
            ss << "      \"vy\": " << std::fixed << std::setprecision(2) << r.velocity.y << ",\n";
            ss << "      \"state\": \"" << r.driverState << "\",\n";
            ss << "      \"batteryLevel\": " << static_cast<int>(r.battery * 100) << ",\n";
            ss << "      \"goal\": " << (r.targetNodeId >= 0 ? std::to_string(r.targetNodeId) : "null") << ",\n";
            ss << "      \"itinerary\": " << r.remainingWaypoints << "\n";
            ss << "    }";
            if (i < robots.size() - 1) ss << ",";
            ss << "\n";
        }
        ss << "  ],\n";
        
        // Tasks object
        ss << "  \"tasks\": {\n";
        ss << "    \"active\": " << tasks.active << ",\n";
        ss << "    \"completed\": " << tasks.completed << ",\n";
        ss << "    \"pending\": " << tasks.pending << "\n";
        ss << "  },\n";
        
        // Charging stations array
        ss << "  \"charging_stations\": [\n";
        for (size_t i = 0; i < stations.size(); ++i) {
            const auto& s = stations[i];
            ss << "    {\n";
            ss << "      \"id\": " << s.id << ",\n";
            ss << "      \"x\": " << s.x << ",\n";
            ss << "      \"y\": " << s.y << ",\n";
            ss << "      \"status\": \"" << s.status << "\",\n";
            ss << "      \"robot\": " << (s.robotId >= 0 ? std::to_string(s.robotId) : "null") << "\n";
            ss << "    }";
            if (i < stations.size() - 1) ss << ",";
            ss << "\n";
        }
        ss << "  ]\n";
        
        ss << "}\n";
        
        // ATOMIC WRITE: Write to temp file, then rename
        std::string dir = basePath_ + "/output";
        EnsureDirectory(dir);
        
        std::string tempPath = dir + "/robots.tmp.json";
        std::string finalPath = dir + "/robots.json";
        
        // Write to temp file
        std::ofstream f(tempPath);
        if (f.is_open()) {
            f << ss.str();
            f.close();
            
            // Atomic rename (works on POSIX systems, on Windows this may fail if target exists)
            // For Windows compatibility, we remove then rename
            #ifdef _WIN32
            std::filesystem::remove(finalPath);
            #endif
            std::filesystem::rename(tempPath, finalPath);
        }
    }
    
    /**
     * @brief Get current tick counters.
     */
    long long GetPhysicsTick() const { return tickPhysics_; }
    long long GetMapTick() const { return tickMap_; }
};

} // namespace API
} // namespace Backend

#endif // BACKEND_API_SERVICE_HH
