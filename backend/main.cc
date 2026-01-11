/**
 * @file main.cc
 * @brief Entry point for the Mecalux AMR Fleet Management System
 * 
 * This is the main executable that demonstrates:
 * - Scenario A: Boot-up with initial tasks (Full VRP Solve)
 * - Scenario B: Dynamic task injection - small batch (Cheap Insertion)
 * - Scenario C: Dynamic task injection - large batch (Background Re-plan)
 */

#include "FleetManager.hh"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>

// Global flag for signal handling
std::atomic<bool> g_running(true);

void signalHandler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down...\n";
    g_running = false;
}

void printUsage(const char* programName) {
    std::cout << "\n";
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --help        Show this help message\n";
    std::cout << "  --tasks FILE  Path to tasks JSON file (default: ../api/set_of_tasks.json)\n";
    std::cout << "  --robots N    Number of robots (0 = auto from charging stations, default: 0)\n";
    std::cout << "  --duration S  Run for S seconds then exit (default: run until Enter)\n";
    std::cout << "  --batch       Batch mode: no sleep, max speed, auto-terminate when done\n";
    std::cout << "  --demo        Run dynamic task injection demo (Scenarios A, B, C)\n";
    std::cout << "  --cli         Interactive CLI mode for live task injection\n";
    std::cout << "  --from-json   Indicate data was imported from JSON (display info)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << "\n";
    std::cout << "  " << programName << " --tasks custom_tasks.json --robots 5\n";
    std::cout << "  " << programName << " --duration 60\n";
    std::cout << "  " << programName << " --batch  (runs all tasks as fast as possible)\n";
    std::cout << "  " << programName << " --demo   (demonstrates dynamic task injection)\n";
    std::cout << "  " << programName << " --cli   (interactive command line interface)\n";
    std::cout << "  " << programName << " --from-json  (use data generated from JSON warehouse)\n";
    std::cout << "\n";
}

/**
 * @brief Create random tasks between POI nodes.
 * 
 * This simulates real-world task arrival where new pickup/dropoff
 * requests come in dynamically.
 */
