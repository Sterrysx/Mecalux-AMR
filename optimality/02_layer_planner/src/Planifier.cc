#include "Planifier.hh"
#include "algorithms/01_BruteForce.hh"
#include "algorithms/02_Greedy.hh"
#include "algorithms/03_HillClimbing.hh"
#include <memory>
using namespace std;


// Constructors and Destructor
Planifier::Planifier(const Graph& graph, int numRobots, queue<Task> tasks) 
    : G(graph), totalRobots(numRobots), pendingTasks(tasks), currentAlgorithm(nullptr) {
    // Initialize robots with proper IDs
    for (int i = 0; i < totalRobots; i++) {
        Robot robot(i, {0.0, 0.0}, 100.0, -1, 1.6, 100);
        availableRobots.push(robot);
    }
}

Planifier::~Planifier() {}

// Getters
int Planifier::getNumRobots() const { return totalRobots; }
int Planifier::getAvailableRobots() const { return availableRobots.size(); }
int Planifier::getBusyRobots() const { return busyRobots.size(); }
int Planifier::getChargingRobots() const { return chargingRobots.size(); }
const Graph& Planifier::getGraph() const { return G; }
queue<Task> Planifier::getPendingTasks() const { return pendingTasks; }

// Setters
void Planifier::setNumRobots(int numRobots) { const_cast<int&>(this->totalRobots) = numRobots; }
void Planifier::setAvailableRobots(int num) { //nomes borra robots sobrants no afegeix nous
    while (availableRobots.size() > static_cast<size_t>(num)) availableRobots.pop();
}
void Planifier::setBusyRobots(int num) { //igual que el de dalt
    while (busyRobots.size() > static_cast<size_t>(num)) busyRobots.pop();
}
void Planifier::setChargingRobots(int num) { //igual que el de dalt
    while (chargingRobots.size() > static_cast<size_t>(num)) chargingRobots.pop();
}
void Planifier::setPendingTasks(int num) { //igual que el de dalt pero amb tasks no entenc lo del static_cast
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
    
    // Comparison mode: run all algorithms if choice is 0
    if (algorithmChoice == 0) {
        cout << "========================================" << endl;
        cout << "  ALGORITHM COMPARISON TEST" << endl;
        cout << "  Graph with " << G.getNumVertices() << " nodes, " 
             << getPendingTasks().size() << " Tasks, " << totalRobots << " Robots" << endl;
        cout << "========================================" << endl;
        cout << endl;
        
        // Save original tasks
        vector<Task> taskList;
        queue<Task> tempTasks = pendingTasks;
        while (!tempTasks.empty()) {
            taskList.push_back(tempTasks.front());
            tempTasks.pop();
        }
        
        // Run each algorithm
        for (int alg = 1; alg <= 3; alg++) {
            // Recreate task queue
            pendingTasks = queue<Task>();
            for (const Task& t : taskList) {
                pendingTasks.push(t);
            }
            
            // Recreate robot queues
            availableRobots = queue<Robot>();
            busyRobots = queue<Robot>();
            chargingRobots = queue<Robot>();
            for (int i = 0; i < totalRobots; i++) {
                Robot robot(i, {0.0, 0.0}, 100.0, -1, 1.6, 100);
                availableRobots.push(robot);
            }
            
            // Print algorithm header
            if (alg == 1) {
                cout << "--- BRUTE FORCE (Optimal) ---" << endl;
                setAlgorithm(make_unique<BruteForce>());
            } else if (alg == 2) {
                cout << "--- GREEDY (Fast Heuristic) ---" << endl;
                setAlgorithm(make_unique<Greedy>());
            } else if (alg == 3) {
                cout << "--- HILL CLIMBING (Improved Greedy) ---" << endl;
                setAlgorithm(make_unique<HillClimbing>());
            }
            
            // Execute algorithm
            currentAlgorithm->execute(
                G,
                availableRobots,
                busyRobots,
                chargingRobots,
                pendingTasks,
                totalRobots,
                true  // compactMode = true for comparison
            );
            
            cout << endl;
        }
        
        cout << "========================================" << endl;
        return;
    }
    
    // Use Strategy Pattern instead of switch
    switch (selectedAlgorithm) {
        case 1:
            setAlgorithm(make_unique<BruteForce>());
            break;
        case 2:
            setAlgorithm(make_unique<Greedy>());
            break;
        case 3:
            setAlgorithm(make_unique<HillClimbing>());
            break;
        default:
            cout << "Invalid algorithm. Choose 1, 2, or 3." << endl;
            return;
    }
    
    // Execute the selected algorithm
    executePlan();
}