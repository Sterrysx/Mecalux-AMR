#include "TelemetryReader.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonParseError>
#include <algorithm>

TelemetryReader::TelemetryReader(QObject *parent)
    : QObject(parent)
    , updateTimer_(new QTimer(this))
    , fileWatcher_(new QFileSystemWatcher(this))
    , lastTickRead_(-1)
    , isMonitoring_(false)
{
    // Connect timer to update function
    connect(updateTimer_, &QTimer::timeout, this, &TelemetryReader::updateFromLatestFile);
    
    // Connect file system watcher
    connect(fileWatcher_, &QFileSystemWatcher::directoryChanged, 
            this, &TelemetryReader::onDirectoryChanged);
    
    // Default: 50ms update interval (20Hz)
    updateTimer_->setInterval(50);
}

TelemetryReader::~TelemetryReader()
{
    stopMonitoring();
}

void TelemetryReader::startMonitoring(const QString& telemetryDir)
{
    telemetryDir_ = telemetryDir;
    
    // Check if directory exists
    QDir dir(telemetryDir_);
    if (!dir.exists()) {
        emit errorOccurred("Telemetry directory does not exist: " + telemetryDir_);
        return;
    }
    
    // Reset tick counter to allow reading from a restarted backend
    lastTickRead_ = -1;
    
    // Start watching the directory
    if (!fileWatcher_->directories().contains(telemetryDir_)) {
        fileWatcher_->addPath(telemetryDir_);
    }
    
    // Start the update timer
    updateTimer_->start();
    isMonitoring_ = true;
    
    qDebug() << "Started monitoring telemetry directory:" << telemetryDir_;
}

void TelemetryReader::stopMonitoring()
{
    updateTimer_->stop();
    
    if (!fileWatcher_->directories().isEmpty()) {
        fileWatcher_->removePaths(fileWatcher_->directories());
    }
    
    isMonitoring_ = false;
    qDebug() << "Stopped monitoring telemetry";
}

void TelemetryReader::setUpdateInterval(int ms)
{
    updateTimer_->setInterval(ms);
}

std::map<int, TelemetryReader::RobotData> TelemetryReader::getCurrentRobots() const
{
    return currentRobots_;
}

void TelemetryReader::onDirectoryChanged(const QString& path)
{
    // When directory changes, try to read the latest file immediately
    updateFromLatestFile();
}

QString TelemetryReader::findLatestOrcaFile()
{
    QDir dir(telemetryDir_);
    
    // Get all orca_tick_*.json files
    QStringList filters;
    filters << "orca_tick_*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    
    if (files.isEmpty()) {
        return QString();
    }
    
    // Find the file with the highest tick number
    int maxTick = -1;
    QString latestFile;
    
    for (const QFileInfo& fileInfo : files) {
        QString filename = fileInfo.fileName();
        // Extract tick number: orca_tick_123.json -> 123
        QString tickStr = filename.mid(10); // Skip "orca_tick_"
        tickStr = tickStr.left(tickStr.length() - 5); // Remove ".json"
        
        bool ok;
        int tick = tickStr.toInt(&ok);
        if (ok && tick > maxTick) {
            maxTick = tick;
            latestFile = fileInfo.absoluteFilePath();
        }
    }
    
    // Only return if it's a new tick
    if (maxTick > lastTickRead_) {
        lastTickRead_ = maxTick;
        return latestFile;
    }
    
    // If no files or all files are old, return empty
    return QString();
}

void TelemetryReader::updateFromLatestFile()
{
    if (!isMonitoring_) return;
    
    QString latestFile = findLatestOrcaFile();
    if (latestFile.isEmpty()) {
        // No new data available, but keep checking
        return;
    }
    
    if (parseOrcaFile(latestFile)) {
        emit robotsUpdated(currentRobots_);
    }
}

bool TelemetryReader::parseOrcaFile(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred("Failed to open file: " + filepath);
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        emit errorOccurred("JSON parse error: " + parseError.errorString());
        return false;
    }
    
    if (!doc.isObject()) {
        emit errorOccurred("Invalid JSON format: root is not an object");
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Read tick number (for debugging)
    int tick = root["tick"].toInt(-1);
    
    // Read robots array
    QJsonArray robotsArray = root["robots"].toArray();
    
    // Clear current robots and rebuild from file
    currentRobots_.clear();
    
    for (const QJsonValue& value : robotsArray) {
        if (!value.isObject()) continue;
        
        QJsonObject robotObj = value.toObject();
        
        RobotData robot;
        robot.id = robotObj["id"].toInt();
        robot.x = robotObj["x"].toDouble();
        robot.y = robotObj["y"].toDouble();
        robot.vx = robotObj["vx"].toDouble();
        robot.vy = robotObj["vy"].toDouble();
        robot.status = robotObj["status"].toString();
        robot.driverState = robotObj["driverState"].toString();
        robot.battery = robotObj["battery"].toDouble();
        robot.hasPackage = robotObj["hasPackage"].toBool();
        
        currentRobots_[robot.id] = robot;
    }
    
    //qDebug() << "Parsed tick" << tick << "with" << currentRobots_.size() << "robots";
    return true;
}
