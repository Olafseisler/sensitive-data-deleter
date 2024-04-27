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
    std::map<std::string, std::vector<MatchInfo>> scanFiles(const std::vector<std::string> &filePaths,
                                                                std::map<std::string, std::string> &patterns,
                                                                std::map<std::string, std::string> &fileTypes);
    std::vector<MatchInfo> scanFileForSensitiveData(const std::string &filePath, std::map<std::string, std::string> &patterns);
    void deleteFiles(std::vector<std::string> &filePaths);
    void scrambleFile(const std::string &filePath);

};


#endif //SENSITIVE_DATA_DELETER_FILESCANNER_H
