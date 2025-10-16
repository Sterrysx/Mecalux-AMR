#ifndef SIMULATION_CONTROLLER_HH
#define SIMULATION_CONTROLLER_HH

#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <queue>
#include <vector>
#include "Planifier.hh"
#include "Graph.hh"
#include "Task.hh"
#include "Robot.hh"

/**
 * @brief Controller for real-time warehouse simulation
 * 
 * This class manages the main simulation loop, handles time progression,
 * processes user input asynchronously, and coordinates with the Planifier
 * to schedule and execute tasks dynamically.
 * 
 * Architecture:
 * - Main thread: Runs the simulation loop (updateRobots, updateScheduler)
 * - Input thread: Handles user commands asynchronously
 * - Uses Planifier as a library for state management and scheduling
 */
class SimulationController {
public:
    /**
     * @brief Construct a new Simulation Controller
     * 
     * @param graph The warehouse graph structure
     * @param numRobots Number of robots in the simulation
     * @param initialTasks Initial queue of tasks to process
     * @param timeStep Time step for simulation updates (seconds)
     */
    SimulationController(
        const Graph& graph, 
        int numRobots, 
        std::queue<Task> initialTasks,
        double timeStep = 0.1
    );

    /**
     * @brief Destroy the Simulation Controller
     */
    ~SimulationController();

    /**
     * @brief Start the simulation
     * 
     * This method launches the input thread and starts the main simulation loop.
     * The simulation runs until stop() is called or the user issues a quit command.
     */
    void start();

    /**
     * @brief Stop the simulation
     * 
     * Signals the simulation to stop and waits for threads to complete.
     */
    void stop();

    /**
     * @brief Check if simulation is running
     * 
     * @return true if simulation is active
     */
    bool isRunning() const;

    /**
     * @brief Set the scheduling algorithm to use
     * 
     * @param algorithmChoice 1 = BruteForce, 2 = Greedy, 3 = HillClimbing
     */
    void setSchedulingAlgorithm(int algorithmChoice);

    /**
     * @brief Add a new task to the pending queue
     * 
     * This is thread-safe and can be called from the input thread.
     * 
     * @param task The task to add
     */
    void addTask(const Task& task);

    /**
     * @brief Get current simulation statistics
     */
    void printStatus() const;

private:
    // Core components
    std::unique_ptr<Planifier> planifier;
    const Graph& graph;
    
    // Simulation state
    std::atomic<bool> running;
    std::atomic<bool> paused;
    double simulationTime;      // Current simulation time in seconds
    double timeStep;            // Time increment per update cycle
    int schedulingAlgorithm;    // Current algorithm choice (1-3)
    double schedulerInterval;   // How often to run scheduler (seconds)
    double lastSchedulerRun;    // Last time scheduler was executed
    
    // Threading
    std::thread inputThread;
    mutable std::mutex taskMutex;       // Protects task queue operations
    mutable std::mutex statusMutex;     // Protects status printing
    
    // Statistics
    int tasksCompleted;
    int tasksAdded;
    int schedulerCycles;
    
    // Core simulation methods
    /**
     * @brief Main simulation loop
     * 
     * Continuously updates robot states and runs scheduler at intervals
     */
    void simulationLoop();
    
    /**
     * @brief Update all robots' states
     * 
     * @param deltaTime Time elapsed since last update
     */
    void updateRobots(double deltaTime);
    
    /**
     * @brief Run a scheduling cycle if conditions are met
     * 
     * Triggers the Planifier to assign tasks to available robots
     */
    void runSchedulerCycle();
    
    /**
     * @brief Input handling thread function
     * 
     * Continuously reads user commands and processes them
     */
    void handleInput();
    
    /**
     * @brief Process a user command
     * 
     * @param command The command string to process
     */
    void processCommand(const std::string& command);
    
    /**
     * @brief Display available commands to user
     */
    void printHelp() const;
    
    /**
     * @brief Parse and add a task from user input
     * 
     * @param input User input string with task parameters
     */
    void parseAndAddTask(const std::string& input);
};

#endif // SIMULATION_CONTROLLER_HH
