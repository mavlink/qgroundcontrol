// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTATE_H
#define QQUICKSTATE_H

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

#include <qqml.h>
#include <qqmlproperty.h>
#include <QtCore/qobject.h>
#include <QtCore/qsharedpointer.h>
#include <private/qtquickglobal_p.h>
#include <private/qqmlabstractbinding_p.h>
#include <private/qqmlanybinding_p.h>

#include <QtCore/private/qproperty_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcStates)

class QQuickStateActionEvent;
class QQmlBinding;
class QQmlExpression;

class Q_QUICK_EXPORT QQuickStateAction
{
public:
    QQuickStateAction();
    QQuickStateAction(QObject *, const QString &, const QVariant &);
    QQuickStateAction(QObject *, const QQmlProperty &property, const QString &,
                      const QVariant &);

    bool restore:1;
    bool actionDone:1;
    bool reverseEvent:1;
    bool deletableToBinding:1;

    QQmlProperty property;
    QVariant fromValue;
    QVariant toValue;

    QQmlAnyBinding fromBinding;
    QQmlAnyBinding toBinding;
    QQuickStateActionEvent *event;

    //strictly for matching
    QObject *specifiedObject;
    QString specifiedProperty;

    void deleteFromBinding();
};

class Q_AUTOTEST_EXPORT QQuickStateActionEvent
{
public:
    virtual ~QQuickStateActionEvent();

    enum EventType { Script, SignalHandler, ParentChange, AnchorChanges };

    virtual EventType type() const = 0;

    virtual void execute();
    virtual bool isReversable();
    virtual void reverse();
    virtual void saveOriginals() {}
    virtual bool needsCopy() { return false; }
    virtual void copyOriginals(QQuickStateActionEvent *) {}

    virtual bool isRewindable() { return isReversable(); }
    virtual void rewind() {}
    virtual void saveCurrentValues() {}
    virtual void saveTargetValues() {}

    virtual bool changesBindings();
    virtual void clearBindings();
    virtual bool mayOverride(QQuickStateActionEvent*other);
};

//### rename to QQuickStateChange?
class QQuickStateGroup;
class QQuickState;
class QQuickStateOperationPrivate;
class Q_QUICK_EXPORT QQuickStateOperation : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickStateOperation(QObject *parent = nullptr)
        : QObject(parent) {}
    typedef QList<QQuickStateAction> ActionList;

    virtual ActionList actions();

    QQuickState *state() const;
    void setState(QQuickState *state);

protected:
    QQuickStateOperation(QObjectPrivate &dd, QObject *parent = nullptr);

private:
    Q_DECLARE_PRIVATE(QQuickStateOperation)
    Q_DISABLE_COPY(QQuickStateOperation)
};

typedef QQuickStateOperation::ActionList QQuickStateActions;

class QQuickTransition;
class QQuickStatePrivate;
class Q_QUICK_EXPORT QQuickState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(bool when READ when WRITE setWhen)
    Q_PROPERTY(QString extend READ extends WRITE setExtends)
    Q_PROPERTY(QQmlListProperty<QQuickStateOperation> changes READ changes)
    Q_CLASSINFO("DefaultProperty", "changes")
    Q_CLASSINFO("DeferredPropertyNames", "changes")
    QML_NAMED_ELEMENT(State)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickState(QObject *parent=nullptr);
    ~QQuickState() override;

    QString name() const;
    void setName(const QString &);
    bool isNamed() const;

    bool isWhenKnown() const;
    bool when() const;
    void setWhen(bool);

    QString extends() const;
    void setExtends(const QString &);

    QQmlListProperty<QQuickStateOperation> changes();
    int operationCount() const;
    QQuickStateOperation *operationAt(int) const;

    QQuickState &operator<<(QQuickStateOperation *);

    void apply(QQuickTransition *, QQuickState *revert);
    void cancel();

    QQuickStateGroup *stateGroup() const;
    void setStateGroup(QQuickStateGroup *);

    bool containsPropertyInRevertList(QObject *target, const QString &name) const;
    bool changeValueInRevertList(QObject *target, const QString &name, const QVariant &revertValue);
    bool changeBindingInRevertList(QObject *target, const QString &name, QQmlAnyBinding binding);
    bool removeEntryFromRevertList(QObject *target, const QString &name);
    void addEntryToRevertList(const QQuickStateAction &action);
    void removeAllEntriesFromRevertList(QObject *target);
    void addEntriesToRevertList(const QList<QQuickStateAction> &actions);
    QVariant valueInRevertList(QObject *target, const QString &name) const;
    QQmlAnyBinding bindingInRevertList(QObject *target, const QString &name) const;

    bool isStateActive() const;

Q_SIGNALS:
    void completed();

private:
    Q_DECLARE_PRIVATE(QQuickState)
    Q_DISABLE_COPY(QQuickState)
};

QT_END_NAMESPACE

#endif // QQUICKSTATE_H
