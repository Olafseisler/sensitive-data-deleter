//
// Created by olaf on 08/05/2024.
//

#include "chunkreader.h"
#include <QMimeDatabase>


size_t PlainTextChunkReader::readChunkFromFile(char *buffer, int chunkSize) {
    std::ifstream file(filePath, std::ios::binary);
    file.seekg(offset);
    file.read(buffer, chunkSize);
    if (file.eof()) {
        return 0;
    }
    offset = file.tellg();

    return file.gcount();
}


size_t PDFChunkReader::readChunkFromFile(char *buffer, int chunkSize) {
    size_t numBytesRead = 0;
    int i = pageIndex;

    for (; i < doc->pages(); i++) {
        poppler::page* page = doc->create_page(i);
        auto textOnPage = page->text().to_utf8();
        if (numBytesRead + textOnPage.size() > chunkSize) {
            delete page;
            break;
        }

        std::copy(textOnPage.cbegin(), textOnPage.cend(), buffer + numBytesRead);
        numBytesRead += textOnPage.size();
        delete page;
    }
    pageIndex = i;

    return numBytesRead;
}

