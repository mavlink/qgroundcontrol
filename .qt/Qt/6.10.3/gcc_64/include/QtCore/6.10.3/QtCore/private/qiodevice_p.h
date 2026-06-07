// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIODEVICE_P_H
#define QIODEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QIODevice. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qbytearray.h"
#include "QtCore/qiodevice.h"
#include "QtCore/qobjectdefs.h"
#include "QtCore/qstring.h"
#include "QtCore/qvarlengtharray.h"
#include "private/qringbuffer_p.h"
#ifndef QT_NO_QOBJECT
#include "private/qobject_p.h"
#else
static constexpr int QObjectPrivateVersion = QT_VERSION;
#endif

QT_BEGIN_NAMESPACE

#ifndef QIODEVICE_BUFFERSIZE
#define QIODEVICE_BUFFERSIZE 16384
#endif

Q_CORE_EXPORT int qt_subtract_from_timeout(int timeout, int elapsed);

class Q_CORE_EXPORT QIODevicePrivate
#ifndef QT_NO_QOBJECT
    : public QObjectPrivate
#endif
{
    Q_DECLARE_PUBLIC(QIODevice)
    Q_DISABLE_COPY_MOVE(QIODevicePrivate)

public:
    QIODevicePrivate(decltype(QObjectPrivateVersion) version = QObjectPrivateVersion);
    virtual ~QIODevicePrivate();

    enum class ReadLineOption {
        NotNullTerminated,
        NullTerminated,
    };
    Q_DECLARE_FLAGS(ReadLineOptions, ReadLineOption)

    // The size of this class is a subject of the library hook data.
    // When adding a new member, do not make gaps and be aware
    // about the padding. Accordingly, adjust offsets in
    // tests/auto/other/toolsupport and bump the TypeInformationVersion
    // field in src/corelib/global/qhooks.cpp, to notify the developers.
    qint64 pos = 0;
    qint64 devicePos = 0;
    qint64 transactionPos = 0;

    class QRingBufferRef
    {
        QRingBuffer *m_buf;
        inline QRingBufferRef() : m_buf(nullptr) { }
        friend class QIODevicePrivate;
    public:
        // wrap functions from QRingBuffer
        inline void setChunkSize(int size) { Q_ASSERT(m_buf); m_buf->setChunkSize(size); }
        inline int chunkSize() const { Q_ASSERT(m_buf); return m_buf->chunkSize(); }
        inline qint64 nextDataBlockSize() const { return (m_buf ? m_buf->nextDataBlockSize() : Q_INT64_C(0)); }
        inline const char *readPointer() const { return (m_buf ? m_buf->readPointer() : nullptr); }
        inline const char *readPointerAtPosition(qint64 pos, qint64 &length) const { Q_ASSERT(m_buf); return m_buf->readPointerAtPosition(pos, length); }
        inline void free(qint64 bytes) { Q_ASSERT(m_buf); m_buf->free(bytes); }
        inline char *reserve(qint64 bytes) { Q_ASSERT(m_buf); return m_buf->reserve(bytes); }
        inline char *reserveFront(qint64 bytes) { Q_ASSERT(m_buf); return m_buf->reserveFront(bytes); }
        inline void truncate(qint64 pos) { Q_ASSERT(m_buf); m_buf->truncate(pos); }
        inline void chop(qint64 bytes) { Q_ASSERT(m_buf); m_buf->chop(bytes); }
        inline bool isEmpty() const { return !m_buf || m_buf->isEmpty(); }
        inline int getChar() { return (m_buf ? m_buf->getChar() : -1); }
        inline void putChar(char c) { Q_ASSERT(m_buf); m_buf->putChar(c); }
        inline void ungetChar(char c) { Q_ASSERT(m_buf); m_buf->ungetChar(c); }
        inline qint64 size() const { return (m_buf ? m_buf->size() : Q_INT64_C(0)); }
        inline void clear() { if (m_buf) m_buf->clear(); }
        inline qint64 indexOf(char c) const { return (m_buf ? m_buf->indexOf(c, m_buf->size()) : Q_INT64_C(-1)); }
        inline qint64 indexOf(char c, qint64 maxLength, qint64 pos = 0) const { return (m_buf ? m_buf->indexOf(c, maxLength, pos) : Q_INT64_C(-1)); }
        inline qint64 read(char *data, qint64 maxLength) { return (m_buf ? m_buf->read(data, maxLength) : Q_INT64_C(0)); }
        inline QByteArray read() { return (m_buf ? m_buf->read() : QByteArray()); }
        inline qint64 peek(char *data, qint64 maxLength, qint64 pos = 0) const { return (m_buf ? m_buf->peek(data, maxLength, pos) : Q_INT64_C(0)); }
        inline void append(const char *data, qint64 size) { Q_ASSERT(m_buf); m_buf->append(data, size); }
        inline void append(const QByteArray &qba) { Q_ASSERT(m_buf); m_buf->append(qba); }
        inline qint64 skip(qint64 length) { return (m_buf ? m_buf->skip(length) : Q_INT64_C(0)); }
        qint64 readLine(char *data, qint64 maxLength,
                        ReadLineOptions option = ReadLineOption::NullTerminated)
        {
            const auto appendNullByte = option & ReadLineOption::NullTerminated;
            return !m_buf         ? Q_INT64_C(-1)                    :
                   appendNullByte ? m_buf->readLine(data, maxLength) :
                                    m_buf->readLineWithoutTerminatingNull(data, maxLength);
        }
        inline bool canReadLine() const { return m_buf && m_buf->canReadLine(); }
    };

    QRingBufferRef buffer;
    QRingBufferRef writeBuffer;
    const QByteArray *currentWriteChunk = nullptr;
    int readChannelCount = 0;
    int writeChannelCount = 0;
    int currentReadChannel = 0;
    int currentWriteChannel = 0;
    int readBufferChunkSize = QIODEVICE_BUFFERSIZE;
    int writeBufferChunkSize = 0;

    QVarLengthArray<QRingBuffer, 2> readBuffers;
    QVarLengthArray<QRingBuffer, 1> writeBuffers;
    QString errorString;
    QIODevice::OpenMode openMode = QIODevice::NotOpen;

    bool transactionStarted = false;
    bool baseReadLineDataCalled = false;

    virtual bool putCharHelper(char c);

    enum AccessMode : quint8 {
        Unset,
        Sequential,
        RandomAccess
    };
    mutable AccessMode accessMode = Unset;
    inline bool isSequential() const
    {
        if (accessMode == Unset)
            accessMode = q_func()->isSequential() ? Sequential : RandomAccess;
        return accessMode == Sequential;
    }

    inline bool isBufferEmpty() const
    {
        return buffer.isEmpty() || (transactionStarted && isSequential()
                                    && transactionPos == buffer.size());
    }
    bool allWriteBuffersEmpty() const;

    void seekBuffer(qint64 newPos);

    inline void setCurrentReadChannel(int channel)
    {
        buffer.m_buf = (channel < readBuffers.size() ? &readBuffers[channel] : nullptr);
        currentReadChannel = channel;
    }
    inline void setCurrentWriteChannel(int channel)
    {
        writeBuffer.m_buf = (channel < writeBuffers.size() ? &writeBuffers[channel] : nullptr);
        currentWriteChannel = channel;
    }
    void setReadChannelCount(int count);
    void setWriteChannelCount(int count);

    qint64 read(char *data, qint64 maxSize, bool peeking = false);
    qint64 readLine(char *data, qint64 maxSize,
                    ReadLineOption option = ReadLineOption::NullTerminated);

    virtual qint64 peek(char *data, qint64 maxSize);
    virtual QByteArray peek(qint64 maxSize);
    qint64 skipByReading(qint64 maxSize);
    qint64 skipLine();
    void write(const char *data, qint64 size);

    inline bool isWriteChunkCached(const char *data, qint64 size) const
    {
        return currentWriteChunk != nullptr
               && currentWriteChunk->constData() == data
               && currentWriteChunk->size() == size;
    }

#ifdef QT_NO_QOBJECT
    QIODevice *q_ptr = nullptr;
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QIODevicePrivate::ReadLineOptions)

QT_END_NAMESPACE

#endif // QIODEVICE_P_H
