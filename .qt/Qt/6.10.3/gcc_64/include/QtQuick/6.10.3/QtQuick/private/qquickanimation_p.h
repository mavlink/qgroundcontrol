// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATION_H
#define QQUICKANIMATION_H

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
#include <QtGui/qvector3d.h>

#include <qqmlpropertyvaluesource.h>
#include <qqml.h>
#include <qqmlscriptstring.h>

#include <QtCore/qvariant.h>
#include <QtCore/qeasingcurve.h>
#include "private/qabstractanimationjob_p.h"
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractAnimationPrivate;
class QQuickAnimationGroup;
class Q_QUICK_EXPORT QQuickAbstractAnimation : public QObject, public QQmlPropertyValueSource, public QQmlParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickAbstractAnimation)

    Q_INTERFACES(QQmlParserStatus)
    Q_INTERFACES(QQmlPropertyValueSource)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool alwaysRunToEnd READ alwaysRunToEnd WRITE setAlwaysRunToEnd NOTIFY alwaysRunToEndChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopCountChanged)
    Q_CLASSINFO("DefaultMethod", "start()")

    QML_NAMED_ELEMENT(Animation)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Animation is an abstract class")

public:
    enum ThreadingModel  {
        GuiThread,
        RenderThread,
        AnyThread
    };

    QQuickAbstractAnimation(QObject *parent=nullptr);
    ~QQuickAbstractAnimation() override;

    enum Loops { Infinite = -2 };
    Q_ENUM(Loops)

    bool isRunning() const;
    void setRunning(bool);
    bool isPaused() const;
    void setPaused(bool);
    bool alwaysRunToEnd() const;
    void setAlwaysRunToEnd(bool);

    int loops() const;
    void setLoops(int);
    int duration() const;

    int currentTime();
    void setCurrentTime(int);

    QQuickAnimationGroup *group() const;
    void setGroup(QQuickAnimationGroup *, int index = -1);

    void setDefaultTarget(const QQmlProperty &);
    void setDisableUserControl();
    void setEnableUserControl();
    bool userControlDisabled() const;
    void classBegin() override;
    void componentComplete() override;

    virtual ThreadingModel threadingModel() const;

Q_SIGNALS:
    void started();
    void stopped();
    void runningChanged(bool);
    void pausedChanged(bool);
    void alwaysRunToEndChanged(bool);
    void loopCountChanged(int);
    Q_REVISION(2, 12) void finished();

public Q_SLOTS:
    void restart();
    void start();
    void pause();
    void resume();
    void stop();
    void complete();

protected:
    QQuickAbstractAnimation(QQuickAbstractAnimationPrivate &dd, QObject *parent);
    QAbstractAnimationJob* initInstance(QAbstractAnimationJob *animation);

public:
    enum TransitionDirection { Forward, Backward };
    virtual QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr);
    QAbstractAnimationJob* qtAnimation();

private:
    void setTarget(const QQmlProperty &) override;
    void notifyRunningChanged(bool running);
    friend class QQuickBehavior;
    friend class QQuickBehaviorPrivate;
    friend class QQuickAnimationGroup;
};

class QQuickPauseAnimationPrivate;
class Q_QUICK_EXPORT QQuickPauseAnimation : public QQuickAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPauseAnimation)

    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    QML_NAMED_ELEMENT(PauseAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickPauseAnimation(QObject *parent=nullptr);
    ~QQuickPauseAnimation() override;

    int duration() const;
    void setDuration(int);

Q_SIGNALS:
    void durationChanged(int);

protected:
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                                          QQmlProperties &modified,
                                          TransitionDirection direction,
                                          QObject *defaultTarget = nullptr) override;
};

class QQuickScriptActionPrivate;
class Q_QUICK_EXPORT QQuickScriptAction : public QQuickAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickScriptAction)

    Q_PROPERTY(QQmlScriptString script READ script WRITE setScript)
    Q_PROPERTY(QString scriptName READ stateChangeScriptName WRITE setStateChangeScriptName)
    QML_NAMED_ELEMENT(ScriptAction)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickScriptAction(QObject *parent=nullptr);
    ~QQuickScriptAction() override;

    QQmlScriptString script() const;
    void setScript(const QQmlScriptString &);

    QString stateChangeScriptName() const;
    void setStateChangeScriptName(const QString &);

protected:
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
};

class QQuickPropertyActionPrivate;
class Q_QUICK_EXPORT QQuickPropertyAction : public QQuickAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPropertyAction)

    Q_PROPERTY(QObject *target READ target WRITE setTargetObject NOTIFY targetChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(QString properties READ properties WRITE setProperties NOTIFY propertiesChanged)
    Q_PROPERTY(QQmlListProperty<QObject> targets READ targets)
    Q_PROPERTY(QQmlListProperty<QObject> exclude READ exclude)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    QML_NAMED_ELEMENT(PropertyAction)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickPropertyAction(QObject *parent=nullptr);
    ~QQuickPropertyAction() override;

    QObject *target() const;
    void setTargetObject(QObject *);

    QString property() const;
    void setProperty(const QString &);

    QString properties() const;
    void setProperties(const QString &);

    QQmlListProperty<QObject> targets();
    QQmlListProperty<QObject> exclude();

    QVariant value() const;
    void setValue(const QVariant &);

Q_SIGNALS:
    void valueChanged(const QVariant &);
    void propertiesChanged(const QString &);
    void targetChanged();
    void propertyChanged();

protected:
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
};

