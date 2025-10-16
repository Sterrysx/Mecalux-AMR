#include "SimulationController.hh"
#include "algorithms/01_BruteForce.hh"
#include "algorithms/02_Greedy.hh"
#include "algorithms/03_HillClimbing.hh"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

using namespace std;

SimulationController::SimulationController(
    const Graph& graph, 
    int numRobots, 
    queue<Task> initialTasks,
    double timeStep
) : graph(graph),
    running(false),
    paused(false),
    simulationTime(0.0),
    timeStep(timeStep),
    schedulingAlgorithm(2),  // Default to Greedy
    schedulerInterval(5.0),  // Run scheduler every 5 seconds
    lastSchedulerRun(0.0),
    tasksCompleted(0),
    tasksAdded(initialTasks.size()),
    schedulerCycles(0)
{
    // Create the Planifier instance
    planifier = make_unique<Planifier>(graph, numRobots, initialTasks);
    
    // Set default algorithm (Greedy)
    planifier->setAlgorithm(make_unique<Greedy>());
}

SimulationController::~SimulationController() {
    if (running) {
        stop();
    }
}

void SimulationController::start() {
    if (running) {
        cout << "Simulation is already running!" << endl;
        return;
    }
    
    running = true;
    paused = false;
    
    cout << "========================================" << endl;
    cout << "  REAL-TIME WAREHOUSE SIMULATION" << endl;
    cout << "========================================" << endl;
    cout << "Graph: " << graph.getNumVertices() << " nodes" << endl;
    cout << "Robots: " << planifier->getNumRobots() << endl;
    cout << "Initial Tasks: " << tasksAdded << endl;
    cout << "Time Step: " << timeStep << "s" << endl;
    cout << "Scheduler Interval: " << schedulerInterval << "s" << endl;
    cout << endl;
    cout << "Type 'help' for available commands." << endl;
    cout << "========================================" << endl;
    cout << endl;
    
    // Start input handling thread
    inputThread = thread(&SimulationController::handleInput, this);
    
    // Run main simulation loop
    simulationLoop();
}

void SimulationController::stop() {
    if (!running) {
        return;
    }
    
    cout << endl;
    cout << "Stopping simulation..." << endl;
    running = false;
    
    // Wait for input thread to complete
    if (inputThread.joinable()) {
        inputThread.join();
    }
    
    cout << endl;
    cout << "========================================" << endl;
    cout << "  SIMULATION SUMMARY" << endl;
    cout << "========================================" << endl;
    cout << "Total Time: " << fixed << setprecision(2) << simulationTime << "s" << endl;
    cout << "Tasks Added: " << tasksAdded << endl;
    cout << "Tasks Completed: " << tasksCompleted << endl;
    cout << "Scheduler Cycles: " << schedulerCycles << endl;
    cout << "========================================" << endl;
}

bool SimulationController::isRunning() const {
    return running;
}

void SimulationController::setSchedulingAlgorithm(int algorithmChoice) {
    lock_guard<mutex> lock(statusMutex);
    
    schedulingAlgorithm = algorithmChoice;
    
    // Update the Planifier's algorithm
    switch (algorithmChoice) {
        case 1:
            planifier->setAlgorithm(make_unique<BruteForce>());
            cout << "Switched to BruteForce algorithm" << endl;
            break;
        case 2:
            planifier->setAlgorithm(make_unique<Greedy>());
            cout << "Switched to Greedy algorithm" << endl;
            break;
        case 3:
            planifier->setAlgorithm(make_unique<HillClimbing>());
            cout << "Switched to Hill Climbing algorithm" << endl;
            break;
        default:
            cout << "Invalid algorithm choice. Using Greedy." << endl;
            planifier->setAlgorithm(make_unique<Greedy>());
            schedulingAlgorithm = 2;
            break;
    }
}

void SimulationController::addTask(const Task& task) {
    lock_guard<mutex> lock(taskMutex);
    
    // Get current pending tasks, add new one, update planifier
    queue<Task> currentTasks = planifier->getPendingTasks();
    currentTasks.push(task);
    
    // Update the planifier's task queue
    // Note: This is a simplified approach. In a real implementation,
    // you might want to add a method to Planifier to add tasks directly
    planifier->setPendingTasks(currentTasks.size());
    while (!currentTasks.empty()) {
        currentTasks.pop();
    }
    
    tasksAdded++;
    
    cout << "[T=" << fixed << setprecision(1) << simulationTime 
         << "s] New task added: " << task.getTaskId() 
         << " (" << task.getOriginNode() << " -> " << task.getDestinationNode() << ")" << endl;
}

void SimulationController::printStatus() const {
    lock_guard<mutex> lock(statusMutex);
    
    cout << endl;
    cout << "--- Simulation Status ---" << endl;
    cout << "Time: " << fixed << setprecision(2) << simulationTime << "s" << endl;
    cout << "Robots - Available: " << planifier->getAvailableRobots()
         << " | Busy: " << planifier->getBusyRobots()
         << " | Charging: " << planifier->getChargingRobots() << endl;
    cout << "Tasks - Pending: " << planifier->getPendingTasks().size()
         << " | Completed: " << tasksCompleted << endl;
    cout << "Scheduler Cycles: " << schedulerCycles << endl;
    cout << "Status: " << (paused ? "PAUSED" : "RUNNING") << endl;
    cout << "-------------------------" << endl;
}

