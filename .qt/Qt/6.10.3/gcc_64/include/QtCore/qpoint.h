// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPOINT_H
#define QPOINT_H

#include <QtCore/qcheckedint_impl.h>
#include <QtCore/qcompare.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qnumeric.h>

#include <QtCore/q20type_traits.h>
#include <QtCore/q23utility.h>

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
struct CGPoint;
#endif

QT_BEGIN_NAMESPACE

QT_ENABLE_P0846_SEMANTICS_FOR(get)

class QDataStream;
class QLine;
class QPointF;
class QRect;

class QPoint
{
public:
    constexpr QPoint() noexcept;
    constexpr QPoint(int xpos, int ypos) noexcept;

    constexpr inline bool isNull() const noexcept;

    constexpr inline int x() const noexcept;
    constexpr inline int y() const noexcept;
    constexpr inline void setX(int x) noexcept;
    constexpr inline void setY(int y) noexcept;

    constexpr inline int manhattanLength() const;

    constexpr QPoint transposed() const noexcept { return {yp, xp}; }

    constexpr inline int &rx() noexcept;
    constexpr inline int &ry() noexcept;

    constexpr inline QPoint &operator+=(const QPoint &p);
    constexpr inline QPoint &operator-=(const QPoint &p);

    constexpr inline QPoint &operator*=(float factor);
    constexpr inline QPoint &operator*=(double factor);
    constexpr inline QPoint &operator*=(int factor);

    constexpr inline QPoint &operator/=(qreal divisor);

    constexpr static inline int dotProduct(const QPoint &p1, const QPoint &p2)
    { return int(p1.xp * p2.xp + p1.yp * p2.yp); }

private:
    friend constexpr bool comparesEqual(const QPoint &p1, const QPoint &p2) noexcept
    { return p1.xp == p2.xp && p1.yp == p2.yp; }
    Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(QPoint)
    friend constexpr inline QPoint operator+(const QPoint &p1, const QPoint &p2) noexcept
    { return QPoint(p1.xp + p2.xp, p1.yp + p2.yp); }
    friend constexpr inline QPoint operator-(const QPoint &p1, const QPoint &p2) noexcept
    { return QPoint(p1.xp - p2.xp, p1.yp - p2.yp); }
    friend constexpr inline QPoint operator*(const QPoint &p, float factor)
    { return QPoint(QtPrivate::qSaturateRound(p.x() * factor), QtPrivate::qSaturateRound(p.y() * factor)); }
    friend constexpr inline QPoint operator*(const QPoint &p, double factor)
    { return QPoint(QtPrivate::qSaturateRound(p.x() * factor), QtPrivate::qSaturateRound(p.y() * factor)); }
    friend constexpr inline QPoint operator*(const QPoint &p, int factor) noexcept
    { return QPoint(p.xp * factor, p.yp * factor); }
    friend constexpr inline QPoint operator*(float factor, const QPoint &p)
    { return QPoint(QtPrivate::qSaturateRound(p.x() * factor), QtPrivate::qSaturateRound(p.y() * factor)); }
    friend constexpr inline QPoint operator*(double factor, const QPoint &p)
    { return QPoint(QtPrivate::qSaturateRound(p.x() * factor), QtPrivate::qSaturateRound(p.y() * factor)); }
    friend constexpr inline QPoint operator*(int factor, const QPoint &p) noexcept
    { return QPoint(p.xp * factor, p.yp * factor); }
    friend constexpr inline QPoint operator+(const QPoint &p) noexcept
    { return p; }
    friend constexpr inline QPoint operator-(const QPoint &p) noexcept
    { return QPoint(-p.xp, -p.yp); }
    friend constexpr inline QPoint operator/(const QPoint &p, qreal c)
    {
        Q_ASSERT(!qFuzzyIsNull(c));
        return QPoint(QtPrivate::qSaturateRound(p.x() / c), QtPrivate::qSaturateRound(p.y() / c));
    }

public:
#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    [[nodiscard]] Q_CORE_EXPORT CGPoint toCGPoint() const noexcept;
#endif
    [[nodiscard]] constexpr inline QPointF toPointF() const noexcept;

private:
    using Representation = QtPrivate::QCheckedIntegers::QCheckedInt<int>;

    friend class QRect;
    friend class QLine;
    constexpr QPoint(Representation xpos, Representation ypos) noexcept
        : xp(xpos), yp(ypos) {}

    Representation xp;
    Representation yp;

    template <std::size_t I,
              typename P,
              std::enable_if_t<(I < 2), bool> = true,
              std::enable_if_t<std::is_same_v<q20::remove_cvref_t<P>, QPoint>, bool> = true>
    friend constexpr decltype(auto) get(P &&p) noexcept
    {
        if constexpr (I == 0)
            return q23::forward_like<P>(p.xp).as_underlying();
        else if constexpr (I == 1)
            return q23::forward_like<P>(p.yp).as_underlying();
    }
};