std::vector<Backend::Layer2::Task> createRandomTasks(
    int count, 
    int startId,
    const std::vector<int>& pickupNodes,
    const std::vector<int>& dropoffNodes
) {
    std::vector<Backend::Layer2::Task> tasks;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> pickupDist(0, static_cast<int>(pickupNodes.size()) - 1);
    std::uniform_int_distribution<> dropoffDist(0, static_cast<int>(dropoffNodes.size()) - 1);
    
    for (int i = 0; i < count; ++i) {
        Backend::Layer2::Task t;
        t.taskId = startId + i;
        t.sourceNode = pickupNodes[pickupDist(gen)];
        t.destinationNode = dropoffNodes[dropoffDist(gen)];
        tasks.push_back(t);
    }
    
    return tasks;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string taskPath = "../api/set_of_tasks.json";
    int numRobots = 0;  // 0 = auto from charging stations
    int duration = -1;  // -1 means run until Enter pressed
    bool batchMode = false;
    bool demoMode = false;
    bool cliMode = false;
    bool fromJson = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--tasks" && i + 1 < argc) {
            taskPath = argv[++i];
        }
        else if (arg == "--robots" && i + 1 < argc) {
            numRobots = std::stoi(argv[++i]);
        }
        else if (arg == "--duration" && i + 1 < argc) {
            duration = std::stoi(argv[++i]);
        }
        else if (arg == "--batch") {
            batchMode = true;
        }
        else if (arg == "--demo") {
            demoMode = true;
        }
        else if (arg == "--cli") {
            cliMode = true;
        }
        else if (arg == "--from-json") {
            fromJson = true;
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Print banner
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                           ║\n";
    std::cout << "║    ███╗   ███╗███████╗ ██████╗ █████╗ ██╗     ██╗   ██╗██╗  ██╗           ║\n";
    std::cout << "║    ████╗ ████║██╔════╝██╔════╝██╔══██╗██║     ██║   ██║╚██╗██╔╝           ║\n";
    std::cout << "║    ██╔████╔██║█████╗  ██║     ███████║██║     ██║   ██║ ╚███╔╝            ║\n";
    std::cout << "║    ██║╚██╔╝██║██╔══╝  ██║     ██╔══██║██║     ██║   ██║ ██╔██╗            ║\n";
    std::cout << "║    ██║ ╚═╝ ██║███████╗╚██████╗██║  ██║███████╗╚██████╔╝██╔╝ ██╗           ║\n";
    std::cout << "║    ╚═╝     ╚═╝╚══════╝ ╚═════╝╚═╝  ╚═╝╚══════╝ ╚═════╝╚═╝  ╚═╝           ║\n";
    std::cout << "║                                                                           ║\n";
    std::cout << "║                  AMR FLEET MANAGEMENT SYSTEM v1.0                         ║\n";
    std::cout << "║                                                                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Set up signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Create configuration
    Backend::SystemConfig config;
    config.numRobots = numRobots;
    config.robotSpeedMps = 1.6f;  // Consistent with Layer 3
    config.robotRadiusMeters = 0.2f;
    config.orcaTickMs = 50.0f;   // 20 Hz physics
    config.warehouseTickMs = 1000.0f;  // 1 Hz strategic
    config.batchMode = batchMode;
    
    // If using JSON-imported data, ensure we use the generated files
    if (fromJson) {
        config.mapPath = "layer1/assets/map_layout.txt";
        config.poiConfigPath = "layer1/assets/poi_config.json";
    }
    
    if (batchMode) {
        std::cout << "[Main] BATCH MODE: Running at maximum speed, will auto-terminate\n";
    }
    
    // If using JSON-imported data, show info banner
    if (fromJson) {
        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                   RUNNING WITH JSON-IMPORTED DATA                         ║\n";
        std::cout << "╠═══════════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║  Map:  layer1/assets/map_layout.txt (generated from JSON)                ║\n";
        std::cout << "║  POIs: layer1/assets/poi_config.json (extracted from JSON)               ║\n";
        std::cout << "║  Tasks: " << taskPath << std::string(51 - taskPath.length(), ' ') << "║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
    }
    
    // Create FleetManager
    std::cout << "[Main] Creating FleetManager...\n";
    Backend::FleetManager manager(config, ".");
    
    // Initialize the system
    if (!manager.Initialize()) {
        std::cerr << "[Main] ERROR: Failed to initialize FleetManager!\n";
        return 1;
    }
    
    // Load tasks
    int numTasks = manager.LoadTasks(taskPath);
    if (numTasks == 0) {
        std::cerr << "[Main] Warning: No tasks loaded from " << taskPath << "\n";
    }
    
    // Set up callbacks
    manager.SetOnRobotArrived([](int robotId, int nodeId) {
        std::cout << "  [Event] Robot " << robotId << " arrived at node " << nodeId << "\n";
    });
    
    // Start the system
    std::cout << "\n[Main] Starting fleet management system...\n";
    manager.Start();
    
    // Print initial state (skip in batch mode for speed)
    if (!batchMode) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        manager.PrintRobotStates();
    }
    
    // Main loop
    if (batchMode) {
        // BATCH MODE: Run until all tasks complete
        std::cout << "\n[Main] BATCH MODE: Running until all tasks complete...\n";
        std::cout << "[Main] Press Ctrl+C to abort\n\n";
        
        auto batchStart = std::chrono::steady_clock::now();
        int lastPrint = 0;
        
        while (g_running.load() && !manager.IsAllTasksComplete()) {
            // In batch mode, just poll for completion
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Print progress every second
            auto now = std::chrono::steady_clock::now();
            int elapsed = static_cast<int>(std::chrono::duration<double>(now - batchStart).count());
            if (elapsed > lastPrint) {
                auto stats = manager.GetStats();
                std::cout << "[Batch] " << elapsed << "s: "
                          << stats.fleetLoopCount << " physics ticks, "
                          << stats.completedTasks << "/" << stats.totalTasks << " tasks\n";
                lastPrint = elapsed;
            }
        }
        
        auto batchEnd = std::chrono::steady_clock::now();
        double totalTime = std::chrono::duration<double>(batchEnd - batchStart).count();
        
        if (manager.IsAllTasksComplete()) {
            std::cout << "\n[Main] ═══════════════ BATCH COMPLETE ═══════════════\n";
            std::cout << "[Main] All tasks completed in " << std::fixed << std::setprecision(2) 
                      << totalTime << " seconds\n";
        } else {
            std::cout << "\n[Main] Batch mode aborted by user\n";
        }
        
    } else if (demoMode) {
        // DEMO MODE: Demonstrate dynamic task injection scenarios
        std::cout << "\n[Main] ═══════════════ DEMO MODE ═══════════════\n";
        std::cout << "[Main] This will demonstrate:\n";
        std::cout << "  - Scenario A: Boot with initial tasks (Full VRP Solve)\n";
        std::cout << "  - Scenario B: Inject few tasks (Cheap Insertion)\n";
        std::cout << "  - Scenario C: Inject many tasks (Background Re-plan)\n";
        std::cout << "\n";
        
        // Define POI nodes for random task generation
        // These are sample nodes that exist in the graph
        std::vector<int> pickupNodes = {20, 25, 30, 35, 40, 45, 50, 55};
        std::vector<int> dropoffNodes = {60, 65, 70, 75, 80, 85, 90, 95};
        
        auto demoStart = std::chrono::steady_clock::now();
        int taskIdCounter = 1000;
        
        // Wait for Scenario A (initial VRP) to complete
        std::cout << "[Demo] Scenario A: Waiting for initial VRP solve...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Scenario B: Inject a few tasks (should trigger cheap insertion)
        std::cout << "\n[Demo] ═══════════════ SCENARIO B ═══════════════\n";
        std::cout << "[Demo] Injecting 3 tasks (should use CHEAP INSERTION)\n";
        auto scenarioBTasks = createRandomTasks(3, taskIdCounter, pickupNodes, dropoffNodes);
        taskIdCounter += 3;
        manager.InjectTasks(scenarioBTasks);
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Scenario C: Inject many tasks (should trigger background re-plan)
        std::cout << "\n[Demo] ═══════════════ SCENARIO C ═══════════════\n";
        std::cout << "[Demo] Injecting 10 tasks (should trigger BACKGROUND RE-PLAN)\n";
        auto scenarioCTasks = createRandomTasks(10, taskIdCounter, pickupNodes, dropoffNodes);
        taskIdCounter += 10;
        manager.InjectTasks(scenarioCTasks);
        
        // Wait for background replan to complete
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        // Final status
        auto demoEnd = std::chrono::steady_clock::now();
        double totalTime = std::chrono::duration<double>(demoEnd - demoStart).count();
        
        std::cout << "\n[Demo] ═══════════════ DEMO COMPLETE ═══════════════\n";
        std::cout << "[Demo] Demo ran for " << std::fixed << std::setprecision(1) 
                  << totalTime << " seconds\n";
        std::cout << "[Demo] Injected " << (taskIdCounter - 1000) << " additional tasks\n";
        
    } else if (cliMode) {
        // CLI MODE: Interactive command line interface for live task injection
        std::cout << "\n[CLI] ═══════════════ INTERACTIVE CLI MODE ═══════════════\n";
        std::cout << "[CLI] Available commands:\n";
        std::cout << "       inject <N>  - Inject N random tasks (≤5 = Cheap Insertion, >5 = Background Re-plan)\n";
        std::cout << "       status      - Show robot states and queue info\n";
        std::cout << "       stats       - Show system statistics\n";
        std::cout << "       help        - Show this help message\n";
        std::cout << "       quit        - Stop the system and exit\n";
        std::cout << "\n[CLI] Threshold: ≤5 tasks triggers Cheap Insertion, >5 triggers Background Re-plan\n";
        std::cout << "\n";
        
        // Get POI nodes from FleetManager
        std::vector<int> pickupNodes = manager.GetPickupNodes();
        std::vector<int> dropoffNodes = manager.GetDropoffNodes();
        
        // Check if we have valid POI nodes
        if (pickupNodes.empty() || dropoffNodes.empty()) {
            std::cout << "[CLI] Warning: No POI nodes found, using fallback node ranges\n";
            pickupNodes = {20, 25, 30, 35, 40, 45, 50, 55};
            dropoffNodes = {60, 65, 70, 75, 80, 85, 90, 95};
        } else {
            std::cout << "[CLI] Found " << pickupNodes.size() << " pickup nodes, "
                      << dropoffNodes.size() << " dropoff nodes\n";
        }
        
        int taskIdCounter = 1000;
        std::string line;
        
        std::cout << "\n[CLI] Ready. Type 'help' for commands.\n";
        std::cout << "amr> " << std::flush;
        
        while (g_running.load() && std::getline(std::cin, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) {
                std::cout << "amr> " << std::flush;
                continue;
            }
            size_t end = line.find_last_not_of(" \t");
            line = line.substr(start, end - start + 1);
            
            // Parse command
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            
            if (cmd == "quit" || cmd == "exit" || cmd == "q") {
                std::cout << "[CLI] Shutting down...\n";
                break;
                
            } else if (cmd == "help" || cmd == "h" || cmd == "?") {
                std::cout << "\n[CLI] Available commands:\n";
                std::cout << "       inject <N>  - Inject N random tasks\n";
                std::cout << "                     (≤5 = Cheap Insertion, >5 = Background Re-plan)\n";
                std::cout << "       status      - Show robot states and queue info\n";
                std::cout << "       stats       - Show system statistics\n";
                std::cout << "       help        - Show this help message\n";
                std::cout << "       quit        - Stop the system and exit\n\n";
                
            } else if (cmd == "inject") {
                int count = 0;
                if (iss >> count && count > 0) {
                    std::cout << "\n[CLI] Injecting " << count << " random tasks...\n";
                    
                    // Announce which strategy will be used
                    if (count <= 5) {
                        std::cout << "[CLI] Strategy: CHEAP INSERTION (≤5 tasks)\n";
                    } else {
                        std::cout << "[CLI] Strategy: BACKGROUND RE-PLAN (>5 tasks)\n";
                    }
                    
                    auto tasks = createRandomTasks(count, taskIdCounter, pickupNodes, dropoffNodes);
                    taskIdCounter += count;
                    
                    // Print task details
                    std::cout << "[CLI] Generated tasks:\n";
                    for (const auto& t : tasks) {
                        std::cout << "       Task " << t.taskId << ": pickup=" << t.sourceNode 
                                  << " -> dropoff=" << t.destinationNode << "\n";
                    }
                    
                    manager.InjectTasks(tasks);
                    std::cout << "[CLI] Tasks injected successfully!\n\n";
                    
                } else {
                    std::cout << "[CLI] Usage: inject <N> where N is a positive integer\n";
                }
                
            } else if (cmd == "status") {
                std::cout << "\n";
                manager.PrintRobotStates();
                std::cout << "\n";
                
            } else if (cmd == "stats") {
                auto stats = manager.GetStats();
                std::cout << "\n[CLI] ═══════════════ STATISTICS ═══════════════\n";
                std::cout << "[CLI] Simulation time: " << std::fixed << std::setprecision(2) 
                          << stats.simulationTime << "s\n";
                std::cout << "[CLI] Fleet loop count: " << stats.fleetLoopCount << "\n";
                std::cout << "[CLI] Tasks completed: " << stats.completedTasks 
                          << " / " << stats.totalTasks << "\n";
                std::cout << "[CLI] Dynamic tasks injected: " << (taskIdCounter - 1000) << "\n\n";
                
            } else if (!cmd.empty()) {
                std::cout << "[CLI] Unknown command: '" << cmd << "'. Type 'help' for commands.\n";
            }
            
            std::cout << "amr> " << std::flush;
        }
        
    } else if (duration > 0) {
        // Run for specified duration
        std::cout << "\n[Main] Running for " << duration << " seconds...\n";
        std::cout << "[Main] Press Ctrl+C to stop early\n\n";
        
        for (int i = 0; i < duration && g_running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print status every 5 seconds
            if ((i + 1) % 5 == 0) {
                manager.PrintRobotStates();
                auto stats = manager.GetStats();
                std::cout << "[Main] Simulation time: " << std::fixed << std::setprecision(1) 
                          << stats.simulationTime << "s, Fleet loops: " << stats.fleetLoopCount << "\n";
            }
        }
    } else {
        // Run until user presses Enter
        std::cout << "\n[Main] System running. Press Enter to stop...\n";
        std::cout << "[Main] Press Ctrl+C to force stop\n\n";
        
        // Background status printer
        std::thread statusThread([&manager]() {
            int counter = 0;
            while (g_running.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                counter++;
                
                // Print status every 10 seconds
                if (counter % 10 == 0 && g_running.load()) {
                    manager.PrintRobotStates();
                }
            }
        });
        
        // Wait for user input
        std::string input;
        std::getline(std::cin, input);
        
        g_running = false;
        statusThread.join();
    }
    
    // Stop the system
    std::cout << "\n[Main] Stopping fleet management system...\n";
    manager.Stop();
    
    // Print final statistics
    auto stats = manager.GetStats();
    stats.Print();
    
    std::cout << "\n[Main] Fleet Management System terminated successfully.\n";
    std::cout << "       Thank you for using Mecalux AMR!\n\n";
    
    return 0;
}
