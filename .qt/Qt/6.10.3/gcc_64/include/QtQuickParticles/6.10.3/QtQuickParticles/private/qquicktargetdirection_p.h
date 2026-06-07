// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef DIRECTEDVECTOR_H
#define DIRECTEDVECTOR_H

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
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class Q_QUICKPARTICLES_EXPORT QQuickTargetDirection : public QQuickDirection
{
    Q_OBJECT
    Q_PROPERTY(qreal targetX READ targetX WRITE setTargetX NOTIFY targetXChanged)
    Q_PROPERTY(qreal targetY READ targetY WRITE setTargetY NOTIFY targetYChanged)
    //If targetItem is set, X/Y are ignored. Aims at middle of item, use variation for variation
    Q_PROPERTY(QQuickItem* targetItem READ targetItem WRITE setTargetItem NOTIFY targetItemChanged)

    Q_PROPERTY(qreal targetVariation READ targetVariation WRITE setTargetVariation NOTIFY targetVariationChanged)

    //TODO: An enum would be better
    Q_PROPERTY(bool proportionalMagnitude READ proportionalMagnitude WRITE setProportionalMagnitude NOTIFY proprotionalMagnitudeChanged)
    Q_PROPERTY(qreal magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal magnitudeVariation READ magnitudeVariation WRITE setMagnitudeVariation NOTIFY magnitudeVariationChanged)
    QML_NAMED_ELEMENT(TargetDirection)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickTargetDirection(QObject *parent = nullptr);
    QPointF sample(const QPointF &from) override;

    qreal targetX() const
    {
        return m_targetX;
    }

    qreal targetY() const
    {
        return m_targetY;
    }

    qreal targetVariation() const
    {
        return m_targetVariation;
    }

    qreal magnitude() const
    {
        return m_magnitude;
    }

    bool proportionalMagnitude() const
    {
        return m_proportionalMagnitude;
    }

    qreal magnitudeVariation() const
    {
        return m_magnitudeVariation;
    }

    QQuickItem* targetItem() const
    {
        return m_targetItem;
    }

Q_SIGNALS:

    void targetXChanged(qreal arg);

    void targetYChanged(qreal arg);

    void targetVariationChanged(qreal arg);

    void magnitudeChanged(qreal arg);

    void proprotionalMagnitudeChanged(bool arg);

    void magnitudeVariationChanged(qreal arg);

    void targetItemChanged(QQuickItem* arg);

public Q_SLOTS:
    void setTargetX(qreal arg)
    {
        if (m_targetX != arg) {
            m_targetX = arg;
            Q_EMIT targetXChanged(arg);
        }
    }

    void setTargetY(qreal arg)
    {
        if (m_targetY != arg) {
            m_targetY = arg;
            Q_EMIT targetYChanged(arg);
        }
    }

    void setTargetVariation(qreal arg)
    {
        if (m_targetVariation != arg) {
            m_targetVariation = arg;
            Q_EMIT targetVariationChanged(arg);
        }
    }

    void setMagnitude(qreal arg)
    {
        if (m_magnitude != arg) {
            m_magnitude = arg;
            Q_EMIT magnitudeChanged(arg);
        }
    }

    void setProportionalMagnitude(bool arg)
    {
        if (m_proportionalMagnitude != arg) {
            m_proportionalMagnitude = arg;
            Q_EMIT proprotionalMagnitudeChanged(arg);
        }
    }

    void setMagnitudeVariation(qreal arg)
    {
        if (m_magnitudeVariation != arg) {
            m_magnitudeVariation = arg;
            Q_EMIT magnitudeVariationChanged(arg);
        }
    }

    void setTargetItem(QQuickItem* arg)
    {
        if (m_targetItem != arg) {
            m_targetItem = arg;
            Q_EMIT targetItemChanged(arg);
        }
    }

private:
    qreal m_targetX;
    qreal m_targetY;
    qreal m_targetVariation;
    bool m_proportionalMagnitude;
    qreal m_magnitude;
    qreal m_magnitudeVariation;
    QQuickItem *m_targetItem;
};

QT_END_NAMESPACE
#endif // DIRECTEDVECTOR_H
