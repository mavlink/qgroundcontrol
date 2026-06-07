// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef GROUPGOALAFFECTOR_H
#define GROUPGOALAFFECTOR_H

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

class QQuickStochasticEngine;

class Q_QUICKPARTICLES_EXPORT QQuickGroupGoalAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(QString goalState READ goalState WRITE setGoalState NOTIFY goalStateChanged)
    Q_PROPERTY(bool jump READ jump WRITE setJump NOTIFY jumpChanged)
    QML_NAMED_ELEMENT(GroupGoal)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickGroupGoalAffector(QQuickItem *parent = nullptr);

    QString goalState() const
    {
        return m_goalState;
    }

    bool jump() const
    {
        return m_jump;
    }

protected:
    bool affectParticle(QQuickParticleData *d, qreal dt) override;

Q_SIGNALS:

    void goalStateChanged(const QString &arg);

    void jumpChanged(bool arg);

public Q_SLOTS:

    void setGoalState(const QString &arg);

    void setJump(bool arg)
    {
        if (m_jump != arg) {
            m_jump = arg;
            Q_EMIT jumpChanged(arg);
        }
    }

private:
    QString m_goalState;
    bool m_jump;
};

QT_END_NAMESPACE

#endif // GROUPGOALAFFECTOR_H
