// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPROPERTYCHANGES_H
#define QQUICKPROPERTYCHANGES_H

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

#include "qquickstatechangescript_p.h"
#include <private/qqmlcustomparser_p.h>

QT_BEGIN_NAMESPACE

class QQuickPropertyChangesPrivate;
class Q_QUICK_EXPORT QQuickPropertyChanges : public QQuickStateOperation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPropertyChanges)
    Q_PROPERTY(QObject *target READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(bool restoreEntryValues READ restoreEntryValues WRITE setRestoreEntryValues
               NOTIFY restoreEntryValuesChanged)
    Q_PROPERTY(bool explicit READ isExplicit WRITE setIsExplicit NOTIFY isExplicitChanged)
    QML_NAMED_ELEMENT(PropertyChanges)
    QML_ADDED_IN_VERSION(2, 0)
    QML_CUSTOMPARSER
    Q_CLASSINFO("ImmediatePropertyNames", "target,restoreEntryValues,explicit,objectName")

public:
    QQuickPropertyChanges();
    ~QQuickPropertyChanges();

    QObject *object() const;
    void setObject(QObject *);

    bool restoreEntryValues() const;
    void setRestoreEntryValues(bool);

    bool isExplicit() const;
    void setIsExplicit(bool);

    ActionList actions() override;

    bool containsProperty(const QString &name) const;
    bool containsValue(const QString &name) const;
    bool containsExpression(const QString &name) const;
    void changeValue(const QString &name, const QVariant &value);
    void changeExpression(const QString &name, const QString &expression);
    void removeProperty(const QString &name);
    QVariant value(const QString &name) const;
    QString expression(const QString &name) const;

    void detachFromState();
    void attachToState();

    QVariant property(const QString &name) const;

Q_SIGNALS:
    void objectChanged();
    void restoreEntryValuesChanged();
    void isExplicitChanged();
};

class QQuickPropertyChangesParser : public QQmlCustomParser
{
public:
    QQuickPropertyChangesParser()
    : QQmlCustomParser(AcceptsAttachedProperties) {}

    void verifyList(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
            const QV4::CompiledData::Binding *binding);

    void verifyBindings(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
            const QList<const QV4::CompiledData::Binding *> &props) override;
    void applyBindings(
            QObject *obj, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
            const QList<const QV4::CompiledData::Binding *> &bindings) override;
};

template<>
inline QQmlCustomParser *qmlCreateCustomParser<QQuickPropertyChanges>()
{
    return new QQuickPropertyChangesParser;
}

QT_END_NAMESPACE

#endif // QQUICKPROPERTYCHANGES_H
