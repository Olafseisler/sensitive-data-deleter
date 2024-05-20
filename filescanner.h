//
// Created by Olaf Seisler on 18.04.2024.
//

#ifndef SENSITIVE_DATA_DELETER_FILESCANNER_H
#define SENSITIVE_DATA_DELETER_FILESCANNER_H

#include <vector>
#include <string>
#include <regex>
#include <map>
#include <utility>
#include <QPromise>

struct MatchInfo {
    std::pair<std::string, std::string> patternUsed;
    std::string match;
    size_t startIndex;
    size_t endIndex;
};

class FileScanner {
public:
    FileScanner();
    ~FileScanner();

    static void scanFiles(QPromise<std::map<std::string, std::vector<MatchInfo>>> &promise,
                  const std::vector<std::string>& filePaths,
                  const std::map<std::string, std::string>& patterns,
                  const std::map<std::string, std::string>& fileTypes);

    static std::vector<MatchInfo>
    scanFileForSensitiveData(const std::string &filePath, const std::map<std::string, std::string> &patterns);

    void deleteFiles(std::vector<std::string> &filePaths);

    void scrambleFile(const std::string &filePath);

};


#endif //SENSITIVE_DATA_DELETER_FILESCANNER_H