void SimulationController::simulationLoop() {
    auto lastUpdateTime = chrono::high_resolution_clock::now();
    
    while (running) {
        if (paused) {
            // Sleep briefly when paused
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }
        
        // Calculate delta time
        auto currentTime = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = currentTime - lastUpdateTime;
        lastUpdateTime = currentTime;
        
        double deltaTime = min(elapsed.count(), timeStep * 2.0); // Cap delta time
        
        // Update simulation time
        simulationTime += deltaTime;
        
        // Update robot states
        updateRobots(deltaTime);
        
        // Run scheduler if interval has elapsed
        if (simulationTime - lastSchedulerRun >= schedulerInterval) {
            runSchedulerCycle();
            lastSchedulerRun = simulationTime;
        }
        
        // Sleep to maintain time step
        this_thread::sleep_for(chrono::duration<double>(timeStep));
    }
}

void SimulationController::updateRobots(double deltaTime) {
    // This is a placeholder for robot state updates
    // In a full implementation, this would:
    // 1. Move robots along their paths
    // 2. Update battery levels
    // 3. Detect task completions
    // 4. Move robots between queues (busy -> available, available -> charging, etc.)
    
    // For now, we'll just update battery levels as a demonstration
    // Note: This would require access to robot queues and methods to update them
    
    // TODO: Implement robot movement and state transitions
    // This requires either:
    // - Making Planifier expose its robot queues
    // - Adding update methods to Planifier
    // - Implementing robot state management here
}

void SimulationController::runSchedulerCycle() {
    lock_guard<mutex> lock(taskMutex);
    
    // Only run scheduler if there are pending tasks and available robots
    if (planifier->getPendingTasks().empty() || planifier->getAvailableRobots() == 0) {
        return;
    }
    
    cout << endl;
    cout << "[T=" << fixed << setprecision(1) << simulationTime 
         << "s] ========== SCHEDULER CYCLE " << (schedulerCycles + 1) << " ==========" << endl;
    
    // Execute the current algorithm
    AlgorithmResult result = planifier->executePlan();
    
    schedulerCycles++;
    
    cout << "[T=" << simulationTime << "s] Scheduler cycle complete. Makespan: " 
         << result.makespan << "s" << endl;
    cout << "========================================" << endl;
}

void SimulationController::handleInput() {
    string command;
    
    while (running) {
        // Non-blocking input handling
        if (cin.peek() != EOF) {
            getline(cin, command);
            processCommand(command);
        }
        
        // Sleep briefly to avoid busy waiting
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void SimulationController::processCommand(const string& command) {
    if (command.empty()) {
        return;
    }
    
    stringstream ss(command);
    string cmd;
    ss >> cmd;
    
    // Convert to lowercase for case-insensitive commands
    for (char& c : cmd) {
        c = tolower(c);
    }
    
    if (cmd == "help" || cmd == "h") {
        printHelp();
    }
    else if (cmd == "status" || cmd == "s") {
        printStatus();
    }
    else if (cmd == "pause" || cmd == "p") {
        paused = !paused;
        cout << "Simulation " << (paused ? "paused" : "resumed") << endl;
    }
    else if (cmd == "quit" || cmd == "q" || cmd == "exit") {
        cout << "Quitting simulation..." << endl;
        running = false;
    }
    else if (cmd == "add" || cmd == "task" || cmd == "t") {
        parseAndAddTask(command);
    }
    else if (cmd == "algorithm" || cmd == "algo" || cmd == "a") {
        int algoChoice;
        ss >> algoChoice;
        if (ss.fail() || algoChoice < 1 || algoChoice > 3) {
            cout << "Usage: algorithm <1-3> (1=BruteForce, 2=Greedy, 3=HillClimbing)" << endl;
        } else {
            setSchedulingAlgorithm(algoChoice);
        }
    }
    else if (cmd == "interval" || cmd == "i") {
        double newInterval;
        ss >> newInterval;
        if (ss.fail() || newInterval <= 0) {
            cout << "Usage: interval <seconds>" << endl;
        } else {
            schedulerInterval = newInterval;
            cout << "Scheduler interval set to " << newInterval << "s" << endl;
        }
    }
    else {
        cout << "Unknown command: " << cmd << ". Type 'help' for available commands." << endl;
    }
}

void SimulationController::printHelp() const {
    cout << endl;
    cout << "=== Available Commands ===" << endl;
    cout << "  help, h              - Show this help message" << endl;
    cout << "  status, s            - Display current simulation status" << endl;
    cout << "  pause, p             - Pause/resume simulation" << endl;
    cout << "  quit, q, exit        - Stop simulation and exit" << endl;
    cout << "  add <id> <from> <to> - Add a new task" << endl;
    cout << "                         Example: add 100 5 12" << endl;
    cout << "  algorithm <1-3>      - Change scheduling algorithm" << endl;
    cout << "                         1 = BruteForce" << endl;
    cout << "                         2 = Greedy (default)" << endl;
    cout << "                         3 = Hill Climbing" << endl;
    cout << "  interval <seconds>   - Set scheduler run interval" << endl;
    cout << "                         Example: interval 10" << endl;
    cout << "==========================" << endl;
}

void SimulationController::parseAndAddTask(const string& input) {
    stringstream ss(input);
    string cmd;
    int taskId, origin, destination;
    
    ss >> cmd >> taskId >> origin >> destination;
    
    if (ss.fail()) {
        cout << "Usage: add <taskId> <originNode> <destinationNode>" << endl;
        cout << "Example: add 100 5 12" << endl;
        return;
    }
    
    // Validate nodes exist in graph
    if (origin < 0 || origin >= graph.getNumVertices() || 
        destination < 0 || destination >= graph.getNumVertices()) {
        cout << "Error: Invalid node IDs. Graph has " << graph.getNumVertices() << " nodes." << endl;
        return;
    }
    
    Task newTask(taskId, origin, destination);
    addTask(newTask);
}
