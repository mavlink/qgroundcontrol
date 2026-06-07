// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRINGBUFFER_P_H
#define QRINGBUFFER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

#ifndef QRINGBUFFER_CHUNKSIZE
#define QRINGBUFFER_CHUNKSIZE 4096
#endif

class QRingChunk
{
public:
    // initialization and cleanup
    QRingChunk() noexcept = default;
    explicit inline QRingChunk(qsizetype alloc) :
        chunk(alloc, Qt::Uninitialized), tailOffset(0)
    {
    }
    explicit inline QRingChunk(const QByteArray &qba) noexcept :
        chunk(qba), tailOffset(qba.size())
    {
    }
    explicit QRingChunk(QByteArray &&qba) noexcept :
        chunk(std::move(qba)), tailOffset(chunk.size())
    {
    }

    inline void swap(QRingChunk &other) noexcept
    {
        chunk.swap(other.chunk);
        qSwap(headOffset, other.headOffset);
        qSwap(tailOffset, other.tailOffset);
    }

    // allocating and sharing
    void allocate(qsizetype alloc);
    inline bool isShared() const
    {
        return !chunk.isDetached();
    }
    Q_CORE_EXPORT void detach();
    QByteArray toByteArray() &&;

    // getters
    inline qsizetype head() const
    {
        return headOffset;
    }
    inline qsizetype size() const
    {
        return tailOffset - headOffset;
    }
    inline qsizetype capacity() const
    {
        return chunk.size();
    }
    inline qsizetype available() const
    {
        return chunk.size() - tailOffset;
    }
    inline const char *data() const
    {
        return chunk.constData() + headOffset;
    }
    inline char *data()
    {
        if (isShared())
            detach();
        return chunk.data() + headOffset;
    }

    // array management
    inline void advance(qsizetype offset)
    {
        Q_ASSERT(headOffset + offset >= 0);
        Q_ASSERT(size() - offset > 0);

        headOffset += offset;
    }
    inline void grow(qsizetype offset)
    {
        Q_ASSERT(size() + offset > 0);
        Q_ASSERT(head() + size() + offset <= capacity());

        tailOffset += offset;
    }
    inline void assign(const QByteArray &qba)
    {
        chunk = qba;
        headOffset = 0;
        tailOffset = qba.size();
    }
    void assign(QByteArray &&qba)
    {
        chunk = std::move(qba);
        headOffset = 0;
        tailOffset = chunk.size();
    }
    inline void reset()
    {
        headOffset = tailOffset = 0;
    }
    inline void clear()
    {
        *this = {};
    }

private:
    QByteArray chunk;
    qsizetype headOffset = 0;
    qsizetype tailOffset = 0;
};
Q_DECLARE_SHARED(QRingChunk)

class QRingBuffer
{
    Q_DISABLE_COPY(QRingBuffer)
public:
    explicit inline QRingBuffer(int growth = QRINGBUFFER_CHUNKSIZE) :
        bufferSize(0), basicBlockSize(growth) { }

    QRingBuffer(QRingBuffer &&) noexcept = default;
    QRingBuffer &operator=(QRingBuffer &&) noexcept = default;

    inline void setChunkSize(int size) {
        basicBlockSize = size;
    }

    inline int chunkSize() const {
        return basicBlockSize;
    }

    inline qint64 nextDataBlockSize() const {
        return bufferSize == 0 ? Q_INT64_C(0) : buffers.first().size();
    }

    inline const char *readPointer() const {
        return bufferSize == 0 ? nullptr : buffers.first().data();
    }

    Q_CORE_EXPORT const char *readPointerAtPosition(qint64 pos, qint64 &length) const;
    Q_CORE_EXPORT void free(qint64 bytes);
    Q_CORE_EXPORT char *reserve(qint64 bytes);
    Q_CORE_EXPORT char *reserveFront(qint64 bytes);

    inline void truncate(qint64 pos) {
        Q_ASSERT(pos >= 0 && pos <= size());

        chop(size() - pos);
    }

    Q_CORE_EXPORT void chop(qint64 bytes);

    inline bool isEmpty() const {
        return bufferSize == 0;
    }

    inline int getChar() {
        if (isEmpty())
            return -1;
        char c = *readPointer();
        free(1);
        return int(uchar(c));
    }

    inline void putChar(char c) {
        char *ptr = reserve(1);
        *ptr = c;
    }

    void ungetChar(char c)
    {
        char *ptr = reserveFront(1);
        *ptr = c;
    }


    inline qint64 size() const {
        return bufferSize;
    }

    Q_CORE_EXPORT void clear();
    inline qint64 indexOf(char c) const { return indexOf(c, size()); }
    Q_CORE_EXPORT qint64 indexOf(char c, qint64 maxLength, qint64 pos = 0) const;
    Q_CORE_EXPORT qint64 read(char *data, qint64 maxLength);
    Q_CORE_EXPORT QByteArray read();
    Q_CORE_EXPORT qint64 peek(char *data, qint64 maxLength, qint64 pos = 0) const;
    Q_CORE_EXPORT void append(const char *data, qint64 size);
    Q_CORE_EXPORT void append(const QByteArray &qba);
    Q_CORE_EXPORT void append(QByteArray &&qba);

    inline qint64 skip(qint64 length) {
        qint64 bytesToSkip = qMin(length, bufferSize);

        free(bytesToSkip);
        return bytesToSkip;
    }

    Q_CORE_EXPORT qint64 readLineWithoutTerminatingNull(char *data, qint64 maxLength);
    Q_CORE_EXPORT qint64 readLine(char *data, qint64 maxLength);

    inline bool canReadLine() const {
        return indexOf('\n') >= 0;
    }

private:
    QList<QRingChunk> buffers;
    qint64 bufferSize;
    int basicBlockSize;
};

Q_DECLARE_TYPEINFO(QRingBuffer, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QRINGBUFFER_P_H
