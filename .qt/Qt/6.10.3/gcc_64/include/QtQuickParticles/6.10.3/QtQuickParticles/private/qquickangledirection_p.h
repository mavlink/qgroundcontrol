// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQuickANGLEDDIRECTION_H
#define QQuickANGLEDDIRECTION_H

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
#include <QtQuickParticles/qtquickparticlesexports.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickAngleDirection : public QQuickDirection
{
    Q_OBJECT
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(qreal magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal angleVariation READ angleVariation WRITE setAngleVariation NOTIFY angleVariationChanged)
    Q_PROPERTY(qreal magnitudeVariation READ magnitudeVariation WRITE setMagnitudeVariation NOTIFY magnitudeVariationChanged)
    QML_NAMED_ELEMENT(AngleDirection)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickAngleDirection(QObject *parent = nullptr);
    QPointF sample(const QPointF &from) override;
    qreal angle() const
    {
        return m_angle;
    }

    qreal magnitude() const
    {
        return m_magnitude;
    }

    qreal angleVariation() const
    {
        return m_angleVariation;
    }

    qreal magnitudeVariation() const
    {
        return m_magnitudeVariation;
    }

Q_SIGNALS:

    void angleChanged(qreal arg);

    void magnitudeChanged(qreal arg);

    void angleVariationChanged(qreal arg);

    void magnitudeVariationChanged(qreal arg);

public Q_SLOTS:
void setAngle(qreal arg)
{
    if (m_angle != arg) {
        m_angle = arg;
        Q_EMIT angleChanged(arg);
    }
}

void setMagnitude(qreal arg)
{
    if (m_magnitude != arg) {
        m_magnitude = arg;
        Q_EMIT magnitudeChanged(arg);
    }
}

void setAngleVariation(qreal arg)
{
    if (m_angleVariation != arg) {
        m_angleVariation = arg;
        Q_EMIT angleVariationChanged(arg);
    }
}

void setMagnitudeVariation(qreal arg)
{
    if (m_magnitudeVariation != arg) {
        m_magnitudeVariation = arg;
        Q_EMIT magnitudeVariationChanged(arg);
    }
}

private:
qreal m_angle;
qreal m_magnitude;
qreal m_angleVariation;
qreal m_magnitudeVariation;
};

QT_END_NAMESPACE
#endif // QQuickANGLEDDIRECTION_H
