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

#include "lzma.h"

bool QGCLZMA::inflateLZMAFile(const QString& lzmaFilename, const QString& decompressedFilename)
{
    bool            success                 = true;
    const int       cBuffer                 = 1024 * 5;
    unsigned char   inputBuffer[cBuffer];
    unsigned char   outputBuffer[cBuffer];
    lzma_stream     lzmaStrm                = LZMA_STREAM_INIT;

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

    // UINT64_MAX - used as much memory as needed to decode
    int lzmaRetCode = lzma_alone_decoder(&lzmaStrm, UINT64_MAX);
    if (lzmaRetCode != LZMA_OK) {
        qWarning() << "QGCLZMA::inflateLZMAFile: lzma_alone_decoder failed:" << lzmaRetCode;
        return false;
    }

    // When LZMA_CONCATENATED flag was used when initializing the decoder,
    // we need to tell lzma_code() when there will be no more input.
    // This is done by setting action to LZMA_FINISH instead of LZMA_RUN
    // in the same way as it is done when encoding.
    //
    // When LZMA_CONCATENATED isn't used, there is no need to use
    // LZMA_FINISH to tell when all the input has been read, but it
    // is still OK to use it if you want. When LZMA_CONCATENATED isn't
    // used, the decoder will stop after the first .xz stream. In that
    // case some unused data may be left in lzmaStrm.next_in.
    lzma_action action = LZMA_RUN;

    lzmaStrm.next_in    = nullptr;
    lzmaStrm.avail_in   = 0;
    lzmaStrm.next_out   = nullptr;
    lzmaStrm.avail_out  = cBuffer;

    do {
        lzmaStrm.avail_in = static_cast<unsigned>(inputFile.read((char*)inputBuffer, cBuffer));
        if (lzmaStrm.avail_in == 0) {
            break;
        }
        lzmaStrm.next_in = inputBuffer;

        do {
            lzmaStrm.avail_out  = cBuffer;
            lzmaStrm.next_out   = outputBuffer;

            lzmaRetCode = lzma_code(&lzmaStrm, action);
            if (lzmaRetCode != LZMA_OK && lzmaRetCode != LZMA_STREAM_END) {
                qWarning() << "QGCLZMA::inflateLZMAFile: inflate failed:" << lzmaRetCode;
                goto Error;
            }

            unsigned cBytesInflated = cBuffer - lzmaStrm.avail_out;
            qint64 cBytesWritten = outputFile.write((char*)outputBuffer, static_cast<int>(cBytesInflated));
            if (cBytesWritten != cBytesInflated) {
                qWarning() << "QGCLZMA::inflateLZMAFile: output file write failed:" << outputFile.fileName() << outputFile.errorString();
                goto Error;

            }
        } while (lzmaStrm.avail_out == 0);
    } while (lzmaRetCode != LZMA_STREAM_END);

Out:
    lzma_end(&lzmaStrm);

    return success;

Error:
    success = false;
    goto Out;
}
