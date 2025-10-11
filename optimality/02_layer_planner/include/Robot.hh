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
    double getAlpha() const;
    int getBatteryLifeSpan() const;
    int getBatteryRechargeTime() const;
    double getBatteryRechargeRate() const; // Returns percentage per second

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
    const int batteryLifeSpan = 300; // Battery life span in seconds
    const int batteryRechargeTime = 50; // Battery time to recharge from 0% to 100% in seconds
    const double alpha = 2;
};

#endif // ROBOT_H