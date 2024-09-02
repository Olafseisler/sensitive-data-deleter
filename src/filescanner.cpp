//
// Created by Olaf Seisler on 18.04.2024.
//
#include <algorithm>
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

#define CHUNK_SIZE (64 * 1024)
#define MAX_NUM_MATCHES 100 // Max number of matches per file that will be stored
#define BATCH_SIZE 10


void
FileScanner::scanFiles(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                       const std::vector<std::string> &filePaths,
                       const std::vector<std::pair<std::string, std::string>> &patterns,
                       const std::map<std::string, std::string> &fileTypes) {

    hs_compile_error_t *compile_err;
    flags = std::vector<unsigned int>(patterns.size(), HS_FLAG_SINGLEMATCH | HS_FLAG_UTF8);
    for (int i = 0; i < patterns.size(); ++i) {
        ids.push_back(i);
    }

    for (const auto &item: patterns) {
        scanPatterns.emplace_back(item.first.c_str());
        scanPatternDescriptions.emplace_back(item.second.c_str());
    }

    if (hs_compile_multi(scanPatterns.data(), flags.data(), ids.data(), scanPatterns.size(), HS_MODE_BLOCK,
                         &platformInfo, &database, &compile_err) != HS_SUCCESS) {
        QString errorMessage = QString("Failed to compile patterns: %1").arg(compile_err->message);
        hs_free_compile_error(compile_err);
        ids.clear();
        flags.clear();
        scanPatterns.clear();
        scanPatternDescriptions.clear();

        promise.setException(std::make_exception_ptr(std::runtime_error(errorMessage.toStdString())));
        promise.finish();
        return;
    }

    for (const auto &item: filePaths) {
        file_queue.push(std::filesystem::path(item));
    }

    this->scanFileTypes = fileTypes;

    // Scan files based on given patterns and file types with multiple threads
    filesProcessed = 0;
    std::vector<std::thread> threads;
    uint32_t numThreads = std::thread::hardware_concurrency();

    for (uint32_t j = 0; j < numThreads; j++) {
        threads.emplace_back(&FileScanner::scannerWorker, this,
                             std::ref(promise), std::ref(filesProcessed), filePaths.size());
    }

    file_queue.set_done();

    for (auto &thread: threads) {
        thread.join();
    }

    promise.addResult(matches);

    // Clear the scanner state
    hs_free_database(database);
    matches.clear();
    scanPatterns.clear();
    scanPatternDescriptions.clear();
    ids.clear();
    flags.clear();
    promise.finish();
}

void FileScanner::scannerWorker(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                                std::atomic<size_t> &filesProcessed,
                                size_t totalFiles) {
    hs_scratch_t *scratch = nullptr;
    hs_error_t err = hs_alloc_scratch(database, &scratch);
    if (err != HS_SUCCESS) {
        switch (err) {
            case HS_INVALID:
                qDebug() << "ERROR: Invalid parameters passed to hs_alloc_scratch.";
                break;
            case HS_NOMEM:
                qDebug() << "ERROR: Not enough memory available to allocate scratch space.";
                break;
            case HS_DB_VERSION_ERROR:
                qDebug() << "ERROR: The database provided was built for a different version of Hyperscan.";
                break;
            case HS_DB_PLATFORM_ERROR:
                qDebug() << "ERROR: The database provided was built for a different platform.";
                break;
            default:
                qDebug() << "ERROR: Unable to allocate scratch space. Unknown error occurred.";
                break;
        }
        return;
    }

    std::filesystem::path filePath;
    while (file_queue.pop(filePath)) {
        auto result = scanFileForSensitiveData(filePath, scratch);
        {
            std::lock_guard<std::mutex> lock(matches_mutex);
            matches[filePath.string()] = result;
        }
        size_t processed = ++filesProcessed;
        promise.setProgressValue(static_cast<int>((processed * 100) / totalFiles));
    }

    hs_free_scratch(scratch);
}

