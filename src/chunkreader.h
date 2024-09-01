#ifndef SENSITIVE_DATA_DELETER_CHUNKREADER_H
#define SENSITIVE_DATA_DELETER_CHUNKREADER_H

#include <string>
#include <fstream>
#include <filesystem>
#include <poppler/cpp/poppler-document.h>
#include <poppler-page.h>
#include <minizip-ng/unzip.h>
#include "tinyxml2.h"
#include <QMimeType>
#include <QMimeDatabase>
#include <QDebug>

#define UNZIP_MAX_SIZE 1024 * 1024 * 512 // 512 MB

class ChunkReader {
public:
    explicit ChunkReader(const std::filesystem::path &filePath) : filePath(filePath) {}

    explicit ChunkReader(const std::vector<uint8_t> &fileData) : fileData(fileData) {}

    ChunkReader() = default;

    virtual ~ChunkReader() = default;

    virtual size_t readChunkFromFile(char *buffer, int chunkSize) = 0;

    virtual size_t readChunkFromVector(char *buffer, int chunkSize) { return 0; }

protected:
    std::filesystem::path filePath;
    std::vector<uint8_t> fileData;
};

class PlainTextChunkReader : public ChunkReader {
public:
    explicit PlainTextChunkReader(const std::filesystem::path &filePath) :
            ChunkReader(filePath), fileStream(filePath, std::ios::binary) {
        if (!fileStream.is_open()) {
            throw std::runtime_error("Failed to open file: " + filePath.string());
        }
    }

    explicit PlainTextChunkReader(const std::vector<uint8_t> &fileData) :
            ChunkReader(fileData) {}

    size_t readChunkFromFile(char *buffer, int chunkSize) override;

    size_t readChunkFromVector(char *buffer, int chunkSize) override;

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

    PDFChunkReader(const std::vector<uint8_t> &fileData) :
            doc(poppler::document::load_from_raw_data(reinterpret_cast<const char *>(fileData.data()),
                                                      fileData.size())) {
        if (!doc) {
            throw std::runtime_error("Failed to open PDF document from memory");
        }
    }

    ~PDFChunkReader() override {
        delete doc;
    }

    size_t readChunkFromFile(char *buffer, int chunkSize) override;

private:
    poppler::document *doc;
    int pageIndex = 0;
};

class XMLChunkReader : public ChunkReader {
public:
    explicit XMLChunkReader(const std::filesystem::path &filePath) :
            ChunkReader(filePath) {
        doc.LoadFile(filePath.generic_string().c_str());
        currentNode = doc.RootElement();
        if (doc.ErrorID() != 0) {
            throw std::runtime_error("Failed to open XML document: " + filePath.string());
        }
    }

    explicit XMLChunkReader(const std::vector<uint8_t> &fileData) :
            ChunkReader(fileData) {
        doc.Parse(reinterpret_cast<const char *>(fileData.data()), fileData.size());
        currentNode = doc.RootElement();
        if (doc.ErrorID() != 0) {
            throw std::runtime_error("Failed to parse XML document from memory");
        }
    }

    size_t readChunkFromFile(char *buffer, int chunkSize) override;
    size_t extractTextFromElement(tinyxml2::XMLElement *element, char *buffer, int chunkSize);

private:
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLElement *currentNode = nullptr;
    size_t offset = 0;
};

class ZipChunkReader : public ChunkReader {
public:
    explicit ZipChunkReader(const std::filesystem::path &filePath) :
            ChunkReader(filePath), zipFile(unzOpen(filePath.generic_string().c_str())) {
        if (!zipFile) {
            throw std::runtime_error("Failed to open ZIP file: " + filePath.string());
        }
        if (unzGoToFirstFile(zipFile) != UNZ_OK) {
            unzClose(zipFile);
            throw std::runtime_error("Failed to go to the first file in the zip archive");
        }
    }

    ~ZipChunkReader() override {
        unzClose(zipFile);
    }

    std::vector<uint8_t> extractFileToMemory(unzFile zipfile);

    size_t readChunkFromFile(char *buffer, int chunkSize) override;

private:
    unzFile zipFile;
    int fileIndex = 0;
    std::string currentFileName;
    ChunkReader *currentReader = nullptr;
};

class ChunkReaderFactory {
public:
    static ChunkReader *createReader(const std::filesystem::path &filePath) {
        QMimeType mimeType = QMimeDatabase().mimeTypeForFile(QString::fromStdString(filePath.generic_string()));
        if (mimeType.inherits("application/pdf")) {
            return new PDFChunkReader(filePath);
        } else if (mimeType.inherits("application/zip")) {
            return new ZipChunkReader(filePath);
        } else if (mimeType.inherits("application/xml")) {
            return new XMLChunkReader(filePath);
        } else if (mimeType.inherits("text/plain")) {
            return new PlainTextChunkReader(filePath);
        } else {
            return nullptr;
        }
    }

    static ChunkReader *createReader(const std::vector<uint8_t> &fileData) {
        if (fileData.size() > UNZIP_MAX_SIZE) {
            return nullptr;
        }
        QMimeType mimeType = QMimeDatabase().mimeTypeForData(
                QByteArray(reinterpret_cast<const char *>(fileData.data()), fileData.size()));

        if (mimeType.inherits("application/pdf")) {
            return new PDFChunkReader(fileData);
        } else if (mimeType.inherits("application/xml")) {
            return new XMLChunkReader(fileData);
        } else if (mimeType.inherits("text/plain")) {
            return new PlainTextChunkReader(fileData);
        } else {
            return nullptr;
        }
    };
};

#endif //SENSITIVE_DATA_DELETER_CHUNKREADER_H
