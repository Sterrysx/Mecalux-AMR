#include "Planifier.hh"
#include "algorithms/01_BruteForce.hh"
#include "algorithms/02_Greedy.hh"
#include "algorithms/03_HillClimbing.hh"
#include <memory>
#include <iomanip>
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
AlgorithmResult Planifier::executePlan() {
    AlgorithmResult emptyResult;
    
    if (currentAlgorithm == nullptr) {
        cout << "Error: No algorithm set. Use setAlgorithm() or plan() first." << endl;
        return emptyResult;
    }
    
    cout << "=== Algorithm: " << currentAlgorithm->getName() << " ===" << endl;
    cout << "Description: " << currentAlgorithm->getDescription() << endl;
    cout << endl;
    
    AlgorithmResult result = currentAlgorithm->execute(
        G,
        availableRobots,
        busyRobots,
        chargingRobots,
        pendingTasks,
        totalRobots
    );
    
    cout << "=== End Algorithm ===" << endl;
    
    return result;
}

// Planning method with algorithm selection (refactored to use Strategy Pattern)
void Planifier::plan(int algorithmChoice) {
    int selectedAlgorithm = algorithmChoice;
    
    // Comparison mode: run all algorithms if choice is 0 or -1
    // 0: run all three algorithms (BruteForce, Greedy, HillClimbing)
    // -1: run only heuristic algorithms (Greedy, HillClimbing) - excludes BruteForce
    if (algorithmChoice == 0 || algorithmChoice == -1) {
        bool includeOptimal = (algorithmChoice == 0);
        
        cout << "========================================" << endl;
        if (includeOptimal) {
            cout << "  ALGORITHM COMPARISON TEST (ALL)" << endl;
        } else {
            cout << "  HEURISTIC ALGORITHMS COMPARISON TEST" << endl;
        }
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
        
        ComparisonReport report;
        
        // Determine which algorithms to run
        int startAlg = includeOptimal ? 1 : 2;  // Start from 1 (BruteForce) or 2 (Greedy)
        int endAlg = 3;  // Always end with 3 (HillClimbing)
        
        // Run each algorithm
        for (int alg = startAlg; alg <= endAlg; alg++) {
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
            
            // Execute algorithm and collect result
            AlgorithmResult result = currentAlgorithm->execute(
                G,
                availableRobots,
                busyRobots,
                chargingRobots,
                pendingTasks,
                totalRobots,
                true  // compactMode = true for comparison
            );
            
            // Store result in comparison report
            if (alg == 1) {
                report.bruteForceResult = result;
            } else if (alg == 2) {
                report.greedyResult = result;
            } else if (alg == 3) {
                report.hillClimbingResult = result;
            }
            
            cout << endl;
        }
        
        // Print comparison summary
        cout << "========================================" << endl;
        cout << "  COMPARISON SUMMARY" << endl;
        cout << "========================================" << endl;
        
        if (report.bruteForceResult.has_value()) {
            cout << "Brute Force:   Makespan = " << fixed << setprecision(2) 
                 << report.bruteForceResult->makespan << "s, Time = " 
                 << report.bruteForceResult->computationTimeMs << "ms (Optimal)" << endl;
        }
        if (report.greedyResult.has_value()) {
            cout << "Greedy:        Makespan = " << fixed << setprecision(2) 
                 << report.greedyResult->makespan << "s, Time = " 
                 << report.greedyResult->computationTimeMs << "ms" << endl;
        }
        if (report.hillClimbingResult.has_value()) {
            cout << "Hill Climbing: Makespan = " << fixed << setprecision(2) 
                 << report.hillClimbingResult->makespan << "s, Time = " 
                 << report.hillClimbingResult->computationTimeMs << "ms" << endl;
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