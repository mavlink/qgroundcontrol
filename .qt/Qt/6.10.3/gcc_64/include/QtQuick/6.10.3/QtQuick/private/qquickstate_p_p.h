// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSTATE_P_H
#define QQUICKSTATE_P_H

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

#include "qquicktransitionmanager_p_p.h"

#include <private/qqmlproperty_p.h>
#include <private/qqmlguard_p.h>

#include <private/qqmlbinding_p.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickSimpleAction
{
public:
    enum State { StartState, EndState };
    QQuickSimpleAction(const QQuickStateAction &a, State state = StartState)
    {
        m_property = a.property;
        m_specifiedObject = a.specifiedObject;
        m_specifiedProperty = a.specifiedProperty;
        m_event = a.event;
        if (state == StartState) {
            m_value = a.fromValue;
            m_binding = QQmlAnyBinding::ofProperty(m_property);
            m_reverseEvent = true;
        } else {
            m_value = a.toValue;
            m_binding = a.toBinding;
            m_reverseEvent = false;
        }
    }

    ~QQuickSimpleAction()
    {
    }

    QQuickSimpleAction(const QQuickSimpleAction &other)
        :  m_property(other.m_property),
        m_value(other.m_value),
        m_binding(other.binding()),
        m_specifiedObject(other.m_specifiedObject),
        m_specifiedProperty(other.m_specifiedProperty),
        m_event(other.m_event),
        m_reverseEvent(other.m_reverseEvent)
    {
    }

    QQuickSimpleAction &operator =(const QQuickSimpleAction &other)
    {
        m_property = other.m_property;
        m_value = other.m_value;
        m_binding = other.binding();
        m_specifiedObject = other.m_specifiedObject;
        m_specifiedProperty = other.m_specifiedProperty;
        m_event = other.m_event;
        m_reverseEvent = other.m_reverseEvent;

        return *this;
    }

    void setProperty(const QQmlProperty &property)
    {
        m_property = property;
    }

    const QQmlProperty &property() const
    {
        return m_property;
    }

    void setValue(const QVariant &value)
    {
        m_value = value;
    }

    const QVariant &value() const
    {
        return m_value;
    }

    void setBinding(QQmlAnyBinding binding)
    {
        m_binding = binding;
    }

    QQmlAnyBinding binding() const
    {
        return m_binding;
    }

    QObject *specifiedObject() const
    {
        return m_specifiedObject;
    }

    const QString &specifiedProperty() const
    {
        return m_specifiedProperty;
    }

    QQuickStateActionEvent *event() const
    {
        return m_event;
    }

    bool reverseEvent() const
    {
        return m_reverseEvent;
    }

private:
    QQmlProperty m_property;
    QVariant m_value;
    QQmlAnyBinding m_binding;
    QObject *m_specifiedObject;
    QString m_specifiedProperty;
    QQuickStateActionEvent *m_event;
    bool m_reverseEvent;
};

class QQuickRevertAction
{
public:
    QQuickRevertAction() : event(nullptr) {}
    QQuickRevertAction(const QQmlProperty &prop) : property(prop), event(nullptr) {}
    QQuickRevertAction(QQuickStateActionEvent *e) : event(e) {}
    QQmlProperty property;
    QQuickStateActionEvent *event;
};

class QQuickStateOperationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickStateOperation)

public:

    QQuickStateOperationPrivate()
    : m_state(nullptr) {}

    QQuickState *m_state;
};

class QQuickStatePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickState)

public:
    QQuickStatePrivate()
        : when(false), whenKnown(false), named(false), inState(false), group(nullptr) {}

    typedef QList<QQuickSimpleAction> SimpleActionList;

    QString name;
    bool when;
    bool whenKnown;
    bool named;

    struct OperationGuard : public QQmlGuard<QQuickStateOperation>
    {
        OperationGuard(QObject *obj, QList<OperationGuard> *l) : QQmlGuard<QQuickStateOperation>(
                                                                     OperationGuard::objectDestroyedImpl, nullptr)
                                                               ,list(l)
        {
            setObject(static_cast<QQuickStateOperation *>(obj));
        }
        QList<OperationGuard> *list;

    private:
        static void objectDestroyedImpl(QQmlGuardImpl *guard) {
            auto This = static_cast<OperationGuard *>(guard);
            // we assume priv will always be destroyed after objectDestroyed calls
            This->list->removeOne(*This);
        }
    };
    QList<OperationGuard> operations;

    static void operations_append(QQmlListProperty<QQuickStateOperation> *prop, QQuickStateOperation *op) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        op->setState(qobject_cast<QQuickState*>(prop->object));
        list->append(OperationGuard(op, list));
    }
    static void operations_clear(QQmlListProperty<QQuickStateOperation> *prop) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        for (auto &e : *list)
            e->setState(nullptr);
        list->clear();
    }
    static qsizetype operations_count(QQmlListProperty<QQuickStateOperation> *prop) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        return list->size();
    }
    static QQuickStateOperation *operations_at(QQmlListProperty<QQuickStateOperation> *prop, qsizetype index) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        return list->at(index);
    }
    static void operations_replace(QQmlListProperty<QQuickStateOperation> *prop, qsizetype index,
                                   QQuickStateOperation *op) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        auto &guard = list->at(index);
        if (guard.object() == op) {
            op->setState(qobject_cast<QQuickState*>(prop->object));
        } else {
            list->at(index)->setState(nullptr);
            op->setState(qobject_cast<QQuickState*>(prop->object));
            list->replace(index, OperationGuard(op, list));
        }
    }
    static void operations_removeLast(QQmlListProperty<QQuickStateOperation> *prop) {
        QList<OperationGuard> *list = static_cast<QList<OperationGuard> *>(prop->data);
        list->last()->setState(nullptr);
        list->removeLast();
    }

    QQuickTransitionManager transitionManager;

    SimpleActionList revertList;
    QList<QQuickRevertAction> reverting;
    QString extends;
    mutable bool inState;
    QQuickStateGroup *group;

    QQuickStateOperation::ActionList generateActionList() const;
    void complete();
};

QT_END_NAMESPACE

#endif // QQUICKSTATE_P_H
