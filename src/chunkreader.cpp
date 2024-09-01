//
// Created by Olaf Seisler on 07.08.2024.
//

#include "chunkreader.h"

size_t PlainTextChunkReader::readChunkFromFile(char *buffer, int chunkSize) {
    if (!this->fileData.empty()) {
        return readChunkFromVector(buffer, chunkSize);
    }
    fileStream.seekg(offset);
    fileStream.read(buffer, chunkSize);
    offset = fileStream.tellg();
    return fileStream.gcount();
}

size_t PlainTextChunkReader::readChunkFromVector(char *buffer, int chunkSize) {
    if (offset >= fileData.size()) {
        return 0;
    }

    size_t numBytesRead = std::min(chunkSize, static_cast<int>(fileData.size() - offset));
    std::copy(fileData.begin() + offset, fileData.begin() + offset + numBytesRead, buffer);
    offset += numBytesRead;
    return numBytesRead;
}

size_t PDFChunkReader::readChunkFromFile(char *buffer, int chunkSize) {
    if (doc->pages() == 0) {
        return 0;
    }

    size_t numBytesRead = 0;

    while (pageIndex < doc->pages()) {
        auto page = doc->create_page(pageIndex);
        if (!page) {
            break;
        }

        auto text = page->text().to_utf8();
        if (numBytesRead + text.size() > chunkSize) {
            break;
        }

        if (!text.empty()) {
            std::copy(text.begin(), text.end(), buffer + numBytesRead);
            numBytesRead += text.size();
        }

        pageIndex++;
    }

    return numBytesRead;
}

size_t XMLChunkReader::extractTextFromElement(tinyxml2::XMLElement *element, char *buffer, int chunkSize) {
    if (element == nullptr)
        return 0;

    currentNode = element;
    const char *text = element->GetText();
    // If the length of the text is greater than the chunk size, return
    if (text && offset + strlen(text) > chunkSize) {
        return 0;
    }

    // Copy the text into the buffer
    size_t numBytesRead = 0;
    if (text) {
        std::copy(text, text + strlen(text), buffer + offset);
        offset += strlen(text);
        numBytesRead += strlen(text);
    }

    // Recursively extract text from all child elements
    for (tinyxml2::XMLElement *child = element->FirstChildElement();
         child != nullptr; child = child->NextSiblingElement()) {
        numBytesRead += extractTextFromElement(child, buffer, chunkSize);
    }

    return numBytesRead;
}

size_t XMLChunkReader::readChunkFromFile(char *buffer, int chunkSize) {
    // Read the file from the file stream
    while (true) {
        if (currentNode == nullptr) {
            return 0;
        }

        size_t numBytesRead = extractTextFromElement(currentNode, buffer, chunkSize);
        if (numBytesRead == 0) {
            // If the node has unprocessed siblings, go to next one
            // If not, go to the parent node
            if (currentNode->NextSiblingElement()) {
                currentNode = currentNode->NextSiblingElement();
            } else {
                auto parent = currentNode->Parent()->ToElement();
                if (parent && parent->NextSiblingElement()) {
                    currentNode = parent->NextSiblingElement();
                } else {
                    break;
                }
            }
        } else {
            return numBytesRead;
        }
    }

    return 0;
}

// Function to extract a single file's content into memory
std::vector<uint8_t> ZipChunkReader::extractFileToMemory(unzFile zipfile) {
    unz_file_info fileInfo;
    if (unzGetCurrentFileInfo(zipfile, &fileInfo, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK) {
        throw std::runtime_error("Failed to get file info");
    }

    std::vector<uint8_t> fileData(fileInfo.uncompressed_size);

    if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
        throw std::runtime_error("Failed to open file in zip archive");
    }

    int readBytes = unzReadCurrentFile(zipfile, fileData.data(), fileData.size());
    if (readBytes < 0) {
        unzCloseCurrentFile(zipfile);
        throw std::runtime_error("Error reading file from zip archive");
    }

    unzCloseCurrentFile(zipfile);
    return fileData;
}

// Function to iterate over all files in a zip archive and process them
size_t ZipChunkReader::readChunkFromFile(char *buffer, int chunkSize) {
    size_t numBytesRead;

    if (!zipFile) {
        return 0;
    }

    // Extract the file into memory
    // Get the path of the extracted file
    char extractedFileName[256];
    std::memset(extractedFileName, 0, sizeof(extractedFileName));
    unz_file_info fileInfo;
    unzGetCurrentFileInfo(zipFile, &fileInfo, extractedFileName, sizeof(extractedFileName), nullptr, 0, nullptr, 0);

    // Compare the last and current file paths
    if (currentFileName != extractedFileName) {
        std::vector<uint8_t> fileData = extractFileToMemory(zipFile);
        qDebug() << "Extracted file: " << extractedFileName;
        currentFileName = extractedFileName;
        delete currentReader;
        currentReader = ChunkReaderFactory::createReader(fileData);
        if (!currentReader) {
            return 0;
        }
    }

    numBytesRead = currentReader->readChunkFromFile(buffer, chunkSize);
    if (numBytesRead == 0) {
        // Move to the next file in the zip archive
        if (unzGoToNextFile(zipFile) != UNZ_OK) {
            return 0;
        } else {
            return -1;
        }
    }

    return numBytesRead;
}
