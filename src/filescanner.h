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
#include <QMetaObject>
#include <filesystem>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <hs/hs.h>

enum ScanResult {
    UNDEFINED,
    CLEAN,
    FLAGGED,
    UNSUPPORTED_TYPE,
    UNREADABLE,
    FLAGGED_BUT_UNWRITABLE,
};

struct MatchInfo {
    std::pair<std::string, std::string> patternUsed;
    std::string match;
    size_t startIndex;
    size_t endIndex;

    MatchInfo(const std::pair<std::string, std::string> &pattern,
              const std::string &mtch,
              size_t startIdx,
              size_t endIdx)
        : patternUsed(pattern), 
          match(mtch), 
          startIndex(startIdx), 
          endIndex(endIdx) {}
};

struct ScanContext {
    std::pair<ScanResult, std::vector<MatchInfo>> *returnPair;
    std::vector<const char *> *scanPatterns;
    std::vector<const char *> *scanPatternDescriptions;
    const char *chunk;

    ScanContext(std::pair<ScanResult, std::vector<MatchInfo>> *retPair,
                std::vector<const char *> *patterns,
                std::vector<const char *> *descriptions,
                const char *chnk)
        : returnPair(retPair), 
          scanPatterns(patterns), 
          scanPatternDescriptions(descriptions), 
          chunk(chnk) {}
};

class ThreadSafeQueue {
private:
    std::mutex mutex;
    std::condition_variable cond;
    bool done = false;

public:
    std::vector<std::filesystem::path> queue;

    void push(const std::filesystem::path &item) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(item);
        cond.notify_one();
    }

    bool pop(std::filesystem::path &item) {
        std::unique_lock<std::mutex> lock(mutex);
        cond.wait(lock, [this] { return !queue.empty() || done; });
        if (queue.empty()) return false;
        item = queue.back();
        queue.pop_back();
        return true;
    }

    void set_done() {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        cond.notify_all();
    }

};

class FileScanner {

public:
    void scanFiles(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                   const std::vector<std::string> &filePaths,
                   const std::vector<std::pair<std::string, std::string>> &patterns,
                   const std::map<std::string, std::string> &fileTypes);

    void scannerWorker(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                       std::atomic<size_t> &filesProcessed,
                       size_t totalFiles);

    std::pair<ScanResult, std::vector<MatchInfo>>
    scanFileForSensitiveData(const std::filesystem::path &filePath, hs_scratch_t *threadScratch);

    static int eventHandler(uint32_t id, uint64_t from, uint64_t to, uint32_t flags,
                            void *context);

    void scanChunkWithRegex(const char *chunk,
                            ScanContext &scanContext, hs_scratch_t *scratch);

    void deleteFiles(std::vector<std::string> &filePaths);

    void scrambleFile(const std::string &filePath);

    std::atomic<size_t> filesProcessed;
private:
    ThreadSafeQueue file_queue;
    std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>> matches;
    std::mutex matches_mutex;
    std::vector<const char *> scanPatterns;
    std::vector<const char *> scanPatternDescriptions;

    std::map<std::string, std::string> scanFileTypes;
    hs_database_t *database = nullptr;
    std::vector<uint32_t> flags;
    std::vector<uint32_t> ids;

    const hs_platform_info_t platformInfo = {
            HS_CPU_FEATURES_AVX2,
            HS_TUNE_FAMILY_GENERIC
    };
};


#endif //SENSITIVE_DATA_DELETER_FILESCANNER_H
