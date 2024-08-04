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
#include <QDialog>
#include <regex>

#include "configmanager.h"

void showProblemDialog(const QString &title, const QString &message) {
    qWarning() << message;
    QDialog dialog;
    dialog.setWindowTitle(title);
    dialog.exec();
}

ConfigManager::ConfigManager() {
    // Read configpath.txt to determine the config location
    QFile file("configpath.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showProblemDialog("Error: Could not open configpath.txt.", "Cannot find or read scan config path file."
                                                                   " Please make sure it exists in the executable directory.");
        return;
    }

    QTextStream in(&file);
    QString candidatePath = in.readLine();
    file.close();

    if (candidatePath.isEmpty()) {
        showProblemDialog("Error: configpath.txt is empty.",
                          "Please specify a valid configuration file path in configpath.txt.");
        return;
    }

    QList<QString> parts = candidatePath.split("=");
    if (parts.size() != 2) {
        showProblemDialog("Error: configpath.txt is not formatted correctly.",
                          "Please format the file as 'CONFIG_FILE_PATH=<path>'");
        return;
    }
    if (parts[0] != "CONFIG_FILE_PATH") {
        showProblemDialog("Error: configpath.txt is not formatted correctly.",
                          "Please format the file as 'CONFIG_FILE_PATH=<path>'");
        return;
    }
    if (parts[1].isEmpty()) {
        showProblemDialog("Error: configpath.txt is empty.",
                          "Please specify a valid configuration file path in configpath.txt.");
        return;
    }
    if (!parts[1].endsWith(".json")) {
        showProblemDialog("Error: The config file must be a .json file.",
                          "Please specify a valid configuration file path in configpath.txt.");
        return;
    }

    configFilePath = parts[1];
    loadConfigFromFile(configFilePath);
}

ConfigManager::~ConfigManager() = default;

void ConfigManager::loadConfigFromFile(QString &path) {
    this->scanPatterns.clear();
    this->fileTypes.clear();
    // Open the .json config and read it into memory
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showProblemDialog("Error: Could not open config file for reading.",
                          "Cannot open the scan config file for reading.");
        return;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(file.readAll());
    if (jsonDocument.isNull()) {
        showProblemDialog("Error: The config file is not a valid .json file.",
                          "The scan config file is not a properly formatted JSON.");
        return;
    }

    // Parse the .json file
    QJsonObject rootObj = jsonDocument.object();
    QJsonValue newFileTypes = rootObj["fileTypes"];
    QJsonValue newScanPatterns = rootObj["scanPatterns"];

    if (newFileTypes.isNull() || newScanPatterns.isNull() || !newFileTypes.isArray() || !newScanPatterns.isArray()) {
        showProblemDialog("Error: The config file is not formatted correctly.",
                          "The scan config file is not in expected format.");
        return;
    }

    QJsonArray fileTypesArray = newFileTypes.toArray();
    bool fileTypesError = false;
    bool scanPatternsError = false;
    bool regexError = false;

    for (auto &&i: fileTypesArray) {
        QJsonValue value = i;
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString fileType = obj["fileType"].toString();
            QString description = obj["description"].toString();
            if (fileType.isNull() || description.isNull() || fileType.isEmpty() || description.isEmpty()) {
                showProblemDialog("Warning: Could not load all file types and scan patterns.",
                                  "Some file types could not be loaded.");
                fileTypesError = true;
                continue;
            }
            this->fileTypes.append(qMakePair(fileType, description));
        }
    }

    QJsonArray scanPatternsArray = newScanPatterns.toArray();
    for (auto &&i: scanPatternsArray) {
        QJsonValue value = i;
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString scanPattern = obj["pattern"].toString();
            QString description = obj["description"].toString();
            if (scanPattern.isNull() || description.isNull() ||
                scanPattern.isEmpty() || description.isEmpty()) {
                showProblemDialog("Warning: Could not load all file types and scan patterns.",
                                  "Some scan patterns could not be loaded.");
                scanPatternsError = true;
                continue;
            }

            if (!isValidRegex(scanPattern)) {
                qDebug() << "The scan pattern is not a valid regex expression.";
                regexError = true;
                continue;
            }
            this->scanPatterns.append(qMakePair(scanPattern, description));
        }
    }

    QString errorMessage = "";
    if (fileTypesError) {
        errorMessage += "Some file types could not be loaded.\n";
    }
    if (scanPatternsError) {
        errorMessage += "Some scan patterns could not be loaded.\n";
    }
    if (regexError) {
        errorMessage += "Some scan patterns are not valid regex expressions.\n";
    }
    if (!errorMessage.isEmpty()) {
        showProblemDialog("Error: Could not load all file types and scan patterns.", errorMessage);
    }
}

