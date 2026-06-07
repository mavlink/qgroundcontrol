// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// Copyright (C) 2015 Keith Gardner <kreios4004@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QTYPEREVISION_H
#define QTYPEREVISION_H

#include <QtCore/qassert.h>
#include <QtCore/qcompare.h>
#include <QtCore/qcontainertools_impl.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qtypeinfo.h>

#include <limits>

QT_BEGIN_NAMESPACE

class QDataStream;
class QDebug;

class QTypeRevision;
Q_CORE_EXPORT size_t qHash(const QTypeRevision &key, size_t seed = 0);

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream& operator<<(QDataStream &out, const QTypeRevision &revision);
Q_CORE_EXPORT QDataStream& operator>>(QDataStream &in, QTypeRevision &revision);
#endif

class QTypeRevision
{
public:
    template<typename Integer>
    using if_valid_segment_type = typename std::enable_if<
            std::is_integral<Integer>::value, bool>::type;

    template<typename Integer>
    using if_valid_value_type = typename std::enable_if<
            std::is_integral<Integer>::value
            && (sizeof(Integer) > sizeof(quint16)
                || (sizeof(Integer) == sizeof(quint16)
                    && !std::is_signed<Integer>::value)), bool>::type;

    template<typename Integer, if_valid_segment_type<Integer> = true>
    static constexpr bool isValidSegment(Integer segment)
    {
        // using extra parentheses around max to avoid expanding it if it is a macro
        // and adding zero to cause it to be promoted
        constexpr auto Max = (std::numeric_limits<Integer>::max)() + 0;
        constexpr bool HasSufficientRange = Max >= SegmentUnknown;
        return segment >= Integer(0)
                && (!HasSufficientRange || segment < Integer(SegmentUnknown));
    }

    template<typename Major, typename Minor,
             if_valid_segment_type<Major> = true,
             if_valid_segment_type<Minor> = true>
    static constexpr QTypeRevision fromVersion(Major majorVersion, Minor minorVersion)
    {
        return Q_ASSERT(isValidSegment(majorVersion)),
               Q_ASSERT(isValidSegment(minorVersion)),
               QTypeRevision(quint8(majorVersion), quint8(minorVersion));
    }

    template<typename Major, if_valid_segment_type<Major> = true>
    static constexpr QTypeRevision fromMajorVersion(Major majorVersion)
    {
        return Q_ASSERT(isValidSegment(majorVersion)),
               QTypeRevision(quint8(majorVersion), SegmentUnknown);
    }

    template<typename Minor, if_valid_segment_type<Minor> = true>
    static constexpr QTypeRevision fromMinorVersion(Minor minorVersion)
    {
        return Q_ASSERT(isValidSegment(minorVersion)),
               QTypeRevision(SegmentUnknown, quint8(minorVersion));
    }

    template<typename Integer, if_valid_value_type<Integer> = true>
    static constexpr QTypeRevision fromEncodedVersion(Integer value)
    {
        return Q_ASSERT((value & ~Integer(0xffff)) == Integer(0)),
               QTypeRevision((value & Integer(0xff00)) >> 8, value & Integer(0xff));
    }

    static constexpr QTypeRevision zero() { return QTypeRevision(0, 0); }

    constexpr QTypeRevision() = default;

    constexpr bool hasMajorVersion() const { return m_majorVersion != SegmentUnknown; }
    constexpr quint8 majorVersion() const { return m_majorVersion; }

    constexpr bool hasMinorVersion() const { return m_minorVersion != SegmentUnknown; }
    constexpr quint8 minorVersion() const { return m_minorVersion; }

    constexpr bool isValid() const { return hasMajorVersion() || hasMinorVersion(); }

    template<typename Integer, if_valid_value_type<Integer> = true>
    constexpr Integer toEncodedVersion() const
    {
        return Integer(m_majorVersion << 8) | Integer(m_minorVersion);
    }

private:
    friend constexpr bool
    comparesEqual(const QTypeRevision &lhs, const QTypeRevision &rhs) noexcept
    { return lhs.toEncodedVersion<quint16>() == rhs.toEncodedVersion<quint16>(); }
    friend constexpr Qt::strong_ordering
    compareThreeWay(const QTypeRevision &lhs, const QTypeRevision &rhs) noexcept
    {
        // For both major and minor the following rule applies:
        // non-0 ver > unspecified ver > 0 ver
        auto cmpUnspecified = [](quint8 leftVer, quint8 rightVer) {
            Q_ASSERT(leftVer != rightVer
                     && (leftVer == QTypeRevision::SegmentUnknown
                         || rightVer == QTypeRevision::SegmentUnknown));
            if (leftVer != QTypeRevision::SegmentUnknown)
                return leftVer > 0 ? Qt::strong_ordering::greater : Qt::strong_ordering::less;
            return rightVer > 0 ? Qt::strong_ordering::less : Qt::strong_ordering::greater;
        };

        if (lhs.hasMajorVersion() != rhs.hasMajorVersion()) {
            return cmpUnspecified(lhs.majorVersion(), rhs.majorVersion());
        } else {
            const auto majorRes = Qt::compareThreeWay(lhs.majorVersion(), rhs.majorVersion());
            if (is_eq(majorRes)) {
                if (lhs.hasMinorVersion() != rhs.hasMinorVersion())
                    return cmpUnspecified(lhs.minorVersion(), rhs.minorVersion());
                return Qt::compareThreeWay(lhs.minorVersion(), rhs.minorVersion());
            }
            return majorRes;
        }
    }
    Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(QTypeRevision)

    enum { SegmentUnknown = 0xff };

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    constexpr QTypeRevision(quint8 major, quint8 minor)
        : m_minorVersion(minor), m_majorVersion(major) {}

    quint8 m_minorVersion = SegmentUnknown;
    quint8 m_majorVersion = SegmentUnknown;
#else
    constexpr QTypeRevision(quint8 major, quint8 minor)
        : m_majorVersion(major), m_minorVersion(minor) {}

    quint8 m_majorVersion = SegmentUnknown;
    quint8 m_minorVersion = SegmentUnknown;
#endif
};

static_assert(sizeof(QTypeRevision) == 2);
Q_DECLARE_TYPEINFO(QTypeRevision, Q_RELOCATABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QTypeRevision &revision);
#endif

QT_END_NAMESPACE

QT_DECL_METATYPE_EXTERN(QTypeRevision, Q_CORE_EXPORT)

#endif // QTYPEREVISION_H

#if !defined(QT_LEAN_HEADERS) || QT_LEAN_HEADERS < 2
// make QVersionNumber available from <QTypeRevision>
#include <QtCore/qversionnumber.h>
#endif
