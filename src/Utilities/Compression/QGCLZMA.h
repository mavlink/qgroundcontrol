/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
