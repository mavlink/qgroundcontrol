// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQUICKPACKAGE_H
#define QQUICKPACKAGE_H

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
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>

QT_REQUIRE_CONFIG(qml_delegate_model);

QT_BEGIN_NAMESPACE

class QQuickPackagePrivate;
class QQuickPackageAttached;
class Q_QMLMODELS_EXPORT QQuickPackage : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPackage)

    Q_CLASSINFO("DefaultProperty", "data")
    QML_NAMED_ELEMENT(Package)
    QML_ADDED_IN_VERSION(2, 0)
    QML_ATTACHED(QQuickPackageAttached)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data)

public:
    QQuickPackage(QObject *parent=nullptr);

    QQmlListProperty<QObject> data();

    QObject *part(const QString & = QString());
    bool hasPart(const QString &);

    static QQuickPackageAttached *qmlAttachedProperties(QObject *);
};

class QQuickPackageAttached : public QObject
{
Q_OBJECT
Q_PROPERTY(QString name READ name WRITE setName FINAL)
public:
    QQuickPackageAttached(QObject *parent);
    virtual ~QQuickPackageAttached();

    QString name() const;
    void setName(const QString &n);

    static QHash<QObject *, QQuickPackageAttached *> attached;
private:
    QString _name;
};

QT_END_NAMESPACE

#endif // QQUICKPACKAGE_H
