//
// Created by Olaf Seisler on 18.04.2024.
//

#include <fstream>
#include <algorithm>
#include <filesystem>
#include <QMimeType>
#include <QMimeDatabase>
#include <QFile>
#include <QFileInfo>
#include <random>
#include <iostream>
#include "poppler-document.h"
#include "poppler-page.h"
#include <hs/hs.h>

#include "filescanner.h"
#include "chunkreader.h"

#define CHUNK_SIZE 4096
#define MAX_NUM_MATCHES 100

FileScanner::FileScanner() {

}

FileScanner::~FileScanner() {
    // Destructor
}


void
FileScanner::scanFiles(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                            const std::vector<std::string>& filePaths,
                            const std::map<std::string, std::string>& patterns,
                            const std::map<std::string, std::string>& fileTypes) {
    // Scan files based on given patterns and file types
    int i = 0;
    std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>> matches;

    for (const auto &filePath: filePaths) {
        // If the file type is not in the list of file types, skip the file
        bool foundInFileTypes = false;
        for (const auto &item: fileTypes) {
            if (filePath.find(item.first) != std::string::npos) {
                foundInFileTypes = true;
                break;
            }
        }
        if (!foundInFileTypes) {
            matches[filePath] = std::make_pair(ScanResult::UNSUPPORTED_TYPE, std::vector<MatchInfo>());
            continue;
        }

        promise.setProgressValue(static_cast<int>((100 * i) / filePaths.size()));

        // If the file is not a text file based on MIME type, skip the file
        QMimeType mimeType = QMimeDatabase().mimeTypeForFile(QString::fromStdString(filePath));
        if (!mimeType.inherits("text/plain") &&
            !mimeType.inherits("application/pdf")) {
            matches[filePath] = std::make_pair(ScanResult::UNSUPPORTED_TYPE, std::vector<MatchInfo>());
            continue;
        }

        std::pair<ScanResult, std::vector<MatchInfo>> fileMatches = scanFileForSensitiveData(filePath, patterns);
        matches[filePath] = fileMatches;

        ++i;
    }
    promise.addResult(matches);
    promise.setProgressValue(100);
    promise.finish();
}

std::pair<ScanResult, std::vector<MatchInfo>>
FileScanner::scanFileForSensitiveData(const std::filesystem::path &filePath, const std::map<std::string, std::string> &patterns) {
    // Check if the file is readable and writeable with Qt
    QFileInfo fileInfo(QString::fromStdString(filePath.generic_string()));
    if (!fileInfo.isReadable()) {
        return std::make_pair(ScanResult::UNREADABLE, std::vector<MatchInfo>());
    }

    auto returnPair = std::make_pair(ScanResult::CLEAN, std::vector<MatchInfo>());
    char buffer[CHUNK_SIZE];
    ChunkReader *chunkReader;

    try{
        chunkReader = ChunkReaderFactory::createReader(filePath);

    } catch (std::exception &e) {
        return std::make_pair(ScanResult::UNSUPPORTED_TYPE, std::vector<MatchInfo>());
    }

    while (true) {
        // Read a chunk from file
        size_t numBytesRead = chunkReader->readChunkFromFile(buffer, CHUNK_SIZE);
        if (numBytesRead == 0) {
            break;
        }

        // Scan the given chunk with regex
        std::string chunk(buffer, numBytesRead);
        auto chunkMatches = scanChunkWithRegex(chunk, patterns);
        if (chunkMatches.first == ScanResult::FLAGGED) {
            returnPair.first = ScanResult::FLAGGED;
        }
        returnPair.second.insert(returnPair.second.end(), chunkMatches.second.begin(), chunkMatches.second.end());

        if (returnPair.second.size() > MAX_NUM_MATCHES) {
            // Trim the matches list down to MAX_NUM_MATCHES
            returnPair.second.resize(MAX_NUM_MATCHES);
            return returnPair;
        }
    }

    if (returnPair.first == ScanResult::FLAGGED && !fileInfo.isWritable()) {
        returnPair.first = ScanResult::FLAGGED_BUT_IMMUTABLE;
    }
    delete chunkReader;

    return returnPair;
}

std::pair<ScanResult, std::vector<MatchInfo>> FileScanner::scanChunkWithRegex(const std::string &chunk,
                                                                              const std::map<std::string,
                                                                              std::string> &patterns) {
    auto returnPair = std::make_pair(ScanResult::CLEAN, std::vector<MatchInfo>());
    for (const auto &pattern: patterns) {
        std::regex regex(pattern.first);
        std::smatch match;
        auto searchStart = chunk.cbegin();
        while (std::regex_search(searchStart, chunk.cend(), match, regex)) {
            // Sensitive data found
            returnPair.first = ScanResult::FLAGGED;
            MatchInfo matchInfo;
            matchInfo.patternUsed = std::make_pair(pattern.first, pattern.second);
            matchInfo.match = match.str();
            matchInfo.startIndex = match.position() + std::distance(chunk.cbegin(), searchStart);
            matchInfo.endIndex = matchInfo.startIndex + match.length();
            returnPair.second.push_back(matchInfo);

            if (returnPair.second.size() >= MAX_NUM_MATCHES) {
                return returnPair;
            }

            searchStart = match.suffix().first;
        }
    }
    return returnPair;
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
    std::uniform_int_distribution<int16_t> dis(-128, 127);


    for (int i = 0; i < numBlocks; i++) {
        int16_t buffer[CHUNK_SIZE];
        for (int16_t &j: buffer) {
            j = dis(gen);
        }
        file.write((char *) buffer, CHUNK_SIZE);
    }

    // Pad the file with random data
    auto *buffer = new int16_t[numBytesToPad];
    for (int i = 0; i < numBytesToPad; i++) {
        buffer[i] = dis(gen);
    }
    delete[] buffer;

    file.write((char *) buffer, numBytesToPad);
    file.close();
}

void FileScanner::deleteFiles(std::vector<std::string> &filePaths) {
    std::vector<std::string *> filePaths2;
    for (const auto &filePath: filePaths) {
        try {
            scrambleFile(filePath);
            std::filesystem::remove(filePath);

        } catch (std::exception &e) {
            std::cerr << "Failed to scramble file: " << e.what() << std::endl;
        }
    }
}