class QQuickPropertyAnimationPrivate;
class Q_QUICK_EXPORT QQuickPropertyAnimation : public QQuickAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPropertyAnimation)

    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QVariant from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(QVariant to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)
    Q_PROPERTY(QObject *target READ target WRITE setTargetObject NOTIFY targetChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(QString properties READ properties WRITE setProperties NOTIFY propertiesChanged)
    Q_PROPERTY(QQmlListProperty<QObject> targets READ targets)
    Q_PROPERTY(QQmlListProperty<QObject> exclude READ exclude)
    QML_NAMED_ELEMENT(PropertyAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickPropertyAnimation(QObject *parent=nullptr);
    ~QQuickPropertyAnimation() override;

    virtual int duration() const;
    virtual void setDuration(int);

    QVariant from() const;
    void setFrom(const QVariant &);

    QVariant to() const;
    void setTo(const QVariant &);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

    QObject *target() const;
    void setTargetObject(QObject *);

    QString property() const;
    void setProperty(const QString &);

    QString properties() const;
    void setProperties(const QString &);

    QQmlListProperty<QObject> targets();
    QQmlListProperty<QObject> exclude();

protected:
    QQuickStateActions createTransitionActions(QQuickStateActions &actions,
                                                     QQmlProperties &modified,
                                                     QObject *defaultTarget = nullptr);

    QQuickPropertyAnimation(QQuickPropertyAnimationPrivate &dd, QObject *parent);
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
Q_SIGNALS:
    void durationChanged(int);
    void fromChanged();
    void toChanged();
    void easingChanged(const QEasingCurve &);
    void propertiesChanged(const QString &);
    void targetChanged();
    void propertyChanged();
};

class Q_QUICK_EXPORT QQuickColorAnimation : public QQuickPropertyAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPropertyAnimation)
    Q_PROPERTY(QColor from READ from WRITE setFrom)
    Q_PROPERTY(QColor to READ to WRITE setTo)
    QML_NAMED_ELEMENT(ColorAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickColorAnimation(QObject *parent=nullptr);
    ~QQuickColorAnimation() override;

    QColor from() const;
    void setFrom(const QColor &);

    QColor to() const;
    void setTo(const QColor &);
};

class Q_QUICK_EXPORT QQuickNumberAnimation : public QQuickPropertyAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPropertyAnimation)

    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged)
    QML_NAMED_ELEMENT(NumberAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickNumberAnimation(QObject *parent=nullptr);
    ~QQuickNumberAnimation() override;

    qreal from() const;
    void setFrom(qreal);

    qreal to() const;
    void setTo(qreal);

protected:
    QQuickNumberAnimation(QQuickPropertyAnimationPrivate &dd, QObject *parent);

private:
    void init();
};

class Q_QUICK_EXPORT QQuickVector3dAnimation : public QQuickPropertyAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPropertyAnimation)

    Q_PROPERTY(QVector3D from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(QVector3D to READ to WRITE setTo NOTIFY toChanged)
    QML_NAMED_ELEMENT(Vector3dAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickVector3dAnimation(QObject *parent=nullptr);
    ~QQuickVector3dAnimation() override;

    QVector3D from() const;
    void setFrom(QVector3D);

    QVector3D to() const;
    void setTo(QVector3D);
};

class QQuickRotationAnimationPrivate;
class Q_QUICK_EXPORT QQuickRotationAnimation : public QQuickPropertyAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickRotationAnimation)

    Q_PROPERTY(qreal from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(qreal to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(RotationDirection direction READ direction WRITE setDirection NOTIFY directionChanged)
    QML_NAMED_ELEMENT(RotationAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickRotationAnimation(QObject *parent=nullptr);
    ~QQuickRotationAnimation() override;

    qreal from() const;
    void setFrom(qreal);

    qreal to() const;
    void setTo(qreal);

    enum RotationDirection { Numerical, Shortest, Clockwise, Counterclockwise };
    Q_ENUM(RotationDirection)
    RotationDirection direction() const;
    void setDirection(RotationDirection direction);

Q_SIGNALS:
    void directionChanged();
};

class QQuickAnimationGroupPrivate;
class Q_QUICK_EXPORT QQuickAnimationGroup : public QQuickAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickAnimationGroup)

    Q_CLASSINFO("DefaultProperty", "animations")
    Q_PROPERTY(QQmlListProperty<QQuickAbstractAnimation> animations READ animations)

public:
    QQuickAnimationGroup(QObject *parent);
    ~QQuickAnimationGroup() override;

    QQmlListProperty<QQuickAbstractAnimation> animations();
    friend class QQuickAbstractAnimation;

protected:
    QQuickAnimationGroup(QQuickAnimationGroupPrivate &dd, QObject *parent);
};

class Q_QUICK_EXPORT QQuickSequentialAnimation : public QQuickAnimationGroup
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickAnimationGroup)
    QML_NAMED_ELEMENT(SequentialAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickSequentialAnimation(QObject *parent=nullptr);
    ~QQuickSequentialAnimation() override;

protected:
    ThreadingModel threadingModel() const override;
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
};

class Q_QUICK_EXPORT QQuickParallelAnimation : public QQuickAnimationGroup
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickAnimationGroup)
    QML_NAMED_ELEMENT(ParallelAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickParallelAnimation(QObject *parent=nullptr);
    ~QQuickParallelAnimation() override;

protected:
    ThreadingModel threadingModel() const override;
    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
};


QT_END_NAMESPACE

#endif // QQUICKANIMATION_H
