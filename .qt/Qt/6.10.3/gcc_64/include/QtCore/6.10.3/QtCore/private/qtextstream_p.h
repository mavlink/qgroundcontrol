// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QTEXTSTREAM_P_H
#define QTEXTSTREAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qstringconverter.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qlocale.h>
#include "qtextstream.h"

QT_BEGIN_NAMESPACE

class QTextStreamPrivate
{
    Q_DECLARE_PUBLIC(QTextStream)
public:
    // streaming parameters
    class Params
    {
    public:
        void reset();

        // As for QString, QLocale functions taking these: the values of
        // precision, base and width can't sensibly need even eight bits, so
        // there's no sense expanding beyond int.
        int realNumberPrecision;
        int integerBase;
        int fieldWidth;
        QChar padChar;
        QTextStream::FieldAlignment fieldAlignment;
        QTextStream::RealNumberNotation realNumberNotation;
        QTextStream::NumberFlags numberFlags;
    };

    QTextStreamPrivate(QTextStream *q_ptr);
    ~QTextStreamPrivate();
    void reset();

    inline void setupDevice(QIODevice *device);
    inline void disconnectFromDevice();

    // device
    QIODevice *device;
#ifndef QT_NO_QOBJECT
    QMetaObject::Connection aboutToCloseConnection;
#endif

    // string
    QString *string;
    qsizetype stringOffset;
    QIODevice::OpenMode stringOpenMode;

    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    QStringEncoder fromUtf16;
    QStringDecoder toUtf16;
    QStringDecoder savedToUtf16;

    QString writeBuffer;
    QString readBuffer;
    qsizetype readBufferOffset;
    qsizetype readConverterSavedStateOffset; //the offset between readBufferStartDevicePos and that start of the buffer
    qint64 readBufferStartDevicePos;

    Params params;

    // status
    QTextStream::Status status;
    QLocale locale;
    QTextStream *q_ptr;

    qsizetype lastTokenSize;
    bool deleteDevice;
    bool autoDetectUnicode;
    bool hasWrittenData = false;
    bool generateBOM = false;

    // i/o
    enum TokenDelimiter {
        Space,
        NotSpace,
        EndOfLine
    };

    QString read(qsizetype maxlen);
    bool scan(const QChar **ptr, qsizetype *tokenLength,
              qsizetype maxlen, TokenDelimiter delimiter);
    inline const QChar *readPtr() const;
    inline void consumeLastToken();
    inline void consume(qsizetype nchars);
    void saveConverterState(qint64 newPos);
    void restoreToSavedConverterState();

    // Return value type for getNumber()
    enum NumberParsingStatus {
        npsOk,
        npsMissingDigit,
        npsInvalidPrefix
    };

    inline bool getChar(QChar *ch);
    inline void ungetChar(QChar ch);
    NumberParsingStatus getNumber(qulonglong *l);
    bool getReal(double *f);

    void write(QStringView data);
    void write(QChar ch);
    void write(QLatin1StringView data);
    void writePadding(qsizetype len);

    enum class PutStringMode : bool { String, Number };
    void putString(QStringView string, PutStringMode = PutStringMode::String);
    void putString(QLatin1StringView data, PutStringMode = PutStringMode::String);
    void putString(QUtf8StringView data, PutStringMode = PutStringMode::String);

    inline void putChar(QChar ch);
    void putNumber(qulonglong number, bool negative);

    struct PaddingResult {
        qsizetype left, right;
    };
    PaddingResult padding(qsizetype len) const;

    // buffers
    bool fillReadBuffer(qint64 maxBytes = -1);
    void resetReadBuffer();
    void flushWriteBuffer();

private:
    template <typename Appendable>
    void writeImpl(Appendable data);
    template <typename StringView>
    void putStringImpl(StringView view, PutStringMode mode);
};

QT_END_NAMESPACE

#endif // QTEXTSTREAM_P_H
