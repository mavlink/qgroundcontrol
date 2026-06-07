// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef SPRITEGOALAFFECTOR_H
#define SPRITEGOALAFFECTOR_H

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
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

class QQuickStochasticEngine;

class Q_QUICKPARTICLES_EXPORT QQuickSpriteGoalAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(QString goalState READ goalState WRITE setGoalState NOTIFY goalStateChanged)
    Q_PROPERTY(bool jump READ jump WRITE setJump NOTIFY jumpChanged)
    Q_PROPERTY(bool systemStates READ systemStates WRITE setSystemStates NOTIFY systemStatesChanged)
    QML_NAMED_ELEMENT(SpriteGoal)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickSpriteGoalAffector(QQuickItem *parent = nullptr);

    QString goalState() const
    {
        return m_goalState;
    }

    bool jump() const
    {
        return m_jump;
    }
    bool systemStates() const
    {
        return m_systemStates;
    }

protected:
    bool affectParticle(QQuickParticleData *d, qreal dt) override;

Q_SIGNALS:

    void goalStateChanged(const QString &arg);

    void jumpChanged(bool arg);

    void systemStatesChanged(bool arg);

public Q_SLOTS:

void setGoalState(const QString &arg);

void setJump(bool arg)
{
    if (m_jump != arg) {
        m_jump = arg;
        Q_EMIT jumpChanged(arg);
    }
}

void setSystemStates(bool arg)
{
    if (m_systemStates != arg) {
        //TODO: GroupGoal was added (and this deprecated) Oct 4 - remove it in a few weeks.
        qmlWarning(this) << "systemStates is deprecated and will be removed soon. Use GroupGoal instead.";
        m_systemStates = arg;
        Q_EMIT systemStatesChanged(arg);
    }
}

private:
    void updateStateIndex(QQuickStochasticEngine* e);
    QString m_goalState;
    int m_goalIdx;
    QQuickStochasticEngine* m_lastEngine;
    bool m_jump;
    bool m_systemStates;

    bool m_notUsingEngine;
};

QT_END_NAMESPACE

#endif // SPRITEGOALAFFECTOR_H