Q_DECLARE_TYPEINFO(QPoint, Q_PRIMITIVE_TYPE);

/*****************************************************************************
  QPoint stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QPoint &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QPoint &);
#endif

/*****************************************************************************
  QPoint inline functions
 *****************************************************************************/

constexpr inline QPoint::QPoint() noexcept : xp(0), yp(0) {}

constexpr inline QPoint::QPoint(int xpos, int ypos) noexcept : xp(xpos), yp(ypos) {}

constexpr inline bool QPoint::isNull() const noexcept
{
    return xp == 0 && yp == 0;
}

constexpr inline int QPoint::x() const noexcept
{
    return xp.value();
}

constexpr inline int QPoint::y() const noexcept
{
    return yp.value();
}

constexpr inline void QPoint::setX(int xpos) noexcept
{
    xp.setValue(xpos);
}

constexpr inline void QPoint::setY(int ypos) noexcept
{
    yp.setValue(ypos);
}

inline int constexpr QPoint::manhattanLength() const
{
    return (qAbs(xp) + qAbs(yp)).value();
}

constexpr inline int &QPoint::rx() noexcept
{
    return xp.as_underlying();
}

constexpr inline int &QPoint::ry() noexcept
{
    return yp.as_underlying();
}

constexpr inline QPoint &QPoint::operator+=(const QPoint &p)
{
    xp += p.xp;
    yp += p.yp;
    return *this;
}

constexpr inline QPoint &QPoint::operator-=(const QPoint &p)
{
    xp -= p.xp;
    yp -= p.yp;
    return *this;
}

constexpr inline QPoint &QPoint::operator*=(float factor)
{
    xp.setValue(QtPrivate::qSaturateRound(x() * factor));
    yp.setValue(QtPrivate::qSaturateRound(y() * factor));
    return *this;
}

constexpr inline QPoint &QPoint::operator*=(double factor)
{
    xp.setValue(QtPrivate::qSaturateRound(x() * factor));
    yp.setValue(QtPrivate::qSaturateRound(y() * factor));
    return *this;
}

constexpr inline QPoint &QPoint::operator*=(int factor)
{
    xp = xp * factor;
    yp = yp * factor;
    return *this;
}

constexpr inline QPoint &QPoint::operator/=(qreal c)
{
    Q_ASSERT(!qFuzzyIsNull(c));
    xp.setValue(qRound(int(xp) / c));
    yp.setValue(qRound(int(yp) / c));
    return *this;
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QPoint &);
#endif

Q_CORE_EXPORT size_t qHash(QPoint key, size_t seed = 0) noexcept;




class QPointF
{
public:
    constexpr QPointF() noexcept;
    constexpr QPointF(const QPoint &p) noexcept;
    constexpr QPointF(qreal xpos, qreal ypos) noexcept;

    constexpr inline qreal manhattanLength() const;

    inline bool isNull() const noexcept;

    constexpr inline qreal x() const noexcept;
    constexpr inline qreal y() const noexcept;
    constexpr inline void setX(qreal x) noexcept;
    constexpr inline void setY(qreal y) noexcept;

    constexpr QPointF transposed() const noexcept { return {yp, xp}; }

    constexpr inline qreal &rx() noexcept;
    constexpr inline qreal &ry() noexcept;

    constexpr inline QPointF &operator+=(const QPointF &p);
    constexpr inline QPointF &operator-=(const QPointF &p);
    constexpr inline QPointF &operator*=(qreal c);
    constexpr inline QPointF &operator/=(qreal c);

    constexpr static inline qreal dotProduct(const QPointF &p1, const QPointF &p2)
    {
        return p1.xp * p2.xp + p1.yp * p2.yp;
    }

private:
    friend constexpr bool qFuzzyCompare(const QPointF &p1, const QPointF &p2) noexcept
    {
        return QtPrivate::fuzzyCompare(p1.xp, p2.xp)
            && QtPrivate::fuzzyCompare(p1.yp, p2.yp);
    }
    friend constexpr bool qFuzzyIsNull(const QPointF &point) noexcept
    {
        return qFuzzyIsNull(point.xp) && qFuzzyIsNull(point.yp);
    }
    friend constexpr bool comparesEqual(const QPointF &p1, const QPointF &p2) noexcept
    { return qFuzzyCompare(p1, p2); }
    Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(QPointF)
    friend constexpr bool comparesEqual(const QPointF &p1, const QPoint &p2) noexcept
    { return comparesEqual(p1, p2.toPointF()); }
    Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(QPointF, QPoint)
    friend constexpr inline QPointF operator+(const QPointF &p1, const QPointF &p2)
    { return QPointF(p1.xp + p2.xp, p1.yp + p2.yp); }
    friend constexpr inline QPointF operator-(const QPointF &p1, const QPointF &p2)
    { return QPointF(p1.xp - p2.xp, p1.yp - p2.yp); }
    friend constexpr inline QPointF operator*(const QPointF &p, qreal c)
    { return QPointF(p.xp * c, p.yp * c); }
    friend constexpr inline QPointF operator*(qreal c, const QPointF &p)
    { return QPointF(p.xp * c, p.yp * c); }
    friend constexpr inline QPointF operator+(const QPointF &p)
    { return p; }
    friend constexpr inline QPointF operator-(const QPointF &p)
    { return QPointF(-p.xp, -p.yp); }
    friend constexpr inline QPointF operator/(const QPointF &p, qreal divisor)
    {
        Q_ASSERT(divisor < 0 || divisor > 0);
        return QPointF(p.xp / divisor, p.yp / divisor);
    }

public:
    constexpr QPoint toPoint() const;

#if defined(Q_OS_DARWIN) || defined(Q_QDOC)
    [[nodiscard]] Q_CORE_EXPORT static QPointF fromCGPoint(CGPoint point) noexcept;
    [[nodiscard]] Q_CORE_EXPORT CGPoint toCGPoint() const noexcept;
#endif

private:
    friend class QTransform;

