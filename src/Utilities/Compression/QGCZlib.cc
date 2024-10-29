/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCZlib.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>

#include <zlib.h>

QGC_LOGGING_CATEGORY(QGCZlibLog, "qgc.compression.qgczlib")

namespace QGCZlib
{

bool inflateGzipFile(const QString &gzippedFileName, const QString &decompressedFilename)
{
    QFile inputFile(gzippedFileName);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qCWarning(QGCZlibLog) << "open input file failed" << gzippedFileName << inputFile.errorString();
        return false;
    }

    QFile outputFile(decompressedFilename);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(QGCZlibLog) << "open output file failed" << outputFile.fileName() << outputFile.errorString();
        inputFile.close();
        return false;
    }

    z_stream strm;
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = 0;
    strm.next_in = nullptr;

    int ret = inflateInit2(&strm, 16 + MAX_WBITS);
    if (ret != Z_OK) {
        qCWarning(QGCZlibLog) << "inflateInit2 failed:" << ret;
        inputFile.close();
        outputFile.close();
        return false;
    }

    constexpr int cBuffer = 1024 * 5;
    unsigned char inputBuffer[cBuffer];
    unsigned char outputBuffer[cBuffer];
    do {
        strm.avail_in = static_cast<unsigned>(inputFile.read(reinterpret_cast<char*>(inputBuffer), cBuffer));
        if (strm.avail_in == 0) {
            break;
        }
        strm.next_in = inputBuffer;

        do {
            strm.avail_out = cBuffer;
            strm.next_out = outputBuffer;

            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                qCWarning(QGCZlibLog) << "inflate failed:" << ret;
                inflateEnd(&strm);
                inputFile.close();
                outputFile.close();
                return false;
            }

            const unsigned cBytesInflated = cBuffer - strm.avail_out;
            if (outputFile.write(reinterpret_cast<char*>(outputBuffer), cBytesInflated) != cBytesInflated) {
                qCWarning(QGCZlibLog) << "output file write failed:" << outputFile.fileName() << outputFile.errorString();
                inflateEnd(&strm);
                inputFile.close();
                outputFile.close();
                return false;
            }
        } while (strm.avail_out == 0);

    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    inputFile.close();
    outputFile.close();

    if (ret != Z_STREAM_END) {
        qCWarning(QGCZlibLog) << "inflate did not reach stream end:" << ret;
        return false;
    }

    return true;
}

} // namespace QGCZlib
