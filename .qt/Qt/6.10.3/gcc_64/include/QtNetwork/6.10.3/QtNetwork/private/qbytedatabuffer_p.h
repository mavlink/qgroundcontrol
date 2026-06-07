// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QBYTEDATABUFFER_P_H
#define QBYTEDATABUFFER_P_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>

#include <climits>

QT_BEGIN_NAMESPACE

// this class handles a list of QByteArrays. It is a variant of QRingBuffer
// that avoid malloc/realloc/memcpy.
class QByteDataBuffer
{
private:
    QList<QByteArray> buffers;
    qint64 bufferCompleteSize = 0;
    qint64 firstPos = 0;
public:
    static inline void popFront(QByteArray &ba, qint64 n)
    {
        ba = QByteArray(ba.constData() + n, ba.size() - n);
    }

    inline void squeezeFirst()
    {
        if (!buffers.isEmpty() && firstPos > 0) {
            popFront(buffers.first(), firstPos);
            firstPos = 0;
        }
    }

    inline void append(const QByteDataBuffer& other)
    {
        if (other.isEmpty())
            return;

        buffers.append(other.buffers);
        bufferCompleteSize += other.byteAmount();

        if (other.firstPos > 0)
            popFront(buffers[bufferCount() - other.bufferCount()], other.firstPos);
    }

    inline void append(QByteDataBuffer &&other)
    {
        if (other.isEmpty())
            return;

        auto otherBufferCount = other.bufferCount();
        auto otherByteAmount = other.byteAmount();
        buffers.append(std::move(other.buffers));
        bufferCompleteSize += otherByteAmount;

        if (other.firstPos > 0)
            popFront(buffers[bufferCount() - otherBufferCount], other.firstPos);
    }

    inline void append(const QByteArray& bd)
    {
        append(QByteArray(bd));
    }

    inline void append(QByteArray &&bd)
    {
        if (bd.isEmpty())
            return;

        bufferCompleteSize += bd.size();
        buffers.append(std::move(bd));
    }

    inline void prepend(const QByteArray& bd)
    {
        prepend(QByteArray(bd));
    }

    inline void prepend(QByteArray &&bd)
    {
        if (bd.isEmpty())
            return;

        squeezeFirst();

        bufferCompleteSize += bd.size();
        buffers.prepend(std::move(bd));
    }

    // return the first QByteData. User of this function has to free() its .data!
    // preferably use this function to read data.
    inline QByteArray read()
    {
        Q_ASSERT(!isEmpty());
        squeezeFirst();
        bufferCompleteSize -= buffers.first().size();
        return buffers.takeFirst();
    }

    // return everything. User of this function has to free() its .data!
    // avoid to use this, it might malloc and memcpy.
    inline QByteArray readAll()
    {
        return read(byteAmount());
    }

    // return amount. User of this function has to free() its .data!
    // avoid to use this, it might malloc and memcpy.
    inline QByteArray read(qint64 amount)
    {
        amount = qMin(byteAmount(), amount);
        if constexpr (sizeof(qsizetype) == sizeof(int)) { // 32-bit
            // While we cannot overall have more than INT_MAX memory allocated,
            // the QByteArrays we hold may be shared copies of each other,
            // causing byteAmount() to exceed INT_MAX.
            if (amount > INT_MAX)
                qBadAlloc(); // what resize() would do if it saw past the truncation
        }
        QByteArray byteData;
        byteData.resize(qsizetype(amount));
        read(byteData.data(), byteData.size());
        return byteData;
    }

    // return amount bytes. User of this function has to free() its .data!
    // avoid to use this, it will memcpy.
    qint64 read(char* dst, qint64 amount)
    {
        amount = qMin(amount, byteAmount());
        qint64 originalAmount = amount;
        char *writeDst = dst;

        while (amount > 0) {
            const QByteArray &first = buffers.first();
            qint64 firstSize = first.size() - firstPos;
            if (amount >= firstSize) {
                // take it completely
                bufferCompleteSize -= firstSize;
                amount -= firstSize;
                memcpy(writeDst, first.constData() + firstPos, firstSize);
                writeDst += firstSize;
                firstPos = 0;
                buffers.takeFirst();
            } else {
                // take a part of it & it is the last one to take
                bufferCompleteSize -= amount;
                memcpy(writeDst, first.constData() + firstPos, amount);
                firstPos += amount;
                amount = 0;
            }
        }

        return originalAmount;
    }

    /*!
        \internal
        Returns a view into the first QByteArray contained inside,
        ignoring any already read data. Call advanceReadPointer()
        to advance the view forward. When a QByteArray is exhausted
        the view returned by this function will view into another
        QByteArray if any. Returns a default constructed view if
        no data is available.

        \sa advanceReadPointer
    */
    QByteArrayView readPointer() const
    {
        if (isEmpty())
            return {};
        return { buffers.first().constData() + qsizetype(firstPos),
                 buffers.first().size() - qsizetype(firstPos) };
    }

    /*!
        \internal
        Advances the read pointer by \a distance.

        \sa readPointer
    */
    void advanceReadPointer(qint64 distance)
    {
        qint64 newPos = firstPos + distance;
        if (isEmpty()) {
            newPos = 0;
        } else if (auto size = buffers.first().size(); newPos >= size) {
            while (newPos >= size) {
                bufferCompleteSize -= (size - firstPos);
                newPos -= size;
                buffers.pop_front();
                if (isEmpty()) {
                    size = 0;
                    newPos = 0;
                    break;
                }
                size = buffers.front().size();
            }
            bufferCompleteSize -= newPos;
        } else {
            bufferCompleteSize -= newPos - firstPos;
        }
        firstPos = newPos;
    }

    inline char getChar()
    {
        Q_ASSERT_X(!isEmpty(), "QByteDataBuffer::getChar",
                   "Cannot read a char from an empty buffer!");
        char c;
        read(&c, 1);
        return c;
    }

    inline void clear()
    {
        buffers.clear();
        bufferCompleteSize = 0;
        firstPos = 0;
    }

    // The byte count of all QByteArrays
    inline qint64 byteAmount() const
    {
        return bufferCompleteSize;
    }

    // the number of QByteArrays
    qsizetype bufferCount() const
    {
        return buffers.size();
    }

    inline bool isEmpty() const
    {
        return byteAmount() == 0;
    }

    inline qint64 sizeNextBlock() const
    {
        if (buffers.isEmpty())
            return 0;
        else
            return buffers.first().size() - firstPos;
    }

    QByteArray &operator[](qsizetype i)
    {
        if (i == 0)
            squeezeFirst();

        return buffers[i];
    }

    inline bool canReadLine() const {
        qsizetype i = 0;
        if (i < buffers.size()) {
            if (buffers.at(i).indexOf('\n', firstPos) != -1)
                return true;
            ++i;

            for (; i < buffers.size(); i++)
                if (buffers.at(i).contains('\n'))
                    return true;
        }
        return false;
    }

    const QByteArray &last() const { return buffers.last(); }
};

QT_END_NAMESPACE

#endif // QBYTEDATABUFFER_P_H
