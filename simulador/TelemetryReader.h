#ifndef TELEMETRY_READER_H
#define TELEMETRY_READER_H

#include <QObject>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <map>
#include <tuple>

/**
 * @brief Reads robot telemetry from backend JSON files
 * 
 * This class monitors the backend's output directory and reads
 * robot position/state data from orca_tick_*.json files
 */
class TelemetryReader : public QObject
{
    Q_OBJECT

public:
    struct RobotData {
        int id;
        float x;
        float y;
        float vx;
        float vy;
        QString status;
        QString driverState;
        float battery;
        bool hasPackage;
    };

    explicit TelemetryReader(QObject *parent = nullptr);
    ~TelemetryReader();

    // Start/stop monitoring
    void startMonitoring(const QString& telemetryDir);
    void stopMonitoring();
    
    // Configure update rate
    void setUpdateInterval(int ms); // Default: 50ms (20Hz)
    
    // Get current robot data
    std::map<int, RobotData> getCurrentRobots() const;

signals:
    void robotsUpdated(const std::map<int, RobotData>& robots);
    void errorOccurred(const QString& error);

private slots:
    void updateFromLatestFile();
    void onDirectoryChanged(const QString& path);

private:
    bool parseOrcaFile(const QString& filepath);
    QString findLatestOrcaFile();

    QTimer* updateTimer_;
    QFileSystemWatcher* fileWatcher_;
    QString telemetryDir_;
    std::map<int, RobotData> currentRobots_;
    int lastTickRead_;
    bool isMonitoring_;
};

#endif // TELEMETRY_READER_H
