#include "../include/Robot.hh"

using namespace std;

// Constructors and Destructor
Robot::Robot() : robotId(-1), position({0.0, 0.0}), batteryLevel(100.0), 
                 currentTask(-1), maxSpeed(1.6), loadCapacity(100) {}

Robot::Robot(int id, pair<double, double> pos, double battery, int task, 
             float speed, float capacity)
    : robotId(id), position(pos), batteryLevel(battery), currentTask(task),
      maxSpeed(speed), loadCapacity(static_cast<int>(capacity)){}

Robot::~Robot() {}  

// Getters
int Robot::getId() const { return robotId; }
pair<double, double> Robot::getPosition() const { return position; }
double Robot::getBatteryLevel() const { return batteryLevel; }
int Robot::getCurrentTask() const { return currentTask; }
float Robot::getMaxSpeed() const { return maxSpeed; }
float Robot::getLoadCapacity() const { return static_cast<float>(loadCapacity); }
bool Robot::getAvailability() const { return currentTask!=-1; }
double Robot::getAlpha() const { return alpha; }
int Robot::getBatteryLifeSpan() const { return batteryLifeSpan; }
int Robot::getBatteryRechargeTime() const { return batteryRechargeTime; }
double Robot::getBatteryRechargeRate() const { return 100.0 / batteryRechargeTime; } // % per second

// Setters
void Robot::setPosition(const pair<double, double>& pos) { position = pos; }
void Robot::setBatteryLevel(double battery) { batteryLevel = battery; }
void Robot::setCurrentTask(int task) { currentTask = task; }
void Robot::setMaxSpeed(float speed) { maxSpeed = speed; }
void Robot::setLoadCapacity(int capacity) { loadCapacity = capacity; }
void Robot::freeRobot() {
    currentTask = -1; 
}


void Robot::updateBattery(double time){
  double percentageConsume = 100/(double)batteryLifeSpan;
  batteryLevel -= time * (getAvailability() ?  percentageConsume : percentageConsume*alpha);
}