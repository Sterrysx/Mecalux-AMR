#ifndef ROBOT_H
#define ROBOT_H

#include <vector>
#include <unordered_map>
#include <set>
#include <utility>
#include <iostream>
#include <string>


class Robot {

public:
    // Constructors and Destructor
    Robot();
    Robot(int id, std::pair<double, double> pos, double battery, int task, float speed, float capacity);
    ~Robot();

    // Getters
    int getId() const;
    std::pair<double, double> getPosition() const;
    double getBatteryLevel() const;
    int getCurrentTask() const;
    float getMaxSpeed() const;
    float getLoadCapacity() const;
    bool getAvailability() const;

    // Setters
    void setPosition(const std::pair<double, double>& pos);
    void setBatteryLevel(double battery);
    void setCurrentTask(int task);
    void setMaxSpeed(float speed);
    void setLoadCapacity(int capacity);
    void freeRobot(); //posa currentTask a -1 i isAvailable a true
    void updateBattery(double time);
private:
    int robotId;
    std::pair<double, double> position;
    double batteryLevel;
    int currentTask;
    float maxSpeed;
    int loadCapacity;
    const int batteryLifeSpan = 3600;    
    const double alpha = 2;
};

#endif // ROBOT_H