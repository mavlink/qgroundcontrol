#pragma once

#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCLZMALog)

namespace QGCLZMA {
    /// Decompresses the specified file to the specified directory
    ///     @param lzmaFilename         Fully qualified path to lzma file
    ///     @param decompressedFilename Fully qualified path to for file to decompress to
    bool inflateLZMAFile(const QString &lzmaFilename, const QString &decompressedFilename);
} // namespace QGCLZMA
