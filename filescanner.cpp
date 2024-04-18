//
// Created by Olaf Seisler on 18.04.2024.
//

#include <fstream>
#include <algorithm>
#include <filesystem>

#include "filescanner.h"

#define CHUNK_SIZE 4096
#define MAX_NUM_MATCHES 100

FileScanner::FileScanner() {
    // Constructor
}

FileScanner::~FileScanner() {
    // Destructor
}

std::map<std::string, std::vector<MatchInfo>> FileScanner::scanFiles(const std::vector<std::string> &filePaths,
                                                                     std::map<std::string, std::string> &patterns,
                                                                     std::map<std::string, std::string> &fileTypes) {
    // Scan files based on given patterns and file types
    std::map<std::string, std::vector<MatchInfo>> matches;

    for (const auto &filePath: filePaths) {
        // If the file type is not in the list of file types, skip the file
        for (const auto &item: fileTypes) {
            if (filePath.find(item.first) != std::string::npos) {
                break;
            }
        }

        std::vector<MatchInfo> fileMatches = scanFileForSensitiveData(filePath, patterns);
        if (!fileMatches.empty())
            matches[filePath] = fileMatches;
    }

    return matches;
}

std::vector<MatchInfo>
FileScanner::scanFileForSensitiveData(const std::string &filePath, std::map<std::string, std::string> &patterns) {
    // Scan a single file with regex for sensitive data
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for scanning.");
    }

    std::vector<MatchInfo> matches;

    // Read file in chunks
    char buffer[CHUNK_SIZE];
    memset(buffer, NULL, CHUNK_SIZE);
    // Calculate num bytes to read so that we don't read more than in the file
    long numBytesToRead = std::min((long) CHUNK_SIZE, (long) std::filesystem::file_size(filePath));
    while (file.read(buffer, numBytesToRead)) {
        if (matches.size() > MAX_NUM_MATCHES) {
            break;
        }
        std::string chunk(buffer);
        for (const auto &pattern: patterns) {
            std::regex regex(pattern.first);
            std::smatch match;
            if (std::regex_search(chunk, match, regex)) {
                // Sensitive data found
                MatchInfo matchInfo;
                matchInfo.patternUsed = std::make_pair(pattern.first, pattern.second);
                matchInfo.match = match.str();
                matchInfo.startIndex = match.position();
                matchInfo.endIndex = match.position() + match.length();
                matches.push_back(matchInfo);
            }
        }
        memset(buffer, NULL, CHUNK_SIZE);
    }

    file.close();
    return matches;
}

void FileScanner::deleteFiles(std::vector<std::string> &filePaths) {
    // TODO: Fuzz the file content before deletion

}