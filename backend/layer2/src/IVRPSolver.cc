/**
 * @file IVRPSolver.cc
 * @brief Implementation of VRPResult helper methods
 */

#include "../include/IVRPSolver.hh"
#include <iostream>
#include <iomanip>

namespace Backend {
namespace Layer2 {

void VRPResult::Print() const {
    std::cout << "\n=== VRP Solution (" << algorithmName << ") ===\n";
    std::cout << "Status: " << (isFeasible ? "FEASIBLE" : "INFEASIBLE");
    if (isOptimal) std::cout << " (OPTIMAL)";
    std::cout << "\n";
    
    std::cout << "Makespan: " << std::fixed << std::setprecision(2) 
              << makespan << " units\n";
    std::cout << "Total Distance: " << std::fixed << std::setprecision(2) 
              << totalDistance << " units\n";
    std::cout << "Computation Time: " << std::fixed << std::setprecision(3) 
              << computationTimeMs << " ms\n";
    
    std::cout << "\nRobot Itineraries:\n";
    for (const auto& [robotId, itinerary] : robotItineraries) {
        std::cout << "  Robot " << robotId << ": [";
        for (size_t i = 0; i < itinerary.size(); ++i) {
            std::cout << itinerary[i];
            if (i < itinerary.size() - 1) std::cout << " -> ";
        }
        std::cout << "]\n";
    }
    std::cout << "=================================\n";
}

} // namespace Layer2
} // namespace Backend
