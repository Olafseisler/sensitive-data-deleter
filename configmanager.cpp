//
// Created by Olaf Seisler on 03.03.2024.
//

#include <QFile>
#include <QDebug>
#include <QString>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "configmanager.h"

ConfigManager::ConfigManager() {
    // Read configpath.txt to determine the config location
    QFile file("configpath.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Could not open configpath.txt. Cannot find scan config path file.");
    }

    QTextStream in(&file);
    QString candidatePath = in.readLine();
    file.close();

    if (candidatePath.isEmpty()) {
        throw std::runtime_error("Configpath.txt is empty");
    }

    QList<QString> parts = candidatePath.split("=");
    if (parts.size() != 2) {
        throw std::runtime_error("Configpath.txt is not formatted correctly");
    }
    if (parts[0] != "CONFIG_FILE_PATH") {
        throw std::runtime_error("Configpath.txt is not formatted correctly");
    }
    if (parts[1].isEmpty()) {
        throw std::runtime_error("The config file path is empty. Please specify a path in configpath.txt.");
    }
    if (!parts[1].endsWith(".json")) {
        throw std::runtime_error("The config file must be a .json file.");
    }

    configFilePath = parts[1];
    loadConfigFromFile(configFilePath);
}

ConfigManager::~ConfigManager() {
    updateConfigFile();
}

void ConfigManager::loadConfigFromFile(QString &path) {
    // Open the .json config and read it into memory
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Could not open config file for reading.");
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(file.readAll());
    if (jsonDocument.isNull()) {
        throw std::runtime_error("The config file is not a valid .json file.");
    }

    // Parse the .json file
    QJsonObject rootObj = jsonDocument.object();
    QJsonValue fileTypes = rootObj["fileTypes"];
    QJsonValue scanPatterns = rootObj["scanPatterns"];

    if (!fileTypes.isArray() || !scanPatterns.isArray()) {
        throw std::runtime_error("The config file is not formatted correctly.");
    }

    QJsonArray fileTypesArray = fileTypes.toArray();
    for (auto && i : fileTypesArray) {
        QJsonValue value = i;
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString fileType = obj["fileType"].toString();
            QString description = obj["description"].toString();
            this->fileTypes.insert(fileType, description);
        }
    }

    QJsonArray scanPatternsArray = scanPatterns.toArray();
    for (auto && i : scanPatternsArray) {
        QJsonValue value = i;
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString scanPattern = obj["pattern"].toString();
            QString description = obj["description"].toString();
            this->scanPatterns.insert(scanPattern, description);
        }
    }
}

void ConfigManager::addNewFileType(QString &fileType, QString &description) {
    fileTypes.insert(fileType, fileType);
    updateConfigFile();
}

void ConfigManager::removeFileType(QString &fileType) {
    fileTypes.remove(fileType);
    updateConfigFile();
}

void ConfigManager::addNewScanPattern(QString &scanPattern, QString &description) {
    scanPatterns.insert(scanPattern, description);
    updateConfigFile();
}

void ConfigManager::removeScanPattern(QString &scanPattern) {
    scanPatterns.remove(scanPattern);
    updateConfigFile();
}

void ConfigManager::updateConfigFile() {
    QJsonObject obj;
    QJsonArray fileTypesArray;
    QJsonArray scanPatternsArray;

    for (const auto & fileType : fileTypes.keys()) {
        QJsonObject fileTypeObj;
        fileTypeObj["fileType"] = fileType;
        fileTypeObj["description"] = fileTypes[fileType];
        fileTypesArray.append(fileTypeObj);
    }
    for (const auto & scanPattern : scanPatterns.keys()) {
        QJsonObject scanPatternObj;
        scanPatternObj["pattern"] = scanPattern;
        scanPatternObj["description"] = scanPatterns[scanPattern];
        scanPatternsArray.append(scanPatternObj);
    }


    obj["fileTypes"] = fileTypesArray;
    obj["scanPatterns"] = scanPatternsArray;

    QJsonDocument doc(obj);
    QFile file(configFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error("Could not open config file for writing.");
    }

    QTextStream out(&file);
    out << doc.toJson();
    file.close();
}

QMap<QString, QString> ConfigManager::getFileTypes() {
    return fileTypes;
}

QMap<QString, QString> ConfigManager::getScanPatterns() {
    return scanPatterns;
}





