//
// Created by Olaf Seisler on 07.08.2024.
//

#include "chunkreader.h"

size_t PlainTextChunkReader::readChunkFromFile(char *buffer, int chunkSize) {

    fileStream.seekg(offset);
    fileStream.read(buffer, chunkSize);
    offset = fileStream.tellg();
    return fileStream.gcount();
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