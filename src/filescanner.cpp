//
// Created by Olaf Seisler on 18.04.2024.
//

#include <fstream>
#include <algorithm>
#include <filesystem>
#include <QMimeType>
#include <QMimeDatabase>
#include <random>
#include <iostream>
#include <QFileInfo>

#include "filescanner.h"
#include "chunkreader.h"

#define CHUNK_SIZE 64 * 1024
#define MAX_NUM_MATCHES 100


void
FileScanner::scanFiles(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                       const std::vector<std::string> &filePaths,
                       const std::map<std::string, std::string> &patterns,
                       const std::map<std::string, std::string> &fileTypes) {


    for (const auto &item: filePaths) {
        file_queue.push(std::filesystem::path(item));
    }


    // Scan files based on given patterns and file types
    std::atomic<size_t> filesProcessed = 0;
    std::vector<std::thread> threads;
    uint numThreads = std::thread::hardware_concurrency();

    for (uint j = 0; j < numThreads; j++) {
        threads.emplace_back(&FileScanner::scannerWorker, this, std::ref(patterns), std::ref(fileTypes),
                             std::ref(promise), std::ref(filesProcessed), filePaths.size());
    }

    file_queue.set_done();

    for (auto &thread: threads) {
        thread.join();
    }

    promise.addResult(matches);

    // Empty the matches list
    matches.clear();
    promise.finish();
}

void FileScanner::scannerWorker(const std::map<std::string, std::string> &patterns,
                                const std::map<std::string, std::string> &fileTypes,
                                QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                                std::atomic<size_t> &filesProcessed,
                                size_t totalFiles) {
    std::filesystem::path filePath;
    while (file_queue.pop(filePath)) {
        auto result = scanFileForSensitiveData(filePath, fileTypes, patterns);
        {
            std::lock_guard<std::mutex> lock(matches_mutex);
            matches[filePath.string()] = result;
        }
        size_t processed = ++filesProcessed;
        promise.setProgressValue(static_cast<int>((processed * 100) / totalFiles));
    }
}

std::pair<ScanResult, std::vector<MatchInfo>>
FileScanner::scanFileForSensitiveData(const std::filesystem::path &filePath,
                                      const std::map<std::string, std::string> &fileTypes,
                                      const std::map<std::string, std::string> &patterns) {

    // Check if the file extension exists in the file types map
    if (fileTypes.find(filePath.extension()) == fileTypes.end()) {
        return std::make_pair(ScanResult::UNSUPPORTED_TYPE, std::vector<MatchInfo>());
    }
    // If the file is not a text file based on MIME type, skip the file
    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(QString::fromStdString(filePath));
    if (!mimeType.inherits("text/plain") && !mimeType.inherits("application/pdf")) {
        return std::make_pair(ScanResult::UNSUPPORTED_TYPE, std::vector<MatchInfo>());
    }
    QFileInfo fileInfo(QString::fromStdString(filePath));
    if (!fileInfo.isReadable()) {
        return std::make_pair(ScanResult::UNREADABLE, std::vector<MatchInfo>());
    }

    auto returnPair = std::make_pair(ScanResult::CLEAN, std::vector<MatchInfo>());
    std::unique_ptr<ChunkReader> chunkReader;
    try {
        chunkReader.reset(ChunkReaderFactory::createReader(filePath));
    } catch (std::exception &e) {
        qWarning() << "Error creating ChunkReader: " << e.what() << " for file: " << filePath.string();
        return std::make_pair(ScanResult::UNREADABLE, std::vector<MatchInfo>());
    }

    while (true) {
        char buffer[CHUNK_SIZE];
        size_t numBytesRead = chunkReader->readChunkFromFile(buffer, CHUNK_SIZE);
        if (numBytesRead == 0) {
            break;
        }

        std::string chunk(buffer, numBytesRead);
        scanChunkWithRegex(chunk, patterns, returnPair);
    }

    if (returnPair.first == ScanResult::FLAGGED && !fileInfo.isWritable()) {
        returnPair.first = ScanResult::FLAGGED_BUT_UNWRITABLE;
    }

    return returnPair;
}


void FileScanner::scanChunkWithRegex(const std::string &chunk, const std::map<std::string, std::string> &patterns,
                                     std::pair<ScanResult, std::vector<MatchInfo>> &returnPair) {
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

            if (returnPair.second.size() == MAX_NUM_MATCHES) {
                return;
            }

            searchStart = match.suffix().first;
        }
    }
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
        int8_t buffer[CHUNK_SIZE];
        for (signed char &j: buffer) {
            j = dis(gen);
        }
        file.write((char *) buffer, CHUNK_SIZE);
    }

    // Pad the file with random data
    auto *buffer = new int8_t[numBytesToPad];
    for (int i = 0; i < numBytesToPad; i++) {
        buffer[i] = dis(gen);
    }
    file.write((char *) buffer, numBytesToPad);
    delete[] buffer;

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