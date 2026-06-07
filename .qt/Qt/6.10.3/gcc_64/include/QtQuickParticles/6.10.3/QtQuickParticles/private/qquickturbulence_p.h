// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TURBULENCEAFFECTOR_H
#define TURBULENCEAFFECTOR_H

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
#include <QQmlListProperty>

QT_BEGIN_NAMESPACE

class QQuickParticlePainter;

class Q_QUICKPARTICLES_EXPORT QQuickTurbulenceAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
    Q_PROPERTY(QUrl noiseSource READ noiseSource WRITE setNoiseSource NOTIFY noiseSourceChanged)
    QML_NAMED_ELEMENT(Turbulence)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickTurbulenceAffector(QQuickItem *parent = nullptr);
    ~QQuickTurbulenceAffector();
    void affectSystem(qreal dt) override;

    qreal strength() const
    {
        return m_strength;
    }

    QUrl noiseSource() const
    {
        return m_noiseSource;
    }
Q_SIGNALS:

    void strengthChanged(qreal arg);

    void noiseSourceChanged(const QUrl &arg);

public Q_SLOTS:

    void setStrength(qreal arg)
    {
        if (m_strength != arg) {
            m_strength = arg;
            Q_EMIT strengthChanged(arg);
        }
    }

    void setNoiseSource(const QUrl &arg)
    {
        if (m_noiseSource != arg) {
            m_noiseSource = arg;
            Q_EMIT noiseSourceChanged(arg);
            initializeGrid();
        }
    }

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
private:
    void ensureInit();
    void mapUpdate();
    void initializeGrid();
    qreal boundsRespectingField(int x, int y);
    qreal m_strength;
    int m_gridSize;
    qreal** m_field;
    QPointF** m_vectorField;
    bool m_inited;
    QUrl m_noiseSource;
};

QT_END_NAMESPACE
#endif // TURBULENCEAFFECTOR_H
