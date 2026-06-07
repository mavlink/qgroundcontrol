// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QUUID_H
#define QUUID_H

#include <QtCore/qcompare.h>
#include <QtCore/qendian.h>
#include <QtCore/qstring.h>
#include <QtCore/qsystemdetection.h>

#if defined(Q_OS_WIN) || defined(Q_QDOC)
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    ulong   Data1;
    ushort  Data2;
    ushort  Data3;
    uchar   Data4[8];
} GUID, *REFGUID, *LPGUID;
#endif
#endif

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
Q_FORWARD_DECLARE_CF_TYPE(CFUUID);
Q_FORWARD_DECLARE_OBJC_CLASS(NSUUID);
#endif

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QUuid
{
    QUuid(Qt::Initialization) {}
public:
    enum Variant {
        VarUnknown        =-1,
        NCS                = 0, // 0 - -
        DCE                = 2, // 1 0 -
        Microsoft        = 6, // 1 1 0
        Reserved        = 7  // 1 1 1
    };

    enum Version {
        VerUnknown        =-1,
        Time                = 1, // 0 0 0 1
        EmbeddedPOSIX        = 2, // 0 0 1 0
        Md5                 = 3, // 0 0 1 1
        Name = Md5,
        Random                = 4,  // 0 1 0 0
        Sha1            = 5, // 0 1 0 1
        UnixEpoch       = 7, // 0 1 1 1
    };

    enum StringFormat {
        WithBraces      = 0,
        WithoutBraces   = 1,
        Id128           = 3
    };

    union alignas(16) Id128Bytes {
        quint8 data[16];
        quint16 data16[8];
        quint32 data32[4];
        quint64 data64[2];
#if defined(QT_COMPILER_SUPPORTS_INT128)
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wpedantic")    // ISO C++ does not support ‘__int128’ for ‘data128’
        unsigned __int128 data128[1];
QT_WARNING_POP
#elif defined(QT_SUPPORTS_INT128)
#  error "struct QUuid::Id128Bytes should not depend on QT_SUPPORTS_INT128 for ABI reasons."
#  error "Adjust the declaration of the `data128` member above so it is always defined if it's " \
        "supported by the current compiler/architecture in any configuration."
#endif

        constexpr explicit operator QByteArrayView() const noexcept
        {
            return QByteArrayView(data, sizeof(data));
        }

        friend constexpr Id128Bytes qbswap(Id128Bytes b) noexcept
        {
            // 128-bit byte swap
            auto b0 = qbswap(b.data64[0]);
            auto b1 = qbswap(b.data64[1]);
            b.data64[0] = b1;
            b.data64[1] = b0;
            return b;
        }
    };

    constexpr QUuid() noexcept : data1(0), data2(0), data3(0), data4{0,0,0,0,0,0,0,0} {}

    constexpr QUuid(uint l, ushort w1, ushort w2, uchar b1, uchar b2, uchar b3,
                           uchar b4, uchar b5, uchar b6, uchar b7, uchar b8) noexcept
        : data1(l), data2(w1), data3(w2), data4{b1, b2, b3, b4, b5, b6, b7, b8} {}
    explicit inline QUuid(Id128Bytes id128, QSysInfo::Endian order = QSysInfo::BigEndian) noexcept;

    explicit QUuid(QAnyStringView string) noexcept
        : QUuid{fromString(string)} {}
    static QUuid fromString(QAnyStringView string) noexcept;
#if QT_CORE_REMOVED_SINCE(6, 3)
    explicit QUuid(const QString &);
    static QUuid fromString(QStringView string) noexcept;
    static QUuid fromString(QLatin1StringView string) noexcept;
    explicit QUuid(const char *);
    explicit QUuid(const QByteArray &);
#endif
    QString toString(StringFormat mode = WithBraces) const;
    QByteArray toByteArray(StringFormat mode = WithBraces) const;
    inline Id128Bytes toBytes(QSysInfo::Endian order = QSysInfo::BigEndian) const noexcept;
    QByteArray toRfc4122() const;

    static inline QUuid fromBytes(const void *bytes, QSysInfo::Endian order = QSysInfo::BigEndian);
#if QT_CORE_REMOVED_SINCE(6, 3)
    static QUuid fromRfc4122(const QByteArray &);
#endif
    static QUuid fromRfc4122(QByteArrayView) noexcept;

#if QT_CORE_REMOVED_SINCE(6, 9)
    bool isNull() const noexcept;
#endif
    constexpr bool isNull(QT6_DECL_NEW_OVERLOAD) const noexcept
    {
#if defined(__cpp_lib_bit_cast) && defined(QT_SUPPORTS_INT128)
        return std::bit_cast<quint128>(*this) == 0;
#else
        // QNX fails to compile
        // data4[0] == 0 && data4[1] == 0 && ...
        // in constexpr context, so rewrite it using a loop. This way we have
        // only single data4[i] != 0 check at each iteration
        for (size_t i = 0; i < 8; ++i) {
            if (data4[i] != 0)
                return false;
        }
        return data1 == 0 && data2 == 0 && data3 == 0;
#endif
    }

#ifdef QT_SUPPORTS_INT128
    static constexpr QUuid fromUInt128(quint128 uuid, QSysInfo::Endian order = QSysInfo::BigEndian) noexcept;
    constexpr quint128 toUInt128(QSysInfo::Endian order = QSysInfo::BigEndian) const noexcept;
#endif

private:
    friend constexpr bool comparesEqual(const QUuid &lhs, const QUuid &rhs) noexcept
    {
        return is_eq(compareThreeWay_helper(lhs, rhs));
    }
    static constexpr Qt::strong_ordering
    compareThreeWay_helper(const QUuid &lhs, const QUuid &rhs) noexcept
    {
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
        if (const auto c = Qt::compareThreeWay(lhs.data1, rhs.data1); !is_eq(c))
            return c;
        if (const auto c = Qt::compareThreeWay(lhs.data2, rhs.data2); !is_eq(c))
            return c;
        if (const auto c = Qt::compareThreeWay(lhs.data3, rhs.data3); !is_eq(c))
            return c;
#elif defined(__cpp_lib_bit_cast) && defined(QT_SUPPORTS_INT128)
        quint128 lu = qFromBigEndian(std::bit_cast<quint128>(lhs));
        quint128 ru = qFromBigEndian(std::bit_cast<quint128>(rhs));
        return Qt::compareThreeWay(lu, ru);
#else
        auto make_int = [](const QUuid &u) {
            quint64 result = quint64(u.data3) << 48;
            result |= quint64(u.data2) << 32;
            return qFromBigEndian(result | u.data1);
        };
        if (const auto c = Qt::compareThreeWay(make_int(lhs), make_int(rhs)); !is_eq(c))
            return c;
#endif
        for (unsigned i = 0; i < sizeof(lhs.data4); ++i) {
            if (const auto c = Qt::compareThreeWay(lhs.data4[i], rhs.data4[i]); !is_eq(c))
                return c;
        }
        return Qt::strong_ordering::equal;
    }
    friend constexpr Qt::strong_ordering compareThreeWay(const QUuid &lhs, const QUuid &rhs) noexcept
    {
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0) && !defined(QT_BOOTSTRAPPED)
        // Keep the old sorting order from before Qt 6.8, which sorted first on
        // variant(). We don't need the exact algorithm to achieve same results.
        auto fastVariant = [](const QUuid &uuid) {
            quint8 v = uuid.data4[0];
            // i.e.: return v >= Microsoft ? v : v >= DCE ? DCE : NCS;
            return v >= 0xC0 ? v & 0xE0 : v >= 0x80 ? 0x80 : 0;
        };
        if (const auto c = Qt::compareThreeWay(fastVariant(lhs), fastVariant(rhs)); !is_eq(c))
            return c;
#endif
        return compareThreeWay_helper(lhs, rhs);
    }

