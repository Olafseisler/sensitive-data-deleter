#ifndef SENSITIVE_DATA_DELETER_CHUNKREADER_H
#define SENSITIVE_DATA_DELETER_CHUNKREADER_H

#include <string>
#include <fstream>
#include <filesystem>
#include <poppler/cpp/poppler-document.h>
#include <poppler-page.h>
#include <QMimeType>
#include <QMimeDatabase>


class ChunkReader {
public:
    explicit ChunkReader(const std::filesystem::path &filePath) : filePath(filePath) {}
    virtual ~ChunkReader() = default;
    virtual size_t readChunkFromFile(char *buffer, int chunkSize) = 0;

protected:
    std::filesystem::path filePath;
};

class PlainTextChunkReader : public ChunkReader {
public:
    explicit PlainTextChunkReader(const std::filesystem::path &filePath) :
            ChunkReader(filePath), fileStream(filePath, std::ios::binary) {}
    size_t readChunkFromFile(char *buffer, int chunkSize) override;

private:
    std::ifstream fileStream;
    std::streamsize offset = 0;
};

class PDFChunkReader : public ChunkReader {
public:
    explicit PDFChunkReader(const std::filesystem::path &filePath) :
            ChunkReader(filePath), doc(poppler::document::load_from_file(filePath.generic_string())) {
        if (!doc) {
            throw std::runtime_error("Failed to open PDF document: " + filePath.string());
        }
    }
    ~PDFChunkReader() override {
        delete doc;
    }
    size_t readChunkFromFile(char *buffer, int chunkSize) override;

private:
    poppler::document* doc;
    int pageIndex = 0;
};

class ChunkReaderFactory {
public:
    static ChunkReader* createReader(const std::filesystem::path &filePath) {
        QMimeType mimeType = QMimeDatabase().mimeTypeForFile(QString::fromStdString(filePath.generic_string()));
        if (mimeType.inherits("text/plain")) {
            return new PlainTextChunkReader(filePath);
        } else if (mimeType.inherits("application/pdf")) {
            return new PDFChunkReader(filePath);
        }
        return nullptr;
    }
};

#endif //SENSITIVE_DATA_DELETER_CHUNKREADER_H
