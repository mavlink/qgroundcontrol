/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCLZMA.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFile>

#include <mutex>

#include <xz.h>

QGC_LOGGING_CATEGORY(QGCLZMALog, "qgc.compression.qgclzma")

static std::once_flag crc_init;

namespace QGCLZMA {

bool inflateLZMAFile(const QString &lzmaFilename, const QString &decompressedFilename)
{
    QFile inputFile(lzmaFilename);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qCWarning(QGCLZMALog) << "open input file failed" << lzmaFilename << inputFile.errorString();
        return false;
    }

    QFile outputFile(decompressedFilename);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(QGCLZMALog) << "open input file failed" << outputFile.fileName() << outputFile.errorString();
        return false;
    }

    std::call_once(crc_init, []() {
        xz_crc32_init();
        xz_crc64_init();
    });

    xz_dec* const s = xz_dec_init(XZ_DYNALLOC, static_cast<uint32_t>(-1));
    if (s == nullptr) {
        qCWarning(QGCLZMALog) << "Memory allocation failed";
        return false;
    }

    constexpr int buf_size = 4 * 1024;
    uint8_t in[buf_size];
    uint8_t out[buf_size];

    xz_buf b;
    b.in = in;
    b.in_pos = 0;
    b.in_size = 0;
    b.out = out;
    b.out_pos = 0;
    b.out_size = buf_size;

    while (true) {
        if (b.in_pos == b.in_size) {
            b.in_size = static_cast<size_t>(inputFile.read((char*)in, sizeof(in)));
            b.in_pos = 0;
        }

        xz_ret ret = xz_dec_run(s, &b);

        if (b.out_pos == sizeof(out)) {
            const size_t cBytesWritten = static_cast<size_t>(outputFile.write((char*)out, static_cast<int>(b.out_pos)));
            if (cBytesWritten != b.out_pos) {
                qCWarning(QGCLZMALog) << "output file write failed:" << outputFile.fileName() << outputFile.errorString();
                goto error;
            }

            b.out_pos = 0;
        }

        if (ret == XZ_OK) {
            continue;
        }

        if (ret == XZ_UNSUPPORTED_CHECK) {
            qCWarning(QGCLZMALog) << "Unsupported check; not verifying file integrity";
            continue;
        }

        const size_t cBytesWritten = static_cast<size_t>(outputFile.write((char*)out, static_cast<int>(b.out_pos)));
        if (cBytesWritten != b.out_pos) {
            qCWarning(QGCLZMALog) << "output file write failed:" << outputFile.fileName() << outputFile.errorString();
            goto error;
        }

        switch (ret) {
        case XZ_STREAM_END:
            xz_dec_end(s);
            return true;
        case XZ_MEM_ERROR:
            qCWarning(QGCLZMALog) << "Memory allocation failed";
            goto error;
        case XZ_MEMLIMIT_ERROR:
            qCWarning(QGCLZMALog) << "Memory usage limit reached";
            goto error;
        case XZ_FORMAT_ERROR:
            qCWarning(QGCLZMALog) << "Not a .xz file";
            goto error;
        case XZ_OPTIONS_ERROR:
            qCWarning(QGCLZMALog) << "Unsupported options in the .xz headers";
            goto error;
        case XZ_DATA_ERROR:
        case XZ_BUF_ERROR:
            qCWarning(QGCLZMALog) << "File is corrupt";
            goto error;
        default:
            qCWarning(QGCLZMALog) << "Bug!";
            goto error;
        }
    }

    xz_dec_end(s);
    return true;

error:
    xz_dec_end(s);
    return false;

}

} // namespace QGCLZMA
