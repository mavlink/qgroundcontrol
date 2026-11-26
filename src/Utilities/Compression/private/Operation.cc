#include "Operation.h"

#include <filesystem>
#include <system_error>

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>

#include "QGCFileHelper.h"

namespace Compression {

Outcome validateFileInput(const QString& filePath)
{
    if (!QGCFileHelper::exists(filePath)) {
        qCDebug(CompressionLog) << "File does not exist:" << filePath;
        return {.error = Error::FileNotFound,
                .message = QCoreApplication::translate("Compression", "File does not exist: %1").arg(filePath)};
    }
    if (!QGCFileHelper::isQtResource(filePath) && !QFileInfo(filePath).isReadable()) {
        qCDebug(CompressionLog) << "No permission to read:" << filePath;
        return {.error = Error::PermissionDenied,
                .message = QCoreApplication::translate("Compression", "No permission to read: %1").arg(filePath)};
    }
    return {};
}

bool isSameFile(const QString& path, const QString& otherPath)
{
    std::error_code error;
    return std::filesystem::equivalent(QFileInfo(path).filesystemAbsoluteFilePath(),
                                       QFileInfo(otherPath).filesystemAbsoluteFilePath(), error) &&
           !error;
}

/// End of the failure message of a LibArchive operation, for an error code that adds to what the message says
static QString failureReason(Error error)
{
    switch (error) {
        case Error::SizeLimitExceeded:
            return QCoreApplication::translate("Compression", ": size limit exceeded",
                                               "Ends a message that tells what failed");
        case Error::Cancelled:
            return QCoreApplication::translate("Compression", ": cancelled", "Ends a message that tells what failed");
        case Error::DiskSpaceExceeded:
            return QCoreApplication::translate("Compression", ": not enough disk space",
                                               "Ends a message that tells what failed");
        case Error::PermissionDenied:
            return QCoreApplication::translate("Compression", ": no permission to write the output",
                                               "Ends a message that tells what failed");
        case Error::UnsupportedFormat:
            return QCoreApplication::translate("Compression", ": the compression is not supported by this build",
                                               "Ends a message that tells what failed");
        case Error::None:
        case Error::FileNotFound:
        case Error::InvalidArchive:
        case Error::IoError:
        case Error::InternalError:
            // The message already says that the input or the output failed
            break;
    }
    return {};
}

Outcome operationOutcome(const LibArchive::Result& result, const QString& what)
{
    if (result) {
        return {.filterName = result.filterName, .skippedEntries = result.skippedEntries};
    }
    return {.error = result.error,
            .message = what + failureReason(result.error),
            .detail = result.detail,
            .filterName = result.filterName,
            .skippedEntries = result.skippedEntries};
}

}  // namespace Compression
