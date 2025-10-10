#include "../include/Robot.hh"

// Constructors and Destructor
Robot::Robot() : robotId(-1), position({0.0, 0.0}), batteryLevel(100.0), 
                 currentTask(""), maxSpeed(1.6), loadCapacity(100), isAvailable(true) {}

Robot::Robot(int id, pair<double, double> pos, double battery, string task, 
             float speed, float capacity, bool available)
    : robotId(id), position(pos), batteryLevel(battery), currentTask(task),
      maxSpeed(speed), loadCapacity(static_cast<int>(capacity)), isAvailable(available) {}

Robot::~Robot() {}

// Getters
int Robot::getId() const { return robotId; }
pair<double, double> Robot::getPosition() const { return position; }
double Robot::getBatteryLevel() const { return batteryLevel; }
string Robot::getCurrentTask() const { return currentTask; }
float Robot::getMaxSpeed() const { return maxSpeed; }
float Robot::getLoadCapacity() const { return static_cast<float>(loadCapacity); }
bool Robot::getAvailability() const { return isAvailable; }

// Setters
void Robot::setPosition(const pair<double, double>& pos) { position = pos; }
void Robot::setBatteryLevel(double battery) { batteryLevel = battery; }
void Robot::setCurrentTask(const string& task) { currentTask = task; }
void Robot::setMaxSpeed(float speed) { maxSpeed = speed; }
void Robot::setLoadCapacity(int capacity) { loadCapacity = capacity; }
void Robot::setAvailability(bool available) { isAvailable = available; }
