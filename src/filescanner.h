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
#include <filesystem>

enum ScanResult {
    CLEAN,
    FLAGGED,
    UNSUPPORTED_TYPE,
    UNREADABLE,
    FLAGGED_BUT_UNWRITABLE,
    DIRECTORY_TOO_DEEP,
    UNDEFINED
};

struct MatchInfo {
    std::pair<std::string, std::string> patternUsed;
    std::string match;
    size_t startIndex;
    size_t endIndex;
};

class FileScanner {
public:
//    FileScanner() = default;
//    ~FileScanner()= default;

    void scanFiles(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                          const std::vector<std::string> &filePaths,
                          const std::map<std::string, std::string> &patterns,
                          const std::map<std::string, std::string> &fileTypes);

    std::pair<ScanResult, std::vector<MatchInfo>>
    scanFileForSensitiveData(const std::filesystem::path &filePath, const std::map<std::string, std::string> &patterns);
    void scanChunkWithRegex(const std::string &chunk, const std::map<std::string, std::string> &patterns,
                       std::pair<ScanResult, std::vector<MatchInfo>> &returnPair);
    void deleteFiles(std::vector<std::string> &filePaths);

    void scrambleFile(const std::string &filePath);
};


#endif //SENSITIVE_DATA_DELETER_FILESCANNER_H
