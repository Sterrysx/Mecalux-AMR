#include "../include/Planifier.hh"
using namespace std;


// Constructors and Destructor
Planifier::Planifier() : numRobots(0) {}

Planifier::Planifier(const Graph& graph, int numRobots) : G(graph), numRobots(numRobots) {}

Planifier::~Planifier() {}

// Getters
int Planifier::getNumRobots() const { return numRobots; }
int Planifier::getAvailableRobots() const { return availableRobots.size(); }
int Planifier::getBusyRobots() const { return busyRobots.size(); }
int Planifier::getChargingRobots() const { return chargingRobots.size(); }
Graph Planifier::getGraph() const { return G; }

// Setters
void Planifier::setGraph(const Graph& graph) { G = graph; }
void Planifier::setNumRobots(int numRobots) { const_cast<int&>(this->numRobots) = numRobots; }
void Planifier::setAvailableRobots(int num) { 
    while (availableRobots.size() > num) availableRobots.pop();
}
void Planifier::setBusyRobots(int num) { 
    while (busyRobots.size() > num) busyRobots.pop();
}
void Planifier::setChargingRobots(int num) { 
    while (chargingRobots.size() > num) chargingRobots.pop();
}

// Planning method with algorithm selection
void Planifier::plan(int algorithm) {
    int selectedAlgorithm = algorithm;
    
    // Interactive mode: ask user if algorithm is not 1 (default)
    if (algorithm == 0) {
        cout << "\n=== Algorithm Selection ===" << endl;
        cout << "Available algorithms:" << endl;
        cout << "  1. Algorithm 1" << endl;
        cout << "  2. Algorithm 2" << endl;
        cout << "  3. Algorithm 3" << endl;
        cout << "  4. Algorithm 4" << endl;
        cout << "Choose an algorithm (1-4): ";
        cin >> selectedAlgorithm;
        cout << endl;
    }
    
    switch (selectedAlgorithm) {
        case 1:
            cout << "Placeholder for Algorithm 1" << endl;
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