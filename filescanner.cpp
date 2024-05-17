//
// Created by Olaf Seisler on 18.04.2024.
//

#include <fstream>
#include <algorithm>
#include <filesystem>
#include <QMimeType>
#include <QMimeDatabase>
#include <filesystem>
#include <random>
#include <iostream>

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

        // If the file is not a text file based on MIME type, skip the file
        QString mimeType = QMimeDatabase().mimeTypeForFile(QString::fromStdString(filePath)).name();
        if (!mimeType.contains("text")) {
            continue;
        }

        std::vector<MatchInfo> fileMatches = scanFileForSensitiveData(filePath, patterns);
        if (!fileMatches.empty())
            matches[filePath] = fileMatches;
    }

    return matches;
}


std::vector<MatchInfo>
FileScanner::scanFileForSensitiveData(const std::string &filePath, std::map<std::string, std::string> &patterns) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for scanning.");
    }

    std::vector<MatchInfo> matches;
    char buffer[CHUNK_SIZE];

    while (file) {
        // Read file in chunks
        file.read(buffer, CHUNK_SIZE);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) {
            break;
        }

        std::string chunk(buffer, bytesRead);

        for (const auto &pattern: patterns) {
            std::regex regex(pattern.first);
            std::smatch match;
            auto searchStart = chunk.cbegin();
            while (std::regex_search(searchStart, chunk.cend(), match, regex)) {
                // Sensitive data found
                MatchInfo matchInfo;
                matchInfo.patternUsed = std::make_pair(pattern.first, pattern.second);
                matchInfo.match = match.str();
                matchInfo.startIndex = match.position() + std::distance(chunk.cbegin(), searchStart);
                matchInfo.endIndex = matchInfo.startIndex + match.length();
                matches.push_back(matchInfo);

                if (matches.size() > MAX_NUM_MATCHES) {
                    file.close();
                    return matches;
                }

                searchStart = match.suffix().first;
            }
        }
    }

    file.close();
    return matches;
}

/**
 * Write the file full of random data
 * Pad to the nearest 4KB block size
 * @param filePath The path to the file to scramble
 */
void FileScanner::scrambleFile(const std::string &filePath) {
    std::ofstream file(filePath, std::ios::binary);
    // Handle file write error
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for scrambling.");
    }

    // Get the file size
    size_t fileSize = std::filesystem::file_size(filePath);
    // Calculate the number of blocks
    size_t numBlocks = fileSize / CHUNK_SIZE;
    // Calculate the number of bytes to pad
    long numBytesToPad = CHUNK_SIZE - (fileSize % CHUNK_SIZE);

    // Initialize random generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int8_t> dis(-128, 127);


    for (int i = 0; i < numBlocks; i++) {
        int8_t buffer[CHUNK_SIZE];
        for (signed char & j : buffer) {
            j = dis(gen);
        }
        file.write((char *) buffer, CHUNK_SIZE);
    }

    // Pad the file with random data
    int8_t buffer[numBytesToPad];
    for (signed char &j : buffer) {
        j = dis(gen);
    }
    file.write((char *) buffer, numBytesToPad);

    file.close();
}

void FileScanner::deleteFiles(std::vector<std::string> &filePaths) {
    std::vector<std::string*> filePaths2;
    for (const auto &filePath: filePaths) {
        try {
            scrambleFile(filePath);
            std::filesystem::remove(filePath);

        } catch (std::exception &e) {
            std::cerr << "Failed to scramble file: " << e.what() << std::endl;
        }
    }
}