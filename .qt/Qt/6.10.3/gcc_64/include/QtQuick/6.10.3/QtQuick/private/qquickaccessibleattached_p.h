// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKACCESSIBLEATTACHED_H
#define QQUICKACCESSIBLEATTACHED_H

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

#include <QtQuick/qquickitem.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#if QT_CONFIG(accessibility)

#include <QtGui/qaccessible.h>
#include <private/qtquickglobal_p.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE


#define STATE_PROPERTY(P) \
    Q_PROPERTY(bool P READ P WRITE set_ ## P NOTIFY P ## Changed FINAL) \
    bool P() const { return m_proxying && !m_stateExplicitlySet.P ? m_proxying->P() : m_state.P ; } \
    void set_ ## P(bool arg) \
    { \
        if (m_proxying) \
            m_proxying->set_##P(arg);\
        m_stateExplicitlySet.P = true; \
        if (m_state.P == arg) \
            return; \
        m_state.P = arg; \
        Q_EMIT P ## Changed(arg); \
        QAccessible::State changedState; \
        changedState.P = true; \
        QAccessibleStateChangeEvent ev(parent(), changedState); \
        QAccessible::updateAccessibility(&ev); \
    } \
    Q_SIGNAL void P ## Changed(bool arg);

class Q_QUICK_EXPORT QQuickAccessibleAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAccessible::Role role READ role WRITE setRole NOTIFY roleChanged FINAL)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged FINAL)
    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged FINAL)
    Q_PROPERTY(bool ignored READ ignored WRITE setIgnored NOTIFY ignoredChanged FINAL)
    Q_PROPERTY(QQuickItem *labelledBy READ labelledBy WRITE setLabelledBy NOTIFY labelledByChanged FINAL)
    Q_PROPERTY(QQuickItem *labelFor READ labelFor WRITE setLabelFor NOTIFY labelForChanged FINAL)

    QML_NAMED_ELEMENT(Accessible)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Accessible is only available via attached properties.")
    QML_ATTACHED(QQuickAccessibleAttached)
    QML_EXTENDED_NAMESPACE(QAccessible)

