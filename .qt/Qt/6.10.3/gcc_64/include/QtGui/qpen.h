// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPEN_H
#define QPEN_H

#include <QtCore/qshareddata.h>
#include <QtGui/qtguiglobal.h>
#include <QtGui/qcolor.h>
#include <QtGui/qbrush.h>

QT_BEGIN_NAMESPACE


class QVariant;
class QPenPrivate;
class QBrush;
class QPen;

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QPen &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPen &);
#endif

QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QPenPrivate, Q_GUI_EXPORT)

class Q_GUI_EXPORT QPen
{
public:
    QPen();
    QPen(Qt::PenStyle);
    QPen(const QColor &color);
    QPen(const QBrush &brush, qreal width, Qt::PenStyle s = Qt::SolidLine,
         Qt::PenCapStyle c = Qt::SquareCap, Qt::PenJoinStyle j = Qt::BevelJoin);
    QPen(const QPen &pen) noexcept;

    ~QPen();

    QPen &operator=(const QPen &pen) noexcept;
    QPen(QPen &&other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QPen)
    void swap(QPen &other) noexcept { d.swap(other.d); }

    QPen &operator=(QColor color);
    QPen &operator=(Qt::PenStyle style);

    Qt::PenStyle style() const;
    void setStyle(Qt::PenStyle);

    QList<qreal> dashPattern() const;
    void setDashPattern(const QList<qreal> &pattern);

    qreal dashOffset() const;
    void setDashOffset(qreal doffset);

    qreal miterLimit() const;
    void setMiterLimit(qreal limit);

    qreal widthF() const;
    void setWidthF(qreal width);

    int width() const;
    void setWidth(int width);

    QColor color() const;
    void setColor(const QColor &color);

    QBrush brush() const;
    void setBrush(const QBrush &brush);

    bool isSolid() const;

    Qt::PenCapStyle capStyle() const;
    void setCapStyle(Qt::PenCapStyle pcs);

    Qt::PenJoinStyle joinStyle() const;
    void setJoinStyle(Qt::PenJoinStyle pcs);

    bool isCosmetic() const;
    void setCosmetic(bool cosmetic);

    bool operator==(const QPen &p) const;
    inline bool operator!=(const QPen &p) const { return !(operator==(p)); }
    operator QVariant() const;

    bool isDetached();

private:
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QPen &);
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QPen &);

    bool isSolidDefaultLine() const noexcept;

    bool doCompareEqualColor(QColor rhs) const noexcept;
    friend bool comparesEqual(const QPen &lhs, QColor rhs) noexcept
    {
        return lhs.doCompareEqualColor(rhs);
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QPen, QColor)

    bool doCompareEqualStyle(Qt::PenStyle rhs) const;
    friend bool comparesEqual(const QPen &lhs, Qt::PenStyle rhs)
    {
        return lhs.doCompareEqualStyle(rhs);
    }
    Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(QPen, Qt::PenStyle)

public:
    using DataPtr = QExplicitlySharedDataPointer<QPenPrivate>;

private:
    void detach();
    DataPtr d;

public:
    inline DataPtr &data_ptr() { return d; }
};

Q_DECLARE_SHARED(QPen)

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QPen &);
#endif

QT_END_NAMESPACE

#endif // QPEN_H
