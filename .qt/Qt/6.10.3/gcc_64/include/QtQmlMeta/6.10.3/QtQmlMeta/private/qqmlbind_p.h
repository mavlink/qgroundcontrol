// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLBIND_H
#define QQMLBIND_H

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

#include <QtQmlMeta/qtqmlmetaexports.h>

#include <QtQml/qqml.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQmlBindPrivate;
class Q_QMLMETA_EXPORT QQmlBind : public QObject, public QQmlPropertyValueSource, public QQmlParserStatus
{
public:
    enum RestorationMode {
        RestoreNone    = 0x0,
        RestoreBinding = 0x1,
        RestoreValue   = 0x2,
        RestoreBindingOrValue = RestoreBinding | RestoreValue
    };

private:
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlBind)
    Q_INTERFACES(QQmlParserStatus)
    Q_INTERFACES(QQmlPropertyValueSource)
    Q_PROPERTY(QObject *target READ object WRITE setObject NOTIFY objectChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(bool when READ when WRITE setWhen NOTIFY whenChanged)
    Q_PROPERTY(bool delayed READ delayed WRITE setDelayed NOTIFY delayedChanged REVISION(2, 8))
    Q_PROPERTY(RestorationMode restoreMode READ restoreMode WRITE setRestoreMode
               NOTIFY restoreModeChanged REVISION(2, 14))
    Q_ENUM(RestorationMode)
    QML_NAMED_ELEMENT(Binding)
    QML_ADDED_IN_VERSION(2, 0)
    Q_CLASSINFO("ImmediatePropertyNames", "objectName,target,property,value,when,delayed,restoreMode");

public:
    QQmlBind(QObject *parent=nullptr);
    ~QQmlBind();

    bool when() const;
    void setWhen(bool);

    QObject *object() const;
    void setObject(QObject *);

    QString property() const;
    void setProperty(const QString &);

    QVariant value() const;
    void setValue(const QVariant &);

    bool delayed() const;
    void setDelayed(bool);

    RestorationMode restoreMode() const;
    void setRestoreMode(RestorationMode);

Q_SIGNALS:
    void restoreModeChanged();
    Q_REVISION(6, 10) void objectChanged();
    Q_REVISION(6, 10) void propertyChanged();
    Q_REVISION(6, 10) void valueChanged();
    Q_REVISION(6, 10) void whenChanged();
    Q_REVISION(6, 10) void delayedChanged();

protected:
    void setTarget(const QQmlProperty &) override;
    void classBegin() override;
    void componentComplete() override;

private:
    void prepareEval();
    void eval();

private Q_SLOTS:
    void targetValueChanged();
};

QT_END_NAMESPACE

#endif