public:
/*  To prevent a meta-type creation ambiguity on Windows, we put comparison
    macros under NOT QT_CORE_REMOVED_SINCE(6, 8) part. */
#if QT_CORE_REMOVED_SINCE(6, 8)
    constexpr bool operator==(const QUuid &orig) const noexcept
    {
        return comparesEqual(*this, orig);
    }

    constexpr bool operator!=(const QUuid &orig) const noexcept
    {
        return !operator==(orig);
    }

    bool operator<(const QUuid &other) const noexcept;
    bool operator>(const QUuid &other) const noexcept;
#else
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QUuid)
#endif // QT_CORE_REMOVED_SINCE(6, 8)
#if defined(Q_OS_WIN) || defined(Q_QDOC)
    // On Windows we have a type GUID that is used by the platform API, so we
    // provide convenience operators to cast from and to this type.
    constexpr QUuid(const GUID &guid) noexcept
        : data1(guid.Data1), data2(guid.Data2), data3(guid.Data3),
          data4{guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]} {}

    constexpr QUuid &operator=(const GUID &guid) noexcept
    {
        *this = QUuid(guid);
        return *this;
    }

    constexpr operator GUID() const noexcept
    {
        GUID guid = { data1, data2, data3, { data4[0], data4[1], data4[2], data4[3], data4[4], data4[5], data4[6], data4[7] } };
        return guid;
    }
