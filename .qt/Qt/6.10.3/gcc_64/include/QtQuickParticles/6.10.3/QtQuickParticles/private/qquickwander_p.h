// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef WANDERAFFECTOR_H
#define WANDERAFFECTOR_H

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
#include <QHash>
#include "qquickparticleaffector_p.h"

QT_BEGIN_NAMESPACE

struct WanderData{
    qreal x_vel;
    qreal y_vel;
    qreal x_peak;
    qreal x_var;
    qreal y_peak;
    qreal y_var;
};

class Q_QUICKPARTICLES_EXPORT QQuickWanderAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal pace READ pace WRITE setPace NOTIFY paceChanged)
    Q_PROPERTY(qreal xVariance READ xVariance WRITE setXVariance NOTIFY xVarianceChanged)
    Q_PROPERTY(qreal yVariance READ yVariance WRITE setYVariance NOTIFY yVarianceChanged)
    Q_PROPERTY(AffectableParameters affectedParameter READ affectedParameter WRITE setAffectedParameter NOTIFY affectedParameterChanged)
    QML_NAMED_ELEMENT(Wander)
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum AffectableParameters {
        Position,
        Velocity,
        Acceleration
    };
    Q_ENUM(AffectableParameters)

    explicit QQuickWanderAffector(QQuickItem *parent = nullptr);
    ~QQuickWanderAffector();
//    virtual void reset(int systemIdx);

    qreal xVariance() const
    {
        return m_xVariance;
    }

    qreal yVariance() const
    {
        return m_yVariance;
    }

    qreal pace() const
    {
        return m_pace;
    }

    AffectableParameters affectedParameter() const
    {
        return m_affectedParameter;
    }

protected:
    bool affectParticle(QQuickParticleData *d, qreal dt) override;

Q_SIGNALS:

    void xVarianceChanged(qreal arg);

    void yVarianceChanged(qreal arg);

    void paceChanged(qreal arg);


    void affectedParameterChanged(AffectableParameters arg);

public Q_SLOTS:
void setXVariance(qreal arg)
{
    if (m_xVariance != arg) {
        m_xVariance = arg;
        Q_EMIT xVarianceChanged(arg);
    }
}

void setYVariance(qreal arg)
{
    if (m_yVariance != arg) {
        m_yVariance = arg;
        Q_EMIT yVarianceChanged(arg);
    }
}

void setPace(qreal arg)
{
    if (m_pace != arg) {
        m_pace = arg;
        Q_EMIT paceChanged(arg);
    }
}


void setAffectedParameter(AffectableParameters arg)
{
    if (m_affectedParameter != arg) {
        m_affectedParameter = arg;
        Q_EMIT affectedParameterChanged(arg);
    }
}

private:
    WanderData* getData(int idx);
    QHash<int, WanderData*> m_wanderData;
    qreal m_xVariance;
    qreal m_yVariance;
    qreal m_pace;
    AffectableParameters m_affectedParameter;
};

QT_END_NAMESPACE
#endif // WANDERAFFECTOR_H
