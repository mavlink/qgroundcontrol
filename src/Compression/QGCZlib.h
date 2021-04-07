/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QString>

class QGCZlib
{
public:
    /// Decompresses the specified file to the specified directory
    ///     @param gzipFilename         Fully qualified path to gzip file
    ///     @param decompressedFilename Fully qualified path to for file to decompress to
    static bool inflateGzipFile(const QString& gzippedFileName, const QString& decompressedFilename);
};
