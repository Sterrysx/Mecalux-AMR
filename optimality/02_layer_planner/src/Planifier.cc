#include "../include/Planifier.hh"
#include "../include/BruteForceAlgorithm.hh"
#include "../include/GreedyAlgorithm.hh"
#include "../include/PriorityAlgorithm.hh"
#include "../include/HybridAlgorithm.hh"
#include <memory>
using namespace std;


// Constructors and Destructor
Planifier::Planifier() : totalRobots(0), currentAlgorithm(nullptr) {}

Planifier::Planifier(const Graph& graph, int numRobots, queue<Task> tasks) 
    : G(graph), totalRobots(numRobots), pendingTasks(tasks), currentAlgorithm(nullptr) {
    // Initialize robots
    for (int i = 0; i < totalRobots; i++) {
        availableRobots.push(Robot());
    }
}

Planifier::~Planifier() {}

// Getters
int Planifier::getNumRobots() const { return totalRobots; }
int Planifier::getAvailableRobots() const { return availableRobots.size(); }
int Planifier::getBusyRobots() const { return busyRobots.size(); }
int Planifier::getChargingRobots() const { return chargingRobots.size(); }
Graph Planifier::getGraph() const { return G; }
queue<Task> Planifier::getPendingTasks() const { return pendingTasks; }

// Setters
void Planifier::setGraph(const Graph& graph) { G = graph; }
void Planifier::setNumRobots(int numRobots) { const_cast<int&>(this->totalRobots) = numRobots; }
void Planifier::setAvailableRobots(int num) { 
    while (availableRobots.size() > static_cast<size_t>(num)) availableRobots.pop();
}
void Planifier::setBusyRobots(int num) { 
    while (busyRobots.size() > static_cast<size_t>(num)) busyRobots.pop();
}
void Planifier::setChargingRobots(int num) { 
    while (chargingRobots.size() > static_cast<size_t>(num)) chargingRobots.pop();
}
void Planifier::setPendingTasks(int num) {
    while (pendingTasks.size() > static_cast<size_t>(num)) pendingTasks.pop();
}

// Strategy Pattern: Set the planning algorithm
void Planifier::setAlgorithm(unique_ptr<Algorithm> algorithm) {
    currentAlgorithm = std::move(algorithm);
}

// Execute the currently set algorithm
void Planifier::executePlan() {
    if (currentAlgorithm == nullptr) {
        cout << "Error: No algorithm set. Use setAlgorithm() or plan() first." << endl;
        return;
    }
    
    cout << "=== Algorithm: " << currentAlgorithm->getName() << " ===" << endl;
    cout << "Description: " << currentAlgorithm->getDescription() << endl;
    cout << endl;
    
    currentAlgorithm->execute(
        G,
        availableRobots,
        busyRobots,
        chargingRobots,
        pendingTasks,
        totalRobots
    );
    
    cout << "=== End Algorithm ===" << endl;
}

// Planning method with algorithm selection (refactored to use Strategy Pattern)
void Planifier::plan(int algorithmChoice) {
    int selectedAlgorithm = algorithmChoice;
    
    // Interactive mode: ask user if algorithm is 0
    if (algorithmChoice == 0) {
        cout << "\n=== Algorithm Selection ===" << endl;
        cout << "Available algorithms:" << endl;
        cout << "  1. Brute Force - Explores all possible assignments" << endl;
        cout << "  2. Greedy - Nearest available robot heuristic" << endl;
        cout << "  3. Priority - Task priority-based assignment" << endl;
        cout << "  4. Hybrid - Combines multiple strategies" << endl;
        cout << "Choose an algorithm (1-4): ";
        cin >> selectedAlgorithm;
        cout << endl;
    }
    
    // Use Strategy Pattern instead of switch
    switch (selectedAlgorithm) {
        case 1:
            setAlgorithm(make_unique<BruteForceAlgorithm>());
            break;
        case 2:
            setAlgorithm(make_unique<GreedyAlgorithm>());
            break;
        case 3:
            setAlgorithm(make_unique<PriorityAlgorithm>());
            break;
        case 4:
            setAlgorithm(make_unique<HybridAlgorithm>());
            break;
        default:
            cout << "Invalid algorithm. Choose 1, 2, 3, or 4." << endl;
            return;
    }
    
    // Execute the selected algorithm
    executePlan();
}