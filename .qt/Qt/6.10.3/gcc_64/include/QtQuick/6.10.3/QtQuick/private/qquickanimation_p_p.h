// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKANIMATION2_P_H
#define QQUICKANIMATION2_P_H

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

#include "qquickanimation_p.h"

#include <private/qqmlnullablevalue_p.h>

#include <qqml.h>
#include <qqmlcontext.h>

#include <private/qvariantanimation_p.h>
#include "private/qpauseanimationjob_p.h"
#include <QDebug>

#include <private/qobject_p.h>
#include "private/qanimationgroupjob_p.h"
#include <QDebug>

#include <private/qobject_p.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

//interface for classes that provide animation actions for QActionAnimation
class QAbstractAnimationAction
{
public:
    virtual ~QAbstractAnimationAction() {}
    virtual void doAction() = 0;
    virtual void debugAction(QDebug, int) const {}
};

//templated animation action
//allows us to specify an action that calls a function of a class.
//(so that class doesn't have to inherit QQuickAbstractAnimationAction)
template<class T, void (T::*method)(), void (T::*debugMethod)(QDebug, int) const>
class QAnimationActionProxy : public QAbstractAnimationAction
{
public:
    QAnimationActionProxy(T *instance) : m_instance(instance) {}
    void doAction() override { (m_instance->*method)(); }
    void debugAction(QDebug d, int indentLevel) const override { (m_instance->*debugMethod)(d, indentLevel); }
private:
    T *m_instance;
};

//performs an action of type QAbstractAnimationAction
class Q_AUTOTEST_EXPORT QActionAnimation : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QActionAnimation)
public:
    QActionAnimation();

    QActionAnimation(QAbstractAnimationAction *action);
    ~QActionAnimation() override;

    int duration() const override;
    void setAnimAction(QAbstractAnimationAction *action);

protected:
    void updateCurrentTime(int) override;
    void updateState(State newState, State oldState) override;
    void debugAnimation(QDebug d) const override;

private:
    QAbstractAnimationAction *animAction;
};

class QQuickBulkValueUpdater
{
public:
    virtual ~QQuickBulkValueUpdater() {}
    virtual void setValue(qreal value) = 0;
    virtual void debugUpdater(QDebug, int) const {}
};

//animates QQuickBulkValueUpdater (assumes start and end values will be reals or compatible)
class Q_QUICK_AUTOTEST_EXPORT QQuickBulkValueAnimator : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QQuickBulkValueAnimator)
public:
    QQuickBulkValueAnimator();
    ~QQuickBulkValueAnimator() override;

    void setAnimValue(QQuickBulkValueUpdater *value);
    QQuickBulkValueUpdater *getAnimValue() const { return animValue; }

    void setFromIsSourcedValue(bool *value) { fromIsSourced = value; }

    int duration() const override { return m_duration; }
    void setDuration(int msecs) { m_duration = msecs; }

    QEasingCurve easingCurve() const { return easing; }
    void setEasingCurve(const QEasingCurve &curve) { easing = curve; }

protected:
    void updateCurrentTime(int currentTime) override;
    void topLevelAnimationLoopChanged() override;
    void debugAnimation(QDebug d) const override;

private:
    QQuickBulkValueUpdater *animValue;
    bool *fromIsSourced;
    int m_duration;
    QEasingCurve easing;
};

//an animation that just gives a tick
template<class T, void (T::*method)(int)>
class QTickAnimationProxy : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QTickAnimationProxy)
public:
    QTickAnimationProxy(T *instance) : QAbstractAnimationJob(), m_instance(instance) {}
    int duration() const override { return -1; }
protected:
    void updateCurrentTime(int msec) override { (m_instance->*method)(msec); }

private:
    T *m_instance;
};

class Q_QUICK_EXPORT QQuickAbstractAnimationPrivate : public QObjectPrivate, public QAnimationJobChangeListener
{
    Q_DECLARE_PUBLIC(QQuickAbstractAnimation)
public:
    QQuickAbstractAnimationPrivate()
    : running(false), paused(false), alwaysRunToEnd(false),
      /*connectedTimeLine(false), */componentComplete(true),
      avoidPropertyValueSourceStart(false), disableUserControl(false),
      needsDeferredSetRunning(false), loopCount(1), group(nullptr), animationInstance(nullptr) {}

    bool running:1;
    bool paused:1;
    bool alwaysRunToEnd:1;
    //bool connectedTimeLine:1;
    bool componentComplete:1;
    bool avoidPropertyValueSourceStart:1;
    bool disableUserControl:1;
    bool needsDeferredSetRunning:1;

    int loopCount;

    void commence();
    void animationFinished(QAbstractAnimationJob *) override;

    QQmlProperty defaultProperty;

    QQuickAnimationGroup *group;
    QAbstractAnimationJob* animationInstance;

    static QQmlProperty createProperty(QObject *obj, const QString &str, QObject *infoObj, QString *errorMessage = nullptr);
    void animationGroupDirty();
};

class QQuickPauseAnimationPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickPauseAnimation)
public:
    QQuickPauseAnimationPrivate()
        : QQuickAbstractAnimationPrivate(), duration(250) {}

    int duration;
};

class QQuickScriptActionPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickScriptAction)
public:
    QQuickScriptActionPrivate();

    QQmlScriptString script;
    QString name;
    QQmlScriptString runScriptScript;
    bool hasRunScriptScript;
    bool reversing;

    void execute();
    QAbstractAnimationAction* createAction();
    void debugAction(QDebug d, int indentLevel) const;
    typedef QAnimationActionProxy<QQuickScriptActionPrivate,
                                 &QQuickScriptActionPrivate::execute,
                                 &QQuickScriptActionPrivate::debugAction> Proxy;
};

class QQuickPropertyActionPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickPropertyAction)
public:
    QQuickPropertyActionPrivate()
    : QQuickAbstractAnimationPrivate(), target(nullptr) {}

    QObject *target;
    QString propertyName;
    QString properties;
    QList<QObject *> targets;
    QList<QObject *> exclude;

    QQmlNullableValue<QVariant> value;
};

class QQuickAnimationGroupPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickAnimationGroup)
public:
    QQuickAnimationGroupPrivate()
    : QQuickAbstractAnimationPrivate(), animationDirty(false) {}

    static void append_animation(QQmlListProperty<QQuickAbstractAnimation> *list, QQuickAbstractAnimation *role);
    static QQuickAbstractAnimation *at_animation(QQmlListProperty<QQuickAbstractAnimation> *list, qsizetype index);
    static qsizetype count_animation(QQmlListProperty<QQuickAbstractAnimation> *list);
    static void clear_animation(QQmlListProperty<QQuickAbstractAnimation> *list);
    static void replace_animation(QQmlListProperty<QQuickAbstractAnimation> *list, qsizetype index,
                                  QQuickAbstractAnimation *role);
    static void removeLast_animation(QQmlListProperty<QQuickAbstractAnimation> *list);
    QList<QQuickAbstractAnimation *> animations;

    void restartFromCurrentLoop();
    void animationCurrentLoopChanged(QAbstractAnimationJob *job) override;
    bool animationDirty: 1;
};

class Q_QUICK_EXPORT QQuickPropertyAnimationPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickPropertyAnimation)
public:
    QQuickPropertyAnimationPrivate()
    : QQuickAbstractAnimationPrivate(), target(nullptr), fromIsDefined(false), toIsDefined(false), ourPropertiesDirty(false),
      defaultToInterpolatorType(0), interpolatorType(0), interpolator(nullptr), extendedInterpolator(nullptr), duration(250), actions(nullptr) {}

    void animationCurrentLoopChanged(QAbstractAnimationJob *job) override;

    QVariant from;
    QVariant to;

    QObject *target;
    QString propertyName;
    QString properties;
    QList<QPointer<QObject>> targets;
    QList<QObject *> exclude;
    QString defaultProperties;

    bool fromIsDefined:1;
    bool toIsDefined:1;
    bool ourPropertiesDirty : 1;
    bool defaultToInterpolatorType:1;
    int interpolatorType;
    QVariantAnimation::Interpolator interpolator;
    typedef QVariant (*ExtendedInterpolator)(const void *from, const void *to, const QVariant &currentValue, qreal progress);
    ExtendedInterpolator extendedInterpolator;
    int duration;
    QEasingCurve easing;

    // for animations that don't use the QQuickBulkValueAnimator
    QQuickStateActions *actions;

    static void convertVariant(QVariant &variant, QMetaType type);
};

class QQuickRotationAnimationPrivate : public QQuickPropertyAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickRotationAnimation)
public:
    QQuickRotationAnimationPrivate() : direction(QQuickRotationAnimation::Numerical) {}

    QQuickRotationAnimation::RotationDirection direction;
};

class Q_AUTOTEST_EXPORT QQuickAnimationPropertyUpdater : public QQuickBulkValueUpdater
{
public:
    QQuickAnimationPropertyUpdater() : interpolatorType(0), interpolator(nullptr), extendedInterpolator(nullptr), prevInterpolatorType(0), reverse(false), fromIsSourced(false), fromIsDefined(false), wasDeleted(nullptr) {}
    ~QQuickAnimationPropertyUpdater() override;

    void setValue(qreal v) override;

    void debugUpdater(QDebug d, int indentLevel) const override;

    QQuickStateActions actions;
    int interpolatorType;       //for Number/ColorAnimation
    QVariantAnimation::Interpolator interpolator;
    QQuickPropertyAnimationPrivate::ExtendedInterpolator extendedInterpolator;
    int prevInterpolatorType;   //for generic
    bool reverse;
    bool fromIsSourced;
    bool fromIsDefined;
    bool *wasDeleted;
};

QT_END_NAMESPACE

#endif // QQUICKANIMATION2_P_H
