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
    void addNewFileType(QString &fileType, QString &description);
    void removeFileType(QString &fileType);
    void addNewScanPattern(QString &scanPattern, QString &description);
    void removeScanPattern(QString &scanPattern);
    QMap<QString, QString> getFileTypes();
    QMap<QString, QString> getScanPatterns();
    void updateConfigFile();

private:
    QString configFilePath;
    QMap<QString, QString> fileTypes;
    QMap<QString, QString> scanPatterns;

};


#endif //SENSITIVE_DATA_DELETER_CONFIGMANAGER_H
