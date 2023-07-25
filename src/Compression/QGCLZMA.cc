/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCLZMA.h"

#include <QFile>
#include <QDir>
#include <QtDebug>

#include <mutex>

#include "xz.h"


static std::once_flag crc_init;

bool QGCLZMA::inflateLZMAFile(const QString& lzmaFilename, const QString& decompressedFilename)
{
    QFile inputFile(lzmaFilename);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qWarning() << "QGCLZMA::inflateLZMAFile: open input file failed" << lzmaFilename << inputFile.errorString();
        return false;
    }

    QFile outputFile(decompressedFilename);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "QGCLZMA::inflateLZMAFile: open input file failed" << outputFile.fileName() << outputFile.errorString();
        return false;
    }


    std::call_once(crc_init, []() {
        xz_crc32_init();
        xz_crc64_init();
    });


    xz_dec *s = xz_dec_init(XZ_DYNALLOC, (uint32_t)-1);
    if (s == nullptr) {
        qWarning() << "QGCLZMA::inflateLZMAFile: Memory allocation failed";
        return false;
    }

    const int buf_size = 4 * 1024;
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
            size_t cBytesWritten = (size_t)outputFile.write((char*)out, static_cast<int>(b.out_pos));
            if (cBytesWritten != b.out_pos) {
                qWarning() << "QGCLZMA::inflateLZMAFile: output file write failed:" << outputFile.fileName() << outputFile.errorString();
                goto error;
            }

            b.out_pos = 0;
        }

        if (ret == XZ_OK)
            continue;

        if (ret == XZ_UNSUPPORTED_CHECK) {
            qWarning() << "QGCLZMA::inflateLZMAFile: Unsupported check; not verifying file integrity";
            continue;
        }

        size_t cBytesWritten = (size_t)outputFile.write((char*)out, static_cast<int>(b.out_pos));
        if (cBytesWritten != b.out_pos) {
            qWarning() << "QGCLZMA::inflateLZMAFile: output file write failed:" << outputFile.fileName() << outputFile.errorString();
            goto error;
        }

        switch (ret) {
        case XZ_STREAM_END:
            xz_dec_end(s);
            return true;

        case XZ_MEM_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: Memory allocation failed";
            goto error;

        case XZ_MEMLIMIT_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: Memory usage limit reached";
            goto error;

        case XZ_FORMAT_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: Not a .xz file";
            goto error;

        case XZ_OPTIONS_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: Unsupported options in the .xz headers";
            goto error;

        case XZ_DATA_ERROR:
        case XZ_BUF_ERROR:
            qWarning() << "QGCLZMA::inflateLZMAFile: File is corrupt";
            goto error;

        default:
            qWarning() << "QGCLZMA::inflateLZMAFile: Bug!";
            goto error;
        }
    }

    xz_dec_end(s);
    return true;

error:
    xz_dec_end(s);
    return false;

}
