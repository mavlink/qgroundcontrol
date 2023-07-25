/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCZlib.h"

#include <QFile>
#include <QDir>
#include <QtDebug>

#include "zlib.h"

bool QGCZlib::inflateGzipFile(const QString& gzippedFileName, const QString& decompressedFilename)
{
    bool            success                 = true;
    int             ret;
    const int       cBuffer                 = 1024 * 5;
    unsigned char   inputBuffer[cBuffer];
    unsigned char   outputBuffer[cBuffer];
    z_stream        strm;

    QFile inputFile(gzippedFileName);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qWarning() << "QGCZlib::inflateGzipFile: open input file failed" << gzippedFileName << inputFile.errorString();
        return false;
    }

    QFile outputFile(decompressedFilename);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "QGCZlib::inflateGzipFile: open input file failed" << outputFile.fileName() << outputFile.errorString();
        return false;
    }

    strm.zalloc     = nullptr;
    strm.zfree      = nullptr;
    strm.opaque     = nullptr;
    strm.avail_in   = 0;
    strm.next_in    = nullptr;

    ret = inflateInit2(&strm, 16+MAX_WBITS);
    if (ret != Z_OK) {
        qWarning() << "QGCZlib::inflateGzipFile: inflateInit2 failed:" << ret;
        goto Error;
    }

    do {
        strm.avail_in = static_cast<unsigned>(inputFile.read((char*)inputBuffer, cBuffer));
        if (strm.avail_in == 0) {
            break;
        }
        strm.next_in = inputBuffer;

        do {
            strm.avail_out  = cBuffer;
            strm.next_out   = outputBuffer;

            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                qWarning() << "QGCZlib::inflateGzipFile: inflate failed:" << ret;
                goto Error;
            }

            unsigned cBytesInflated = cBuffer - strm.avail_out;
            qint64 cBytesWritten = outputFile.write((char*)outputBuffer, static_cast<int>(cBytesInflated));
            if (cBytesWritten != cBytesInflated) {
                qWarning() << "QGCZlib::inflateGzipFile: output file write failed:" << outputFile.fileName() << outputFile.errorString();
                goto Error;

            }
        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

Out:
    inflateEnd(&strm);
    return success;

Error:
    success = false;
    goto Out;
}
