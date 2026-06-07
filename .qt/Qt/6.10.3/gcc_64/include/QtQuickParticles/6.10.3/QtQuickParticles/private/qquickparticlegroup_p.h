// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQuickPARTICLEGROUP
#define QQuickPARTICLEGROUP

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
#include <private/qquickspriteengine_p.h>
#include "qquickparticlesystem_p.h"
#include "qqmlparserstatus.h"

QT_BEGIN_NAMESPACE

class Q_QUICKPARTICLES_EXPORT QQuickParticleGroup : public QQuickStochasticState,
                                                            public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QQuickParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)

    //Intercept children requests and assign to the group & system
    Q_PROPERTY(QQmlListProperty<QObject> particleChildren READ particleChildren DESIGNABLE false)//### Hidden property for in-state system definitions - ought not to be used in actual "Sprite" states
    Q_CLASSINFO("DefaultProperty", "particleChildren")
    QML_NAMED_ELEMENT(ParticleGroup)
    QML_ADDED_IN_VERSION(2, 0)
    Q_INTERFACES(QQmlParserStatus)

public:
    explicit QQuickParticleGroup(QObject *parent = nullptr);

    QQmlListProperty<QObject> particleChildren();

    QQuickParticleSystem* system() const
    {
        return m_system;
    }

public Q_SLOTS:

    void setSystem(QQuickParticleSystem* arg);

    void delayRedirect(QObject* obj);

Q_SIGNALS:

    void systemChanged(QQuickParticleSystem* arg);

protected:
    void componentComplete() override;
    void classBegin() override {}

private:

    void performDelayedRedirects();

    QQuickParticleSystem* m_system;
    QList<QObject*> m_delayedRedirects;
};

QT_END_NAMESPACE

#endif