public:
    STATE_PROPERTY(checkable)
    STATE_PROPERTY(checked)
    STATE_PROPERTY(editable)
    STATE_PROPERTY(focusable)
    STATE_PROPERTY(focused)
    STATE_PROPERTY(multiLine)
    STATE_PROPERTY(readOnly)
    STATE_PROPERTY(selected)
    STATE_PROPERTY(selectable)
    STATE_PROPERTY(pressed)
    STATE_PROPERTY(checkStateMixed)
    STATE_PROPERTY(defaultButton)
    STATE_PROPERTY(passwordEdit)
    STATE_PROPERTY(selectableText)
    STATE_PROPERTY(searchEdit)

    QQuickAccessibleAttached(QObject *parent);
    ~QQuickAccessibleAttached();

    QAccessible::Role role() const { return m_role; }
    void setRole(QAccessible::Role role);
    QString name() const {
        if (m_state.passwordEdit)
            return QString();
        if (!m_nameExplicitlySet && m_proxying && m_proxying->wasNameExplicitlySet())
            return m_proxying->name();
        return m_name;
    }

    bool wasNameExplicitlySet() const;
    void setName(const QString &name) {
        m_nameExplicitlySet = true;
        if (name != m_name) {
            m_name = name;
            Q_EMIT nameChanged();
            QAccessibleEvent ev(parent(), QAccessible::NameChanged);
            QAccessible::updateAccessibility(&ev);
        }
    }
    void setNameImplicitly(const QString &name);

    QString description() const {
        return !m_descriptionExplicitlySet && m_proxying ? m_proxying->description() : m_description;
    }
    void setDescription(const QString &description)
    {
        if (!m_descriptionExplicitlySet && m_proxying) {
            disconnect(m_proxying, &QQuickAccessibleAttached::descriptionChanged, this, &QQuickAccessibleAttached::descriptionChanged);
        }
        m_descriptionExplicitlySet = true;
        if (m_description != description) {
            m_description = description;
            Q_EMIT descriptionChanged();
            QAccessibleEvent ev(parent(), QAccessible::DescriptionChanged);
            QAccessible::updateAccessibility(&ev);
        }
    }
    void setDescriptionImplicitly(const QString &desc);

    QString id() const { return m_id; }
    void setId(const QString &id)
    {
        if (m_id != id) {
            m_id = id;
            Q_EMIT idChanged();
            QAccessibleEvent ev(parent(), QAccessible::IdentifierChanged);
            QAccessible::updateAccessibility(&ev);
        }
    }

    QQuickItem *labelledBy() const
    {
        return findRelation(QAccessible::Labelled);
    }

    void setLabelledBy(QQuickItem *labelledBy)
    {
        setLabelledByInternal(labelledBy);

        QQuickAccessibleAttached *label = qobject_cast<QQuickAccessibleAttached *>(
                qmlAttachedPropertiesObject<QQuickAccessibleAttached>(labelledBy));

        label->setLabelForInternal(qobject_cast<QQuickItem *>(parent()));
    }
    void setLabelledByInternal(QQuickItem *labelledBy)
    {
        m_relations.append({ labelledBy, QAccessible::Labelled });
        Q_EMIT labelledByChanged();
    }

    QQuickItem *labelFor() const
    {
        return findRelation(QAccessible::Label);
    }

    void setLabelFor(QQuickItem *labelFor)
    {
        setLabelForInternal(labelFor);

        QQuickAccessibleAttached *labelled = qobject_cast<QQuickAccessibleAttached *>(
                qmlAttachedPropertiesObject<QQuickAccessibleAttached>(labelFor));
        labelled->setLabelledBy(qobject_cast<QQuickItem *>(parent()));
    }

    void setLabelForInternal(QQuickItem *labelFor)
    {
        m_relations.append({ labelFor, QAccessible::Label });
        Q_EMIT labelForChanged();
    }

    // Factory function
    static QQuickAccessibleAttached *qmlAttachedProperties(QObject *obj);

    static QQuickAccessibleAttached *attachedProperties(const QObject *obj)
    {
        return qobject_cast<QQuickAccessibleAttached*>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(obj, false));
    }

    // Property getter
    static QVariant property(const QObject *object, const char *propertyName)
    {
        if (QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object))
            return attachedObject->property(propertyName);
        return QVariant();
    }

    static bool setProperty(QObject *object, const char *propertyName, const QVariant &value)
    {
        QObject *obj = qmlAttachedPropertiesObject<QQuickAccessibleAttached>(object, true);
        if (!obj) {
            qWarning("cannot set property Accessible.%s of QObject %s", propertyName, object->metaObject()->className());
            return false;
        }
        return obj->setProperty(propertyName, value);
    }

    static QObject *findAccessible(QObject *object, QAccessible::Role role = QAccessible::NoRole)
    {
        while (object) {
            QQuickAccessibleAttached *att = QQuickAccessibleAttached::attachedProperties(object);
            if (att && (role == QAccessible::NoRole || att->role() == role)) {
                break;
            }
            if (auto action = object->property("action").value<QObject *>(); action) {
                QQuickAccessibleAttached *att = QQuickAccessibleAttached::attachedProperties(action);
                if (att && (role == QAccessible::NoRole || att->role() == role)) {
                    object = action;
                    break;
                }
            }
            object = object->parent();
        }
        return object;
    }

    QAccessible::State state() const { return m_state; }
    bool ignored() const;
    bool doAction(const QString &actionName);
    void availableActions(QStringList *actions) const;

    Q_REVISION(6, 2) Q_INVOKABLE static QString stripHtml(const QString &html);
    void setProxying(QQuickAccessibleAttached *proxying);

    Q_REVISION(6, 8) Q_INVOKABLE void announce(const QString &message, QAccessible::AnnouncementPoliteness politeness = QAccessible::AnnouncementPoliteness::Polite);

public Q_SLOTS:
    void valueChanged() {
        QAccessibleValueChangeEvent ev(parent(), parent()->property("value"));
        QAccessible::updateAccessibility(&ev);
    }
    void cursorPositionChanged() {
        QAccessibleTextCursorEvent ev(parent(), parent()->property("cursorPosition").toInt());
        QAccessible::updateAccessibility(&ev);
    }

    void setIgnored(bool ignored);

Q_SIGNALS:
    void roleChanged();
    void nameChanged();
    void descriptionChanged();
    void idChanged();
    void ignoredChanged();
    void labelledByChanged();
    void labelForChanged();
    void pressAction();
    void toggleAction();
    void increaseAction();
    void decreaseAction();
    void scrollUpAction();
    void scrollDownAction();
    void scrollLeftAction();
    void scrollRightAction();
    void previousPageAction();
    void nextPageAction();

private:
    QQuickItem *findRelation(QAccessible::Relation relation) const;

    QAccessible::Role m_role;
    QAccessible::State m_state;
    QAccessible::State m_stateExplicitlySet;
    QString m_name;
    bool m_nameExplicitlySet = false;
    QString m_description;
    bool m_descriptionExplicitlySet = false;
    QQuickAccessibleAttached* m_proxying = nullptr;
    QString m_id;
    QList<std::pair<QQuickItem *, QAccessible::Relation>> m_relations;

    static QMetaMethod sigPress;
    static QMetaMethod sigToggle;
    static QMetaMethod sigIncrease;
    static QMetaMethod sigDecrease;
    static QMetaMethod sigScrollUp;
    static QMetaMethod sigScrollDown;
    static QMetaMethod sigScrollLeft;
    static QMetaMethod sigScrollRight;
    static QMetaMethod sigPreviousPage;
    static QMetaMethod sigNextPage;

public:
    using QObject::property;
};


QT_END_NAMESPACE

#endif // accessibility

#endif
