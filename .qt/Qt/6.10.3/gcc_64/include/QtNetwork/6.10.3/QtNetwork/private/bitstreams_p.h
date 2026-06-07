// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#ifndef BITSTREAMS_P_H
#define BITSTREAMS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qtcoreglobal_p.h>
#include <QtCore/qassert.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtypes.h>

#include <type_traits>
#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

class QByteArray;
class QByteArrayView;

namespace HPack
{

// BitOStream works with an external buffer,
// for example, HEADERS frame.
class Q_AUTOTEST_EXPORT BitOStream
{
public:
    BitOStream(std::vector<uchar> &buffer);

    // Write 'bitLength' bits from the least significant
    // bits in 'bits' to bitstream:
    void writeBits(uchar bits, quint8 bitLength);
    // HPACK data format, we support:
    // * 32-bit integers
    // * strings
    void write(quint32 src);
    void write(QByteArrayView src, bool compressed);

    quint64 bitLength() const;
    quint64 byteLength() const;
    const uchar *begin() const;
    const uchar *end() const;

    void clear();

private:
    Q_DISABLE_COPY_MOVE(BitOStream);

    std::vector<uchar> &buffer;
    quint64 bitsSet;
};

class Q_AUTOTEST_EXPORT BitIStream
{
public:
    // Error is set by 'read' functions.
    // 'peek' does not set the error,
    // since it just peeks some bits
    // without the notion of wrong/right.
    // 'read' functions only change 'streamOffset'
    // on success.
    enum class Error
    {
        NoError,
        NotEnoughData,
        CompressionError,
        InvalidInteger
    };

    BitIStream();
    BitIStream(const uchar *f, const uchar *l);

    quint64 bitLength() const;
    bool hasMoreBits() const;

    // peekBits tries to read 'length' bits from the bitstream into
    // 'dst' ('length' must be <= sizeof(dst) * 8), packing them
    // starting from the most significant bit of the most significant
    // byte. It's a template so that we can use it with different
    // integer types. Returns the number of bits actually read.
    // Does not change stream's offset.

    template<class T>
    quint64 peekBits(quint64 from, quint64 length, T *dstPtr) const
    {
        static_assert(std::is_unsigned<T>::value, "peekBits: unsigned integer type expected");

        Q_ASSERT(dstPtr);
        Q_ASSERT(length <= sizeof(T) * 8);

        if (from >= bitLength() || !length)
            return 0;

        T &dst = *dstPtr;
        dst = T();
        length = std::min(length, bitLength() - from);

        const uchar *srcByte = first + from / 8;
        auto bitsToRead = length + from % 8;

        while (bitsToRead > 8) {
            dst = (dst << 8) | *srcByte;
            bitsToRead -= 8;
            ++srcByte;
        }

        dst <<= bitsToRead;
        dst |= *srcByte >> (8 - bitsToRead);
        dst <<= sizeof(T) * 8 - length;

        return length;
    }

    quint64 streamOffset() const
    {
        return offset;
    }

    bool skipBits(quint64 nBits);
    bool rewindOffset(quint64 nBits);

    bool read(quint32 *dstPtr);
    bool read(QByteArray *dstPtr);

    Error error() const;

private:
    void setError(Error newState);

    const uchar *first;
    const uchar *last;
    quint64 offset;
    Error streamError;
};

} // namespace HPack

QT_END_NAMESPACE

#endif
