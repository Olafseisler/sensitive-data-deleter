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
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <algorithm>
#include <thread>

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

class ThreadSafeQueue {
protected:
public:

    std::vector<std::string> queue;
    std::mutex mutex;
    std::condition_variable cond;
    bool done = false;

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
//    FileScanner() = default;
//    ~FileScanner()= default;

    void scanFiles(QPromise<std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>>> &promise,
                          const std::vector<std::string> &filePaths,
                          const std::map<std::string, std::string> &patterns,
                          const std::map<std::string, std::string> &fileTypes);

    std::pair<ScanResult, std::vector<MatchInfo>>
    scanFileForSensitiveData(const std::filesystem::path &filePath,
                             const std::map<std::string, std::string> &fileTypes,
                             const std::map<std::string, std::string> &patterns);

    void scannerWorker(const std::map<std::string, std::string> &patterns,
                  const std::map<std::string, std::string> &fileTypes);

    void scanChunkWithRegex(const std::string &chunk, const std::map<std::string, std::string> &patterns,std::pair<ScanResult, std::vector<MatchInfo>> &returnPair);
    void deleteFiles(std::vector<std::string> &filePaths);

    void scrambleFile(const std::string &filePath);

private:
    ThreadSafeQueue file_queue;
    std::map<std::string, std::pair<ScanResult, std::vector<MatchInfo>>> matches;
    std::mutex matches_mutex;

};


#endif //SENSITIVE_DATA_DELETER_FILESCANNER_H
