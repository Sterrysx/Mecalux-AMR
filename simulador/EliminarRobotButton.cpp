#include "EliminarRobotButton.h"

EliminarRobotButton::EliminarRobotButton(QWidget *parent) : QPushButton(parent), RobotID(-1)
{

}

void EliminarRobotButton::modificarSelectedRobot(int robotID){
    RobotID = robotID;
}
