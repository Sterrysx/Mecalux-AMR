/**
 * @file PathfindingService.hh
 * @brief Centralized pathfinding service with FIFO request queue
 * 
 * Provides a singleton service for managing pathfinding requests.
 * Requests are processed in FIFO order (no priority).
 */

#ifndef LAYER3_PATHFINDING_PATHFINDINGSERVICE_HH
#define LAYER3_PATHFINDING_PATHFINDINGSERVICE_HH

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <future>

#include "Pathfinding/ThetaStarSolver.hh"
#include "Coordinates.hh"
#include "InflatedBitMap.hh"

namespace Backend {
namespace Layer3 {
namespace Pathfinding {

/**
 * @brief Callback type for path completion.
 */
using PathCallback = std::function<void(const PathResult&)>;

/**
 * @brief A pathfinding request in the queue.
 */
struct PathRequest {
    int requestId;                           ///< Unique request identifier
    Backend::Common::Coordinates start;      ///< Start position
    Backend::Common::Coordinates end;        ///< Goal position
    PathCallback callback;                   ///< Completion callback
    std::promise<PathResult>* promise;       ///< For synchronous waiting (optional)
};

/**
 * @brief Singleton pathfinding service.
 * 
 * Manages a FIFO queue of path requests and processes them using ThetaStarSolver.
 * 
 * Usage:
 *   auto& service = PathfindingService::GetInstance();
 *   service.Initialize(safetyMap);
 *   
 *   // Async request with callback
 *   service.RequestPath(start, end, [](const PathResult& r) {
 *       if (r.success) { ... }
 *   });
 *   
 *   // Sync request (blocking)
 *   PathResult result = service.RequestPathSync(start, end);
 */
class PathfindingService {
private:
    // Singleton instance
    static PathfindingService* instance_;
    static std::mutex instanceMutex_;
    
    // Request queue (FIFO, no priority)
    std::deque<PathRequest> requestQueue_;
    std::mutex queueMutex_;
    
    // Pathfinding solver
    ThetaStarSolver solver_;
    
    // Reference to the safety map
    const Backend::Layer1::InflatedBitMap* safetyMap_;
    
    // Request counter
    int nextRequestId_;
    
    // Private constructor for singleton
    PathfindingService();

public:
    // =========================================================================
    // SINGLETON ACCESS
    // =========================================================================
    
    /**
     * @brief Get the singleton instance.
     */
    static PathfindingService& GetInstance();
    
    /**
     * @brief Destroy the singleton instance.
     */
    static void DestroyInstance();
    
    // Delete copy/move
    PathfindingService(const PathfindingService&) = delete;
    PathfindingService& operator=(const PathfindingService&) = delete;
    PathfindingService(PathfindingService&&) = delete;
    PathfindingService& operator=(PathfindingService&&) = delete;

    // =========================================================================
    // INITIALIZATION
    // =========================================================================
    
    /**
     * @brief Initialize with a safety map.
     * 
     * Must be called before any path requests.
     * 
     * @param safetyMap Inflated bitmap for collision checking
     */
    void Initialize(const Backend::Layer1::InflatedBitMap& safetyMap);
    
    /**
     * @brief Check if service is initialized.
     */
    bool IsInitialized() const { return safetyMap_ != nullptr; }

    // =========================================================================
    // PATH REQUESTS
    // =========================================================================
    
    /**
     * @brief Request a path asynchronously.
     * 
     * The request is added to the FIFO queue and processed in order.
     * The callback is invoked when the path is computed.
     * 
     * @param start Starting position (pixels)
     * @param end Goal position (pixels)
     * @param callback Function to call with the result
     * @return Request ID
     */
    int RequestPath(
        const Backend::Common::Coordinates& start,
        const Backend::Common::Coordinates& end,
        PathCallback callback
    );
    
    /**
     * @brief Request a path synchronously (blocking).
     * 
     * Adds the request to the queue and waits for the result.
     * 
     * @param start Starting position (pixels)
     * @param end Goal position (pixels)
     * @return PathResult with the computed path
     */
    PathResult RequestPathSync(
        const Backend::Common::Coordinates& start,
        const Backend::Common::Coordinates& end
    );
    
    /**
     * @brief Compute a path immediately (bypass queue).
     * 
     * For urgent requests that shouldn't wait in queue.
     * 
     * @param start Starting position
     * @param end Goal position
     * @return PathResult
     */
    PathResult ComputePathImmediate(
        const Backend::Common::Coordinates& start,
        const Backend::Common::Coordinates& end
    );

    // =========================================================================
    // QUEUE MANAGEMENT
    // =========================================================================
    
    /**
     * @brief Process the next request in the queue.
     * 
     * Call this from a worker thread or main loop.
     * 
     * @return true if a request was processed
     */
    bool ProcessNextRequest();
    
    /**
     * @brief Process all pending requests.
     * 
     * @return Number of requests processed
     */
    int ProcessAllRequests();
    
    /**
     * @brief Get the number of pending requests.
     */
    size_t GetQueueSize() const;
    
    /**
     * @brief Clear all pending requests.
     */
    void ClearQueue();

    // =========================================================================
    // STATISTICS
    // =========================================================================
    
    /**
     * @brief Get total requests processed.
     */
    int GetTotalRequestsProcessed() const { return nextRequestId_ - 1; }
};

} // namespace Pathfinding
} // namespace Layer3
} // namespace Backend

#endif // LAYER3_PATHFINDING_PATHFINDINGSERVICE_HH
