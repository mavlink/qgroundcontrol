// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ITEMPARTICLE_H
#define ITEMPARTICLE_H

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
#include "qquickparticlepainter_p.h"
#include <QPointer>
#include <QSet>
#include <private/qquickanimation_p_p.h>
QT_BEGIN_NAMESPACE

class QQuickItemParticleAttached;

class Q_QUICKPARTICLES_EXPORT QQuickItemParticle : public QQuickParticlePainter
{
    Q_OBJECT
    Q_PROPERTY(bool fade READ fade WRITE setFade NOTIFY fadeChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    QML_NAMED_ELEMENT(ItemParticle)
    QML_ADDED_IN_VERSION(2, 0)
    QML_ATTACHED(QQuickItemParticleAttached)
public:
    explicit QQuickItemParticle(QQuickItem *parent = nullptr);
    ~QQuickItemParticle();

    bool fade() const { return m_fade; }

    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

    static QQuickItemParticleAttached *qmlAttachedProperties(QObject *object);
    QQmlComponent* delegate() const
    {
        return m_delegate;
    }

Q_SIGNALS:
    void fadeChanged();

    void delegateChanged(QQmlComponent* arg);

public Q_SLOTS:
    //TODO: Add a follow mode, where moving the delegate causes the logical particle to go with it?
    void freeze(QQuickItem* item);
    void unfreeze(QQuickItem* item);
    void take(QQuickItem* item,bool prioritize=false);//take by modelparticle
    void give(QQuickItem* item);//give from modelparticle

    void setFade(bool arg){if (arg == m_fade) return; m_fade = arg; Q_EMIT fadeChanged();}
    void setDelegate(QQmlComponent* arg)
    {
        if (m_delegate != arg) {
            m_delegate = arg;
            Q_EMIT delegateChanged(arg);
        }
    }

protected:
    void reset() override;
    void commit(int gIdx, int pIdx) override;
    void initialize(int gIdx, int pIdx) override;
    void prepareNextFrame();
private:
    bool clockShouldUpdate() const;
    void updateClock();
    void reconnectSystem(QQuickParticleSystem *system);
    void reconnectParent(QQuickItem *parent);
    void processDeletables();
    void tick(int time = 0);
    QSet<QQuickItem* > m_deletables;
    QList<QQuickItem* > m_managed;
    bool m_fade;

    QList<QQuickItem*> m_pendingItems;
    QSet<QQuickItem*> m_stasis;
    qreal m_lastT;
    int m_activeCount;
    QQmlComponent* m_delegate;

    typedef QTickAnimationProxy<QQuickItemParticle, &QQuickItemParticle::tick> Clock;
    Clock *clock;
    QMetaObject::Connection m_systemRunStateConnection;
    QMetaObject::Connection m_systemPauseStateConnection;
    QMetaObject::Connection m_systemEnabledStateConnection;
    QMetaObject::Connection m_parentEnabledStateConnection;
};

class QQuickItemParticleAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItemParticle* particle READ particle CONSTANT FINAL);
public:
    QQuickItemParticleAttached(QObject* parent)
        : QObject(parent), m_mp(0), m_parentItem(nullptr)
    {;}
    QQuickItemParticle* particle() const { return m_mp; }
    void detach(){Q_EMIT detached();}
    void attach(){Q_EMIT attached();}
private:
    QQuickItemParticle* m_mp;
    QPointer<QQuickItem> m_parentItem;
    friend class QQuickItemParticle;
Q_SIGNALS:
    void detached();
    void attached();
};

QT_END_NAMESPACE

#endif // ITEMPARTICLE_H
