#include "../include/Planifier.hh"
using namespace std;


// Constructors and Destructor
Planifier::Planifier() : totalRobots(0) {}

Planifier::Planifier(const Graph& graph, int numRobots, queue<Task> tasks) 
    : G(graph), totalRobots(numRobots), pendingTasks(tasks) {
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

// Brute force algorithm (placeholder)
void Planifier::bruteforce_algorithm() {
    cout << "Bruteforce algorithm placeholder - to be implemented" << endl;
    // TODO: Implement actual brute-force algorithm
}

// Planning method with algorithm selection
void Planifier::plan(int algorithm) {
    int selectedAlgorithm = algorithm;
    
    // Interactive mode: ask user if algorithm is 0
    if (algorithm == 0) {
        cout << "\n=== Algorithm Selection ===" << endl;
        cout << "Available algorithms:" << endl;
        cout << "  1. Brute Force" << endl;
        cout << "  2. Algorithm 2" << endl;
        cout << "  3. Algorithm 3" << endl;
        cout << "  4. Algorithm 4" << endl;
        cout << "Choose an algorithm (1-4): ";
        cin >> selectedAlgorithm;
        cout << endl;
    }
    
    switch (selectedAlgorithm) {
        case 1:
            cout << "=== Algorithm 1: Brute Force ===" << endl;
            bruteforce_algorithm();
            cout << "=== End Algorithm 1 ===" << endl;
            break;
        case 2:
            cout << "Placeholder for Algorithm 2" << endl;
            break;
        case 3:
            cout << "Placeholder for Algorithm 3" << endl;
            break;
        case 4:
            cout << "Placeholder for Algorithm 4" << endl;
            break;
        default:
            cout << "Invalid algorithm. Choose 1, 2, 3, or 4." << endl;
            break;
    }
}