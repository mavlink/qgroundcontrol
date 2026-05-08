#include "LogViewerULogParser.h"

#include "PX4ULogUtility.h"

#include <QtCore/QCoreApplication>
#include "ULogFullHandler.h"

#include <QtCore/QFile>

#include <ulog_cpp/reader.hpp>

#include <limits>

namespace ULogParser {

LogParseResult parseFile(const QString &filePath)
{
    LogParseResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "Failed to open file");
        return result;
    }

    const qint64 fileSize = file.size();
    if (fileSize <= 0) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "File is empty");
        return result;
    }
    if (fileSize > std::numeric_limits<qsizetype>::max()) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "File is too large to parse");
        return result;
    }

    uchar *const mappedData = file.map(0, fileSize);
    if (mappedData == nullptr) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "Failed to memory-map file");
        return result;
    }

    struct ScopedUnmap {
        QFile &file;
        uchar *data = nullptr;
        ~ScopedUnmap() { if (data) { file.unmap(data); } }
    } scopedUnmap{file, mappedData};

    const char *const raw = reinterpret_cast<const char *>(mappedData);

    // Verify ULog magic
    if (!PX4ULogUtility::isValidHeader(raw, fileSize)) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "File does not appear to be a ULog file (invalid header)");
        return result;
    }

    auto handler = std::make_shared<ULogFullHandler>(result);
    ulog_cpp::Reader reader(handler);
    reader.readChunk(reinterpret_cast<const uint8_t *>(raw), static_cast<size_t>(fileSize));

    if (handler->hadFatalError()) {
        if (result.errorMessage.isEmpty()) {
            result.errorMessage = QCoreApplication::translate("LogFileParser", "Fatal error while parsing ULog file");
        }
        return result;
    }

    if (!handler->isHeaderComplete()) {
        result.errorMessage = QCoreApplication::translate("LogFileParser", "ULog file header is incomplete or corrupt");
        return result;
    }

    handler->finalize();
    return result;
}

} // namespace ULogParser
