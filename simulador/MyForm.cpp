#include "MyForm.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>

MyForm::MyForm (QWidget* parent) : QWidget(parent)
{
  ui.setupUi(this);
  
  // Add a "Connect to Backend" button to the UI
  QPushButton* connectButton = new QPushButton("Connect to Backend", this);
  connectButton->setObjectName("backendConnectButton");
  
  // Add it to the vertical layout (if available)
  if (ui.verticalLayout) {
    ui.verticalLayout->addWidget(connectButton);
  }
  
  // Connect the button to our slot
  connect(connectButton, &QPushButton::clicked, this, &MyForm::onConnectBackendClicked);
}

void MyForm::onConnectBackendClicked()
{
  qDebug() << "Connect to Backend clicked";
  
  // Get the OpenGL widget
  if (!ui.widget) {
    QMessageBox::warning(this, "Error", "Simulator widget not found!");
    return;
  }
  
  // Get the button to update its text
  QPushButton* button = findChild<QPushButton*>("backendConnectButton");
  
  // Check if already monitoring
  if (ui.widget->isMonitoringBackend()) {
    // Stop monitoring
    ui.widget->stopBackendMonitoring();
    if (button) {
      button->setText("Connect to Backend");
      button->setStyleSheet("");
    }
    QMessageBox::information(this, "Backend Connection", 
                            "Disconnected from backend telemetry.\n\n"
                            "Click again to reconnect.");
  } else {
    // Start monitoring the telemetry directory
    QString telemetryDir = "../api/orca";  // Adjust path as needed
    ui.widget->startBackendMonitoring(telemetryDir);
    if (button) {
      button->setText("Disconnect Backend");
      button->setStyleSheet("background-color: #90EE90;");
    }
    QMessageBox::information(this, "Backend Connection", 
                            "Connected to backend telemetry!\n\n"
                            "Monitoring: " + telemetryDir + "\n\n"
                            "Tip: If backend restarts, disconnect and reconnect.");
  }
}