private:
    friend constexpr bool comparesEqual(const QUuid &lhs, const GUID &rhs) noexcept
    {
        return comparesEqual(lhs, QUuid(rhs));
    }
public:
/*  To prevent a meta-type creation ambiguity on Windows, we put comparison
    macros under NOT QT_CORE_REMOVED_SINCE(6, 8) part. */
#if QT_CORE_REMOVED_SINCE(6, 8)
    constexpr bool operator==(const GUID &guid) const noexcept
    {
        return comparesEqual(*this, QUuid(guid));
    }

    constexpr bool operator!=(const GUID &guid) const noexcept
    {
        return !operator==(guid);
    }
#else
    Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(QUuid, GUID)
#endif // !QT_CORE_REMOVED_SINCE(6, 8)
#endif
public:
    static QUuid createUuid();
#if QT_CORE_REMOVED_SINCE(6, 8)
    static QUuid createUuidV3(const QUuid &ns, const QByteArray &baseData) noexcept;
    static QUuid createUuidV5(const QUuid &ns, const QByteArray &baseData) noexcept;
#endif
    static QUuid createUuidV5(QUuid ns, QByteArrayView baseData) noexcept;
#ifndef QT_BOOTSTRAPPED
    static QUuid createUuidV3(QUuid ns, QByteArrayView baseData) noexcept;
#if !QT_CORE_REMOVED_SINCE(6, 8)
    Q_WEAK_OVERLOAD
#endif
    static inline QUuid createUuidV3(const QUuid &ns, const QString &baseData)
    {
        return QUuid::createUuidV3(ns, qToByteArrayViewIgnoringNull(baseData.toUtf8()));
    }
#endif
#if !QT_CORE_REMOVED_SINCE(6, 8)
    Q_WEAK_OVERLOAD
#endif
    static inline QUuid createUuidV5(const QUuid &ns, const QString &baseData)
    {
        return QUuid::createUuidV5(ns, qToByteArrayViewIgnoringNull(baseData.toUtf8()));
    }

    static QUuid createUuidV7();

private:
    static constexpr bool isKnownVersion(Version v) noexcept
    {
        switch (v) {
        case VerUnknown:
            return false;
        case Time:
        case EmbeddedPOSIX:
        case Md5:
        case Random:
        case Sha1:
        case UnixEpoch:
            return true;
        }
        return false;
    }

public:
#if QT_CORE_REMOVED_SINCE(6, 9)
    QUuid::Variant variant() const noexcept;
    QUuid::Version version() const noexcept;
#endif
    constexpr Variant variant(QT6_DECL_NEW_OVERLOAD) const noexcept
    {
        // Check the 3 MSB of data4[0]
        const quint8 var = data4[0] & 0xE0;
        if (var < 0x80)
            return isNull(QT6_CALL_NEW_OVERLOAD) ? VarUnknown : NCS;
        if (var < 0xC0)
            return DCE;
        return Variant(var >> 5); // Microsoft or Reserved
    }
    constexpr Version version(QT6_DECL_NEW_OVERLOAD) const noexcept
    {
        // Check the 4 MSB of data3
        const Version ver = Version(data3 >> 12);
        // Check that variant() == DCE and version is in a valid range
        if (isKnownVersion(ver) && (data4[0] & 0xC0) == 0x80)
            return ver;
        return VerUnknown;
    }

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    static QUuid fromCFUUID(CFUUIDRef uuid);
    CFUUIDRef toCFUUID() const Q_DECL_CF_RETURNS_RETAINED;
    static QUuid fromNSUUID(const NSUUID *uuid);
    NSUUID *toNSUUID() const Q_DECL_NS_RETURNS_AUTORELEASED;