    qreal xp;
    qreal yp;

    template <std::size_t I,
              typename P,
              std::enable_if_t<(I < 2), bool> = true,
              std::enable_if_t<std::is_same_v<q20::remove_cvref_t<P>, QPointF>, bool> = true>
    friend constexpr decltype(auto) get(P &&p) noexcept
    {
        if constexpr (I == 0)
            return q23::forward_like<P>(p.xp);
        else if constexpr (I == 1)
            return q23::forward_like<P>(p.yp);
    }
};

Q_DECLARE_TYPEINFO(QPointF, Q_PRIMITIVE_TYPE);

size_t qHash(QPointF, size_t seed = 0) = delete;

/*****************************************************************************
  QPointF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QPointF &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QPointF &);
#endif

/*****************************************************************************
  QPointF inline functions
 *****************************************************************************/

constexpr inline QPointF::QPointF() noexcept : xp(0), yp(0) { }

constexpr inline QPointF::QPointF(qreal xpos, qreal ypos) noexcept : xp(xpos), yp(ypos) { }

constexpr inline QPointF::QPointF(const QPoint &p) noexcept : xp(p.x()), yp(p.y()) { }

constexpr inline qreal QPointF::manhattanLength() const
{
    return qAbs(x()) + qAbs(y());
}

inline bool QPointF::isNull() const noexcept
{
    return qIsNull(xp) && qIsNull(yp);
}

constexpr inline qreal QPointF::x() const noexcept
{
    return xp;
}

constexpr inline qreal QPointF::y() const noexcept
{
    return yp;
}

constexpr inline void QPointF::setX(qreal xpos) noexcept
{
    xp = xpos;
}

constexpr inline void QPointF::setY(qreal ypos) noexcept
{
    yp = ypos;
}

constexpr inline qreal &QPointF::rx() noexcept
{
    return xp;
}

constexpr inline qreal &QPointF::ry() noexcept
{
    return yp;
}

constexpr inline QPointF &QPointF::operator+=(const QPointF &p)
{
    xp += p.xp;
    yp += p.yp;
    return *this;
}

constexpr inline QPointF &QPointF::operator-=(const QPointF &p)
{
    xp -= p.xp;
    yp -= p.yp;
    return *this;
}

constexpr inline QPointF &QPointF::operator*=(qreal c)
{
    xp *= c;
    yp *= c;
    return *this;
}

constexpr inline QPointF &QPointF::operator/=(qreal divisor)
{
    Q_ASSERT(divisor > 0 || divisor < 0);
    xp /= divisor;
    yp /= divisor;
    return *this;
}

constexpr QPointF QPoint::toPointF() const noexcept { return *this; }

constexpr inline QPoint QPointF::toPoint() const
{
    return QPoint(QtPrivate::qSaturateRound(xp), QtPrivate::qSaturateRound(yp));
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug d, const QPointF &p);
#endif

QT_END_NAMESPACE

/*****************************************************************************
  QPoint/QPointF tuple protocol
 *****************************************************************************/

namespace std {
    template <>
    class tuple_size<QT_PREPEND_NAMESPACE(QPoint)> : public integral_constant<size_t, 2> {};
    template <>
    class tuple_element<0, QT_PREPEND_NAMESPACE(QPoint)> { public: using type = int; };
    template <>
    class tuple_element<1, QT_PREPEND_NAMESPACE(QPoint)> { public: using type = int; };

    template <>
    class tuple_size<QT_PREPEND_NAMESPACE(QPointF)> : public integral_constant<size_t, 2> {};
    template <>
    class tuple_element<0, QT_PREPEND_NAMESPACE(QPointF)> { public: using type = QT_PREPEND_NAMESPACE(qreal); };
    template <>
    class tuple_element<1, QT_PREPEND_NAMESPACE(QPointF)> { public: using type = QT_PREPEND_NAMESPACE(qreal); };
}

#endif // QPOINT_H
