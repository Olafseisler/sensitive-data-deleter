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
    FLAGGED_BUT_IMMUTABLE,
    UNSUPPORTED_TYPE,
    UNREADABLE,
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
    FileScanner();

    ~FileScanner();

    static void scanFiles(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                          const std::vector<std::string> &filePaths,
                          const std::map<std::string, std::string> &patterns,
                          const std::map<std::string, std::string> &fileTypes);

    static std::pair<ScanResult, std::vector<MatchInfo>>
    scanFileForSensitiveData(const std::filesystem::path &filePath, const std::map<std::string, std::string> &patterns);

    static std::pair<ScanResult, std::vector<MatchInfo>> scanChunkWithRegex(const std::string &chunk,
                       const std::map<std::string,
                               std::string> &patterns);

    void deleteFiles(std::vector<std::string> &filePaths);

    void scrambleFile(const std::string &filePath);
};


#endif //SENSITIVE_DATA_DELETER_FILESCANNER_H
