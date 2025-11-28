/**
 * @file PathfindingService.cc
 * @brief Implementation of the pathfinding service singleton
 */

#include "Pathfinding/PathfindingService.hh"
#include <iostream>

namespace Backend {
namespace Layer3 {
namespace Pathfinding {

// =============================================================================
// STATIC MEMBERS
// =============================================================================

PathfindingService* PathfindingService::instance_ = nullptr;
std::mutex PathfindingService::instanceMutex_;

// =============================================================================
// CONSTRUCTOR
// =============================================================================

PathfindingService::PathfindingService()
    : safetyMap_(nullptr)
    , nextRequestId_(1) {}

// =============================================================================
// SINGLETON ACCESS
// =============================================================================

PathfindingService& PathfindingService::GetInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (instance_ == nullptr) {
        instance_ = new PathfindingService();
    }
    return *instance_;
}

void PathfindingService::DestroyInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
    }
}

// =============================================================================
// INITIALIZATION
// =============================================================================

void PathfindingService::Initialize(const Backend::Layer1::InflatedBitMap& safetyMap) {
    safetyMap_ = &safetyMap;
    std::cout << "[PathfindingService] Initialized with safety map\n";
}

// =============================================================================
// PATH REQUESTS
// =============================================================================

int PathfindingService::RequestPath(
    const Backend::Common::Coordinates& start,
    const Backend::Common::Coordinates& end,
    PathCallback callback
) {
    if (!IsInitialized()) {
        std::cerr << "[PathfindingService] ERROR: Not initialized!\n";
        if (callback) {
            PathResult empty;
            empty.success = false;
            callback(empty);
        }
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    PathRequest request;
    request.requestId = nextRequestId_++;
    request.start = start;
    request.end = end;
    request.callback = callback;
    request.promise = nullptr;
    
    requestQueue_.push_back(request);
    
    return request.requestId;
}

PathResult PathfindingService::RequestPathSync(
    const Backend::Common::Coordinates& start,
    const Backend::Common::Coordinates& end
) {
    if (!IsInitialized()) {
        std::cerr << "[PathfindingService] ERROR: Not initialized!\n";
        PathResult empty;
        empty.success = false;
        return empty;
    }
    
    std::promise<PathResult> promise;
    std::future<PathResult> future = promise.get_future();
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        PathRequest request;
        request.requestId = nextRequestId_++;
        request.start = start;
        request.end = end;
        request.callback = nullptr;
        request.promise = &promise;
        
        requestQueue_.push_back(request);
    }
    
    // Process until our request is done
    while (true) {
        bool processed = ProcessNextRequest();
        
        // Check if future is ready
        if (future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            return future.get();
        }
        
        if (!processed) {
            // Queue empty but future not ready - shouldn't happen
            break;
        }
    }
    
    return future.get();
}

PathResult PathfindingService::ComputePathImmediate(
    const Backend::Common::Coordinates& start,
    const Backend::Common::Coordinates& end
) {
    if (!IsInitialized()) {
        std::cerr << "[PathfindingService] ERROR: Not initialized!\n";
        PathResult empty;
        empty.success = false;
        return empty;
    }
    
    return solver_.ComputePath(start, end, *safetyMap_);
}

// =============================================================================
// QUEUE MANAGEMENT
// =============================================================================

bool PathfindingService::ProcessNextRequest() {
    PathRequest request;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (requestQueue_.empty()) {
            return false;
        }
        request = requestQueue_.front();
        requestQueue_.pop_front();
    }
    
    // Compute path
    PathResult result = solver_.ComputePath(request.start, request.end, *safetyMap_);
    
    // Invoke callback or set promise
    if (request.callback) {
        request.callback(result);
    }
    
    if (request.promise) {
        request.promise->set_value(result);
    }
    
    return true;
}

int PathfindingService::ProcessAllRequests() {
    int count = 0;
    while (ProcessNextRequest()) {
        count++;
    }
    return count;
}

size_t PathfindingService::GetQueueSize() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queueMutex_));
    return requestQueue_.size();
}

void PathfindingService::ClearQueue() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    requestQueue_.clear();
}

} // namespace Pathfinding
} // namespace Layer3
} // namespace Backend
