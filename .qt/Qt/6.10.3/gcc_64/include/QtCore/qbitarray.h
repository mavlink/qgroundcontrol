// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QBITARRAY_H
#define QBITARRAY_H

#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class QBitRef;
class Q_CORE_EXPORT QBitArray
{
    Q_CORE_EXPORT friend QBitArray operator&(const QBitArray &a1, const QBitArray &a2);
    friend QBitArray operator&(QBitArray &&a1, const QBitArray &a2)
    { return a1 &= a2; }
    friend QBitArray operator&(const QBitArray &a1, QBitArray &&a2)
    { return a2 &= a1; }
    friend QBitArray operator&(QBitArray &&a1, QBitArray &&a2)
    { return a1 &= a2; }

    Q_CORE_EXPORT friend QBitArray operator|(const QBitArray &a1, const QBitArray &a2);
    friend QBitArray operator|(QBitArray &&a1, const QBitArray &a2)
    { return a1 |= a2; }
    friend QBitArray operator|(const QBitArray &a1, QBitArray &&a2)
    { return a2 |= a1; }
    friend QBitArray operator|(QBitArray &&a1, QBitArray &&a2)
    { return a1 |= a2; }

    Q_CORE_EXPORT friend QBitArray operator^(const QBitArray &a1, const QBitArray &a2);
    friend QBitArray operator^(QBitArray &&a1, const QBitArray &a2)
    { return a1 ^= a2; }
    friend QBitArray operator^(const QBitArray &a1, QBitArray &&a2)
    { return a2 ^= a1; }
    friend QBitArray operator^(QBitArray &&a1, QBitArray &&a2)
    { return a1 ^= a2; }

#ifndef QT_NO_DATASTREAM
    friend Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QBitArray &);
    friend Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QBitArray &);
#endif
    friend Q_CORE_EXPORT size_t qHash(const QBitArray &key, size_t seed) noexcept;
    friend QBitArray operator~(QBitArray a)
    { return std::move(a).inverted_inplace(); }
    QByteArray d;

    QBitArray(QByteArrayData &&dd) : d(std::move(dd)) {}

    template <typename BitArray> static auto bitLocation(BitArray &ba, qsizetype i)
    {
        Q_ASSERT(size_t(i) < size_t(ba.size()));
        struct R {
            decltype(ba.d[1]) byte;
            uchar bitMask;
        };
        qsizetype byteIdx = i >> 3;
        qsizetype bitIdx = i & 7;
        return R{ ba.d[1 + byteIdx], uchar(1U << bitIdx) };
    }

    QBitArray inverted_inplace() &&;

public:
    inline QBitArray() noexcept {}
    explicit QBitArray(qsizetype size, bool val = false);
    // Rule Of Zero applies
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QBitArray(const QBitArray &other) noexcept : d(other.d) {}
    inline QBitArray &operator=(const QBitArray &other) noexcept { d = other.d; return *this; }
    inline QBitArray(QBitArray &&other) noexcept : d(std::move(other.d)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QBitArray)
#endif // Qt 6

    void swap(QBitArray &other) noexcept { d.swap(other.d); }

    qsizetype size() const { return qsizetype((size_t(d.size()) << 3) - *d.constData()); }
    qsizetype count() const { return size(); }
    qsizetype count(bool on) const;

    inline bool isEmpty() const { return d.isEmpty(); }
    inline bool isNull() const { return d.isNull(); }

    void resize(qsizetype size);

    inline void detach() { d.detach(); }
    inline bool isDetached() const { return d.isDetached(); }
    inline void clear() { d.clear(); }

    bool testBit(qsizetype i) const
    { auto r = bitLocation(*this, i); return r.byte & r.bitMask; }
    void setBit(qsizetype i)
    { auto r = bitLocation(*this, i); r.byte |= r.bitMask; }
    void setBit(qsizetype i, bool val)
    { if (val) setBit(i); else clearBit(i); }
    void clearBit(qsizetype i)
    { auto r = bitLocation(*this, i); r.byte &= ~r.bitMask; }
    bool toggleBit(qsizetype i)
    {
        auto r = bitLocation(*this, i);
        bool cl = r.byte & r.bitMask;
        r.byte ^= r.bitMask;
        return cl;
    }

    bool at(qsizetype i) const { return testBit(i); }
    inline QBitRef operator[](qsizetype i);
    bool operator[](qsizetype i) const { return testBit(i); }

    QBitArray &operator&=(QBitArray &&);
    QBitArray &operator|=(QBitArray &&);
    QBitArray &operator^=(QBitArray &&);
    QBitArray &operator&=(const QBitArray &);
    QBitArray &operator|=(const QBitArray &);
    QBitArray &operator^=(const QBitArray &);
#if QT_CORE_REMOVED_SINCE(6, 7)
    QBitArray operator~() const;
#endif

#if QT_CORE_REMOVED_SINCE(6, 8)
    inline bool operator==(const QBitArray &other) const { return comparesEqual(d, other.d); }
    inline bool operator!=(const QBitArray &other) const { return !operator==(other); }
#endif

    bool fill(bool aval, qsizetype asize = -1)
    { *this = QBitArray((asize < 0 ? this->size() : asize), aval); return true; }
    void fill(bool val, qsizetype first, qsizetype last);

    inline void truncate(qsizetype pos) { if (pos < size()) resize(pos); }

    const char *bits() const { return isEmpty() ? nullptr : d.constData() + 1; }
    static QBitArray fromBits(const char *data, qsizetype len);

    quint32 toUInt32(QSysInfo::Endian endianness, bool *ok = nullptr) const noexcept;

public:
    typedef QByteArray::DataPointer DataPtr;
    inline DataPtr &data_ptr() { return d.data_ptr(); }
    inline const DataPtr &data_ptr() const { return d.data_ptr(); }

private:
    friend bool comparesEqual(const QBitArray &lhs, const QBitArray &rhs) noexcept
    {
        return lhs.d == rhs.d;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QBitArray)
};

class QT6_ONLY(Q_CORE_EXPORT) QBitRef
{
private:
    QBitArray &a;
    qsizetype i;
    inline QBitRef(QBitArray &array, qsizetype idx) : a(array), i(idx) { }
    friend class QBitArray;

public:
    inline operator bool() const { return a.testBit(i); }
    inline bool operator!() const { return !a.testBit(i); }
    QBitRef &operator=(const QBitRef &val) { a.setBit(i, val); return *this; }
    QBitRef &operator=(bool val) { a.setBit(i, val); return *this; }
};

QBitRef QBitArray::operator[](qsizetype i)
{ Q_ASSERT(i >= 0); return QBitRef(*this, i); }

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QBitArray &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QBitArray &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QBitArray &);
#endif

Q_DECLARE_SHARED(QBitArray)

QT_END_NAMESPACE

#endif // QBITARRAY_H
