#include "WarehouseLoader.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

WarehouseLoader::WarehouseLoader() : nextRobotID(1), floorSize(30.0f, 30.0f) {
}

WarehouseLoader::~WarehouseLoader() {
}

bool WarehouseLoader::loadFromJSON(const QString& filename) {
    // Open the file
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Error: Could not open warehouse file:" << filename;
        return false;
    }
    
    // Read and parse JSON
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON Parse Error:" << parseError.errorString() 
                   << "at offset" << parseError.offset;
        return false;
    }
    
    if (!doc.isObject()) {
        qWarning() << "Error: JSON root is not an object";
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Clear existing data
    objects.clear();
    robots.clear();
    nextRobotID = 1;
    floorSize = glm::vec2(30.0f, 30.0f); // Default floor size
    
    // Parse floor size
    if (root.contains("floorSize") && root["floorSize"].isArray()) {
        QJsonArray floorArray = root["floorSize"].toArray();
        if (floorArray.size() >= 2) {
            floorSize.x = floorArray[0].toDouble(30.0);
            floorSize.y = floorArray[1].toDouble(30.0);
            qDebug() << "Floor size:" << floorSize.x << "x" << floorSize.y;
        }
    }
    
    // Parse objects array
    if (root.contains("objects") && root["objects"].isArray()) {
        QJsonArray objectsArray = root["objects"].toArray();
        
        for (const QJsonValue& objValue : objectsArray) {
            if (!objValue.isObject()) continue;
            
            QJsonObject obj = objValue.toObject();
            WarehouseObject wObj;
            
            // Parse modelIndex
            if (obj.contains("modelIndex")) {
                wObj.modelIndex = obj["modelIndex"].toInt(0);
            }
            
            // Parse center position
            if (obj.contains("center") && obj["center"].isArray()) {
                QJsonArray centerArray = obj["center"].toArray();
                if (centerArray.size() >= 3) {
                    wObj.center.x = centerArray[0].toDouble(0.0);
                    wObj.center.y = centerArray[1].toDouble(0.0);
                    wObj.center.z = centerArray[2].toDouble(0.0);
                }
            }
            
            // Parse dimensions
            if (obj.contains("dimensions") && obj["dimensions"].isArray()) {
                QJsonArray dimArray = obj["dimensions"].toArray();
                if (dimArray.size() >= 3) {
                    wObj.dimensions.x = dimArray[0].toDouble(1.0);
                    wObj.dimensions.y = dimArray[1].toDouble(1.0);
                    wObj.dimensions.z = dimArray[2].toDouble(1.0);
                }
            }
            
            // Parse rotation
            if (obj.contains("rotation")) {
                wObj.rotation = obj["rotation"].toDouble(0.0);
            }
            
            objects.push_back(wObj);
        }
        
        qDebug() << "Loaded" << objects.size() << "warehouse objects";
    }
    
    // Parse robots array
    if (root.contains("robots") && root["robots"].isArray()) {
        QJsonArray robotsArray = root["robots"].toArray();
        
        for (const QJsonValue& robotValue : robotsArray) {
            if (!robotValue.isObject()) continue;
            
            QJsonObject robot = robotValue.toObject();
            
            int id = robot["id"].toInt(-1);
            float x = robot["x"].toDouble(0.0);
            float y = robot["y"].toDouble(0.0);
            float angle = robot["angle"].toDouble(0.0);
            bool hasBox = robot["hasBox"].toBool(false);  // Load hasBox state, default to false
            
            if (id != -1) {
                robots[id] = std::make_tuple(x, y, angle, hasBox);
                if (id >= nextRobotID) {
                    nextRobotID = id + 1;
                }
            }
        }
        
        qDebug() << "Loaded" << robots.size() << "robots";
    }
    
    qDebug() << "Successfully loaded warehouse from" << filename;
    return true;
}

void WarehouseLoader::clear() {
    objects.clear();
    robots.clear();
    nextRobotID = 1;
    floorSize = glm::vec2(30.0f, 30.0f);
}
