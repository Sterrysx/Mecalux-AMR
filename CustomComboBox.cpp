#include "CustomComboBox.h"

CustomComboBox::CustomComboBox(QWidget *parent) : QComboBox(parent) {


}

void CustomComboBox::addItem(int value) {
    QComboBox::addItem(QString::number(value));
}

void CustomComboBox::addItem(QString value) {
    QComboBox::addItem(value);
}

void CustomComboBox::eliminarRobotSlot() {
    if(this->count() == 0) return; // No hi ha res a eliminar
    if(this->currentText().isEmpty()) return; // Text buit
    int robotID = this->currentText().toInt();
    removeItem(this->currentIndex());
    emit eliminarRobot(robotID);
}