void ConfigManager::editFileType(int index, QString &fileType, QString &description) {
    if (index == fileTypes.size()) {
        fileTypes.append(qMakePair(fileType, description));
    } else {
        fileTypes[index] = qMakePair(fileType, description);
    }
    updateConfigFile();
}

void ConfigManager::removeFileType(QString &fileType) {
    for (size_t i = 0; i < fileTypes.size(); i++) {
        if (fileTypes[i].first == fileType) {
            fileTypes.removeAt(i);
            break;
        }
    }
    updateConfigFile();
}

void ConfigManager::addNewScanPattern(QString &scanPattern, QString &description) {
    scanPatterns.append(qMakePair(scanPattern, description));
    updateConfigFile();
}

void ConfigManager::removeScanPattern(QString &scanPattern) {
    for (size_t i = 0; i < scanPatterns.size(); i++) {
        if (scanPatterns[i].first == scanPattern) {
            scanPatterns.removeAt(i);
            break;
        }
    }

    updateConfigFile();
}

void ConfigManager::updateConfigFile() {
    QJsonObject obj;
    QJsonArray fileTypesArray;
    QJsonArray scanPatternsArray;

    for (const auto &fileType : fileTypes) {
        QJsonObject fileTypeObj;
        fileTypeObj["fileType"] = fileType.first;
        fileTypeObj["description"] = fileType.second;
        fileTypesArray.append(fileTypeObj);
    }
    for (const auto &scanPattern : scanPatterns) {
        QJsonObject scanPatternObj;
        scanPatternObj["pattern"] = scanPattern.first;
        scanPatternObj["description"] = scanPattern.second;
        scanPatternsArray.append(scanPatternObj);
    }

    obj["fileTypes"] = fileTypesArray;
    obj["scanPatterns"] = scanPatternsArray;

    QJsonDocument doc(obj);
    QFile file(configFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        showProblemDialog("Error: Could not open config file for writing.",
                          "Cannot open the scan config file for writing.");
        return;
    }

    QTextStream out(&file);
    out << doc.toJson();

    file.close();
}

void ConfigManager::editScanPattern(int index, QString &scanPattern, QString &description) {
    if (!isValidRegex(scanPattern)) {
        qDebug() << "The scan pattern is not a valid regex expression.";
        return;
    }

    if (index == scanPatterns.size()) {
        scanPatterns.append(qMakePair(scanPattern, description));
    } else {
        scanPatterns[index] = qMakePair(scanPattern, description);
    }
    updateConfigFile();
}

QList<QPair<QString, QString>> ConfigManager::getFileTypes() {
    return fileTypes;
}

QList<QPair<QString, QString>> ConfigManager::getScanPatterns() {
    return scanPatterns;
}

// Check if a string represents a valid c++ regex expression
bool ConfigManager::isValidRegex(QString pattern) {
    try {
        std::regex re(pattern.toStdString());
    } catch (std::regex_error &e) {
        return false;
    }
    return true;
}

void ConfigManager::setConfigFilePath(QString &path) {
    configFilePath = path;
    // Update the configpath.txt file
    QFile file("configpath.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        showProblemDialog("Error: Could not open configpath.txt for writing.",
                          "Cannot open the configpath.txt file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "CONFIG_FILE_PATH=" << path;
    file.close();
}



