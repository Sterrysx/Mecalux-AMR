#include "AfegirRobotButton.h"

AfegirRobotButton::AfegirRobotButton(QWidget *parent) : QPushButton(parent), x(0), y(0)
{

}

void AfegirRobotButton::modificarPosicioY(int newY){
    y = newY;
}

void AfegirRobotButton::modificarPosicioX(int newX){
    x = newX;
}

void AfegirRobotButton::crearRobotSlot(){
    emit crearRobot(x, y);
}