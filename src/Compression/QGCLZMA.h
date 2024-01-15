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

class QGCLZMA
{
public:
    /// Decompresses the specified file to the specified directory
    ///     @param lzmaFilename         Fully qualified path to lzma file
    ///     @param decompressedFilename Fully qualified path to for file to decompress to
    static bool inflateLZMAFile(const QString& lzmaFilename, const QString& decompressedFilename);
};
