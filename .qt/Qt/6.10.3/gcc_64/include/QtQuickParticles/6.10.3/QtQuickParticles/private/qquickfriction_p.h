// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef FRICTIONAFFECTOR_H
#define FRICTIONAFFECTOR_H

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

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickFrictionAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal factor READ factor WRITE setFactor NOTIFY factorChanged)
    Q_PROPERTY(qreal threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged)
    QML_NAMED_ELEMENT(Friction)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickFrictionAffector(QQuickItem *parent = nullptr);

    qreal factor() const
    {
        return m_factor;
    }

    qreal threshold() const
    {
        return m_threshold;
    }

protected:
    bool affectParticle(QQuickParticleData *d, qreal dt) override;

Q_SIGNALS:

    void factorChanged(qreal arg);
    void thresholdChanged(qreal arg);

public Q_SLOTS:

    void setFactor(qreal arg)
    {
        if (m_factor != arg) {
            m_factor = arg;
            Q_EMIT factorChanged(arg);
        }
    }

    void setThreshold(qreal arg)
    {
        if (m_threshold != arg) {
            m_threshold = arg;
            Q_EMIT thresholdChanged(arg);
        }
    }

private:
    qreal m_factor;
    qreal m_threshold;
};

QT_END_NAMESPACE
#endif // FRICTIONAFFECTOR_H
