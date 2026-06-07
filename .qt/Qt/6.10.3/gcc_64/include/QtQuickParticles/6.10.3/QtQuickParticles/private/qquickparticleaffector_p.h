// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef PARTICLEAFFECTOR_H
#define PARTICLEAFFECTOR_H

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

#include <QObject>
#include <QSet>
#include "qquickparticlesystem_p.h"
#include "qquickparticleextruder_p.h"
#include "qtquickparticlesglobal_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickParticleAffector : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    Q_PROPERTY(QStringList groups READ groups WRITE setGroups NOTIFY groupsChanged)
    Q_PROPERTY(QStringList whenCollidingWith READ whenCollidingWith WRITE setWhenCollidingWith NOTIFY whenCollidingWithChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool once READ onceOff WRITE setOnceOff NOTIFY onceChanged)
    Q_PROPERTY(QQuickParticleExtruder* shape READ shape WRITE setShape NOTIFY shapeChanged)

    QML_NAMED_ELEMENT(ParticleAffector)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Abstract type. Use one of the inheriting types instead.")

public:
    explicit QQuickParticleAffector(QQuickItem *parent = nullptr);
    virtual void affectSystem(qreal dt);
    virtual void reset(QQuickParticleData*);//As some store their own data per particle?
    QQuickParticleSystem* system() const
    {
        return m_system;
    }

    QStringList groups() const
    {
        return m_groups;
    }

    bool enabled() const
    {
        return m_enabled;
    }

    bool onceOff() const
    {
        return m_onceOff;
    }

    QQuickParticleExtruder* shape() const
    {
        return m_shape;
    }

    QStringList whenCollidingWith() const
    {
        return m_whenCollidingWith;
    }

Q_SIGNALS:

    void systemChanged(QQuickParticleSystem* arg);

    void groupsChanged(const QStringList &arg);

    void enabledChanged(bool arg);

    void onceChanged(bool arg);

    void shapeChanged(QQuickParticleExtruder* arg);

    void affected(qreal x, qreal y);

    void whenCollidingWithChanged(const QStringList &arg);

public Q_SLOTS:
void setSystem(QQuickParticleSystem* arg)
{
    if (m_system != arg) {
        m_system = arg;
        if (m_system)
            m_system->registerParticleAffector(this);
        Q_EMIT systemChanged(arg);
    }
}

void setGroups(const QStringList &arg)
{
    if (m_groups != arg) {
        m_groups = arg;
        m_updateIntSet = true;
        Q_EMIT groupsChanged(arg);
    }
}

void setEnabled(bool arg)
{
    if (m_enabled != arg) {
        m_enabled = arg;
        Q_EMIT enabledChanged(arg);
    }
}

void setOnceOff(bool arg)
{
    if (m_onceOff != arg) {
        m_onceOff = arg;
        m_needsReset = true;
        Q_EMIT onceChanged(arg);
    }
}

void setShape(QQuickParticleExtruder* arg)
{
    if (m_shape != arg) {
        m_shape = arg;
        Q_EMIT shapeChanged(arg);
    }
}

void setWhenCollidingWith(const QStringList &arg)
{
    if (m_whenCollidingWith != arg) {
        m_whenCollidingWith = arg;
        Q_EMIT whenCollidingWithChanged(arg);
    }
}
public Q_SLOTS:
    void updateOffsets();

protected:
    friend class QQuickParticleSystem;
    virtual bool affectParticle(QQuickParticleData *d, qreal dt);
    bool m_needsReset:1;//### What is this really saving?
    bool m_ignoresTime:1;
    bool m_onceOff:1;
    bool m_enabled:1;

    QQuickParticleSystem* m_system;
    QStringList m_groups;
    bool activeGroup(int g);
    bool shouldAffect(QQuickParticleData* datum);//Call to do the logic on whether it is affecting that datum
    void postAffect(QQuickParticleData* datum);//Call to do the post-affect logic on particles which WERE affected(once off, needs reset, affected signal)
    void componentComplete() override;
    bool isAffectedConnected();
    static const qreal simulationDelta;
    static const qreal simulationCutoff;

    QPointF m_offset;
    QSet<std::pair<int, int>> m_onceOffed;
private:
    QSet<int> m_groupIds;
    bool m_updateIntSet;

    QQuickParticleExtruder* m_shape;

    QStringList m_whenCollidingWith;

    bool isColliding(QQuickParticleData* d) const;
};

QT_END_NAMESPACE
#endif // PARTICLEAFFECTOR_H