std::pair<ScanResult, std::vector<MatchInfo>>
FileScanner::scanFileForSensitiveData(const std::filesystem::path &filePath, hs_scratch_t *threadScratch) {

    // Check if the file extension exists in the file types map
    if (scanFileTypes.find(filePath.extension().string()) == scanFileTypes.end()) {
        return std::make_pair(ScanResult::UNSUPPORTED_TYPE, std::vector<MatchInfo>());
    }
    // If the file is not a text file based on MIME type, skip the file
    QMimeType mimeType = QMimeDatabase().mimeTypeForFile(QString::fromStdString(filePath.string()));
    if (!mimeType.inherits("text/plain") &&
        !mimeType.inherits("application/pdf") &&
        !mimeType.inherits("application/zip")) {
        return std::make_pair(ScanResult::UNSUPPORTED_TYPE, std::vector<MatchInfo>());
    }
    QFileInfo fileInfo(QString::fromStdString(filePath.string()));
    if (!fileInfo.isReadable()) {
        return std::make_pair(ScanResult::UNREADABLE, std::vector<MatchInfo>());
    }

    auto returnPair = std::make_pair(ScanResult::CLEAN, std::vector<MatchInfo>());
    ScanContext scanContext(&returnPair, &scanPatterns, &scanPatternDescriptions, nullptr);
    
    std::unique_ptr<ChunkReader> chunkReader;
    try {
        chunkReader.reset(ChunkReaderFactory::createReader(filePath));
        if (!chunkReader) {
            return std::make_pair(ScanResult::UNREADABLE, std::vector<MatchInfo>());
        }
    } catch (std::exception &e) {
        qWarning() << "Error creating ChunkReader: " << e.what() << " for file: " << filePath.string();
        return std::make_pair(ScanResult::UNREADABLE, std::vector<MatchInfo>());
    }

    while (true) {
        char buffer[CHUNK_SIZE];
        std::memset(buffer, 0, CHUNK_SIZE);
        scanContext.chunk = buffer;
        size_t numBytesRead = chunkReader->readChunkFromFile(buffer, CHUNK_SIZE);
        if (numBytesRead == 0) {
            break;
        } else if (numBytesRead == -1) {
            continue;
        }

        scanChunkWithRegex(buffer, scanContext, threadScratch);
    }

    if (returnPair.first == ScanResult::FLAGGED && !fileInfo.isWritable()) {
        returnPair.first = ScanResult::FLAGGED_BUT_UNWRITABLE;
    }

    return returnPair;
}

int FileScanner::eventHandler(uint32_t id, uint64_t from, uint64_t to, uint32_t flags,
                              void *context) {
    auto *scanContext = static_cast<ScanContext *>(context);

    if (scanContext->returnPair->second.size() >= MAX_NUM_MATCHES) {
        return 0;
    }

    uint64_t startIndex = to > 30 ? to - 30 : 0;
    uint64_t endIndex = to + 10 > CHUNK_SIZE ? CHUNK_SIZE : to + 10;
    scanContext->returnPair->first = ScanResult::FLAGGED;
    scanContext->returnPair->second.emplace_back(
        std::make_pair(scanContext->scanPatterns->at(id), scanContext->scanPatternDescriptions->at(id)),
        std::string(scanContext->chunk + startIndex, scanContext->chunk + endIndex),
        startIndex,
        to
    );

    return 0;
}


void FileScanner::scanChunkWithRegex(const char *chunk,
                                     ScanContext &scanContext, hs_scratch_t *scratch) {
    if (hs_scan(database, chunk, CHUNK_SIZE, 0, scratch, &eventHandler, &scanContext) != HS_SUCCESS) {
        qDebug() << "ERROR: Unable to scan input buffer. Likely encountered invalid UTF-8 sequence.";
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
            qWarning() << "Failed to scramble file: " << e.what();
        }
    }
}
