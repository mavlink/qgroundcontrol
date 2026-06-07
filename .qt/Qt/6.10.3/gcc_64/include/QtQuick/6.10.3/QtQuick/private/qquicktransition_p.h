// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTRANSITION_H
#define QQUICKTRANSITION_H

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

#include "qquickstate_p.h"
#include <private/qabstractanimationjob_p.h>
#include <private/qqmlguard_p.h>
#include <qqml.h>

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimation;
class QQuickTransitionPrivate;
class QQuickTransitionManager;
class QQuickTransition;

class QQuickTransitionInstance : QAnimationJobChangeListener
{
public:
    QQuickTransitionInstance(QQuickTransition *transition, QAbstractAnimationJob *anim);
    ~QQuickTransitionInstance();

    void start();
    void stop();
    void complete();

    bool isRunning() const;

protected:
    void animationStateChanged(QAbstractAnimationJob *, QAbstractAnimationJob::State, QAbstractAnimationJob::State) override;

    void removeStateChangeListener()
    {
        m_anim->removeAnimationChangeListener(this, QAbstractAnimationJob::StateChange);
    }

private:
    QQmlGuard<QQuickTransition> m_transition;
    QAbstractAnimationJob *m_anim;
    friend class QQuickTransition;
};

class Q_QUICK_EXPORT QQuickTransition : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickTransition)

    Q_PROPERTY(QString from READ fromState WRITE setFromState NOTIFY fromChanged)
    Q_PROPERTY(QString to READ toState WRITE setToState NOTIFY toChanged)
    Q_PROPERTY(bool reversible READ reversible WRITE setReversible NOTIFY reversibleChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QQmlListProperty<QQuickAbstractAnimation> animations READ animations)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_CLASSINFO("DefaultProperty", "animations")
    Q_CLASSINFO("DeferredPropertyNames", "animations")
    QML_NAMED_ELEMENT(Transition)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickTransition(QObject *parent=nullptr);
    ~QQuickTransition();

    QString fromState() const;
    void setFromState(const QString &);

    QString toState() const;
    void setToState(const QString &);

    bool reversible() const;
    void setReversible(bool);

    bool enabled() const;
    void setEnabled(bool enabled);

    bool running() const;

    QQmlListProperty<QQuickAbstractAnimation> animations();

    QQuickTransitionInstance *prepare(QQuickStateOperation::ActionList &actions,
                 QList<QQmlProperty> &after,
                 QQuickTransitionManager *end,
                 QObject *defaultTarget);

    void setReversed(bool r);

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void reversibleChanged();
    void enabledChanged();
    void runningChanged();
};

QT_END_NAMESPACE

#endif // QQUICKTRANSITION_H