#endif

    uint    data1;
    ushort  data2;
    ushort  data3;
    uchar   data4[8];
};

Q_DECLARE_TYPEINFO(QUuid, Q_PRIMITIVE_TYPE);

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QUuid &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QUuid &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QUuid &);
#endif

Q_CORE_EXPORT size_t qHash(const QUuid &uuid, size_t seed = 0) noexcept;

QUuid::QUuid(Id128Bytes uuid, QSysInfo::Endian order) noexcept
{
    char bytes[sizeof uuid];
    if (order == QSysInfo::LittleEndian)
        qbswap(uuid, bytes);
    else
        memcpy(bytes, &uuid, sizeof bytes);
    data1 = qFromBigEndian<quint32>(&bytes[0]);
    data2 = qFromBigEndian<quint16>(&bytes[4]);
    data3 = qFromBigEndian<quint16>(&bytes[6]);
    memcpy(data4, &bytes[8], sizeof(data4));
}

QUuid::Id128Bytes QUuid::toBytes(QSysInfo::Endian order) const noexcept
{
    Id128Bytes result = {};
    qToBigEndian(data1, &result.data[0]);
    qToBigEndian(data2, &result.data[4]);
    qToBigEndian(data3, &result.data[6]);
    memcpy(&result.data[8], data4, sizeof(data4));
    if (order == QSysInfo::LittleEndian)
        return qbswap(result);
    return result;
}

QUuid QUuid::fromBytes(const void *bytes, QSysInfo::Endian order)
{
    Id128Bytes result = {};
    memcpy(result.data, bytes, sizeof(result));
    return QUuid(result, order);
}

#ifdef QT_SUPPORTS_INT128
constexpr QUuid QUuid::fromUInt128(quint128 uuid, QSysInfo::Endian order) noexcept
{
    QUuid result = {};
    if (order == QSysInfo::BigEndian) {
        result.data1 = qFromBigEndian<quint32>(int(uuid));
        result.data2 = qFromBigEndian<quint16>(ushort(uuid >> 32));
        result.data3 = qFromBigEndian<quint16>(ushort(uuid >> 48));
        for (int i = 0; i < 8; ++i)
            result.data4[i] = uchar(uuid >> (64 + i * 8));
    } else {
        result.data1 = qFromLittleEndian<quint32>(uint(uuid >> 96));
        result.data2 = qFromLittleEndian<quint16>(ushort(uuid >> 80));
        result.data3 = qFromLittleEndian<quint16>(ushort(uuid >> 64));
        for (int i = 0; i < 8; ++i)
            result.data4[i] = uchar(uuid >> (56 - i * 8));
    }
    return result;
}

constexpr quint128 QUuid::toUInt128(QSysInfo::Endian order) const noexcept
{
    quint128 result = {};
    if (order == QSysInfo::BigEndian) {
        for (int i = 0; i < 8; ++i)
            result |= quint64(data4[i]) << (i * 8);
        result = result << 64;
        result |= quint64(qToBigEndian<quint16>(data3)) << 48;
        result |= quint64(qToBigEndian<quint16>(data2)) << 32;
        result |= qToBigEndian<quint32>(data1);
    } else {
        result = qToLittleEndian<quint32>(data1);
        result = result << 32;
        result |= quint64(qToLittleEndian<quint16>(data2)) << 16;
        result |= quint64(qToLittleEndian<quint16>(data3));
        result = result << 64;
        for (int i = 0; i < 8; ++i)
            result |= quint64(data4[i]) << (56 - i * 8);
    }
    return result;
}
#endif

#if defined(Q_QDOC)
// provide fake declarations of qXXXEndian() functions, so that qDoc could
// distinguish them from the general template
QUuid::Id128Bytes qFromBigEndian(QUuid::Id128Bytes src);
QUuid::Id128Bytes qFromLittleEndian(QUuid::Id128Bytes src);
QUuid::Id128Bytes qToBigEndian(QUuid::Id128Bytes src);
QUuid::Id128Bytes qToLittleEndian(QUuid::Id128Bytes src);
#endif

QT_END_NAMESPACE

#endif // QUUID_H
