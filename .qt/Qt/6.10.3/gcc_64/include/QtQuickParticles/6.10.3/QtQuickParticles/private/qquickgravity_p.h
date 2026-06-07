// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef GRAVITYAFFECTOR_H
#define GRAVITYAFFECTOR_H

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
#include "qquickparticleaffector_p.h"
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickGravityAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal acceleration READ magnitude WRITE setAcceleration NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    QML_NAMED_ELEMENT(Gravity)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickGravityAffector(QQuickItem *parent = nullptr);
    qreal magnitude() const;
    qreal angle() const;

protected:
    bool affectParticle(QQuickParticleData *d, qreal dt) override;

Q_SIGNALS:
    void magnitudeChanged(qreal arg);
    void angleChanged(qreal arg);

public Q_SLOTS:
    void setMagnitude(qreal arg);
    void setAcceleration(qreal arg);
    void setAngle(qreal arg);

private:
    qreal m_magnitude;
    qreal m_angle;

    bool m_needRecalc;
    qreal m_dx;
    qreal m_dy;
};

QT_END_NAMESPACE
#endif // GRAVITYAFFECTOR_H
