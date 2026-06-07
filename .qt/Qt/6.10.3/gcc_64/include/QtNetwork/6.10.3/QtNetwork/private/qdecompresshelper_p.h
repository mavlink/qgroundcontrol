// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef DECOMPRESS_HELPER_P_H
#define DECOMPRESS_HELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of the Network Access API. This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/qtnetworkexports.h>
#include <QtNetwork/private/qbytedatabuffer_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QIODevice;
class QDecompressHelper
{
public:
    enum ContentEncoding {
        None,
        Deflate,
        GZip,
        Brotli,
        Zstandard,
    };

    QDecompressHelper() = default;
    Q_NETWORK_EXPORT ~QDecompressHelper();

    Q_NETWORK_EXPORT bool setEncoding(QByteArrayView contentEncoding);

    Q_NETWORK_EXPORT bool isCountingBytes() const;
    Q_NETWORK_EXPORT void setCountingBytesEnabled(bool shouldCount);

    Q_NETWORK_EXPORT qint64 uncompressedSize() const;

    Q_NETWORK_EXPORT bool hasData() const;
    Q_NETWORK_EXPORT void feed(const QByteArray &data);
    Q_NETWORK_EXPORT void feed(QByteArray &&data);
    Q_NETWORK_EXPORT void feed(const QByteDataBuffer &buffer);
    Q_NETWORK_EXPORT void feed(QByteDataBuffer &&buffer);
    Q_NETWORK_EXPORT qsizetype read(char *data, qsizetype maxSize);

    Q_NETWORK_EXPORT bool isValid() const;

    Q_NETWORK_EXPORT void clear();

    Q_NETWORK_EXPORT void setDecompressedSafetyCheckThreshold(qint64 threshold);

    Q_NETWORK_EXPORT static bool isSupportedEncoding(QByteArrayView encoding);
    Q_NETWORK_EXPORT static QByteArrayList acceptedEncoding();

    Q_NETWORK_EXPORT QString errorString() const;

private:
    bool isPotentialArchiveBomb() const;
    bool hasDataInternal() const;
    qsizetype readInternal(char *data, qsizetype maxSize);

    bool countInternal();
    bool countInternal(const QByteArray &data);
    bool countInternal(const QByteDataBuffer &buffer);

    bool setEncoding(ContentEncoding ce);
    qint64 encodedBytesAvailable() const;

    qsizetype readZLib(char *data, qsizetype maxSize);
    qsizetype readBrotli(char *data, qsizetype maxSize);
    qsizetype readZstandard(char *data, qsizetype maxSize);

    QByteDataBuffer compressedDataBuffer;
    QByteDataBuffer decompressedDataBuffer;
    const qsizetype MaxDecompressedDataBufferSize = 10 * 1024 * 1024;
    bool decoderHasData = false;

    bool countDecompressed = false;
    std::unique_ptr<QDecompressHelper> countHelper;

    QString errorStr;

    // Used for calculating the ratio
    qint64 archiveBombCheckThreshold = 10 * 1024 * 1024;
    qint64 totalUncompressedBytes = 0;
    qint64 totalCompressedBytes = 0;
    qint64 totalBytesRead = 0;

    ContentEncoding contentEncoding = None;

    void *decoderPointer = nullptr;
#if QT_CONFIG(brotli)
    const uint8_t *brotliUnconsumedDataPtr = nullptr;
    size_t brotliUnconsumedAmount = 0;
#endif
};

QT_END_NAMESPACE

#endif // DECOMPRESS_HELPER_P_H
