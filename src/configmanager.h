//
// Created by Olaf Seisler on 03.03.2024.
//

#include <QList>
#include <QString>

#ifndef SENSITIVE_DATA_DELETER_CONFIGMANAGER_H
#define SENSITIVE_DATA_DELETER_CONFIGMANAGER_H


class ConfigManager {

public:
    ConfigManager();
    ~ConfigManager();
    void loadConfigFromFile(QString &path);
    void editFileType(int index, QString &fileType, QString &description);
    void removeFileType(QString &fileType);
    void addNewScanPattern(QString &scanPattern, QString &description);
    void removeScanPattern(QString &scanPattern);
    void editScanPattern(int index, QString &scanPattern, QString &description);
    bool isValidRegex(QString pattern);
    QList<QPair<QString, QString>> getConfigList();
    QList<QPair<QString, QString>> getFileTypes();
    QList<QPair<QString, QString>> getScanPatterns();
    void updateConfigFile();
    void setConfigFilePath(QString &path);

private:
    QString configFilePath;
    QList<QPair<QString, QString>> fileTypes;
    QList<QPair<QString, QString>> scanPatterns;
    QList<QString> immutableTypes = {".txt"};
};


#endif //SENSITIVE_DATA_DELETER_CONFIGMANAGER_H
