// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef POINTVECTOR_H
#define POINTVECTOR_H

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
#include "qquickdirection_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickPointDirection : public QQuickDirection
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(qreal xVariation READ xVariation WRITE setXVariation NOTIFY xVariationChanged)
    Q_PROPERTY(qreal yVariation READ yVariation WRITE setYVariation NOTIFY yVariationChanged)
    QML_NAMED_ELEMENT(PointDirection)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickPointDirection(QObject *parent = nullptr);
    QPointF sample(const QPointF &from) override;
    qreal x() const
    {
        return m_x;
    }

    qreal y() const
    {
        return m_y;
    }

    qreal xVariation() const
    {
        return m_xVariation;
    }

    qreal yVariation() const
    {
        return m_yVariation;
    }

Q_SIGNALS:

    void xChanged(qreal arg);

    void yChanged(qreal arg);

    void xVariationChanged(qreal arg);

    void yVariationChanged(qreal arg);

public Q_SLOTS:
    void setX(qreal arg)
    {
        if (m_x != arg) {
            m_x = arg;
            Q_EMIT xChanged(arg);
        }
    }

    void setY(qreal arg)
    {
        if (m_y != arg) {
            m_y = arg;
            Q_EMIT yChanged(arg);
        }
    }

    void setXVariation(qreal arg)
    {
        if (m_xVariation != arg) {
            m_xVariation = arg;
            Q_EMIT xVariationChanged(arg);
        }
    }

    void setYVariation(qreal arg)
    {
        if (m_yVariation != arg) {
            m_yVariation = arg;
            Q_EMIT yVariationChanged(arg);
        }
    }

private:

    qreal m_x;
    qreal m_y;
    qreal m_xVariation;
    qreal m_yVariation;
};

QT_END_NAMESPACE
#endif // POINTVECTOR_H
