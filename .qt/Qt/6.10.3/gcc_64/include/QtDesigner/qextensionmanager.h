// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QEXTENSIONMANAGER_H
#define QEXTENSIONMANAGER_H

#include <QtDesigner/extension_global.h>
#include <QtDesigner/extension.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

class QObject; // Fool syncqt

class QDESIGNER_EXTENSION_EXPORT QExtensionManager: public QObject, public QAbstractExtensionManager
{
    Q_OBJECT
    Q_INTERFACES(QAbstractExtensionManager)
public:
    explicit QExtensionManager(QObject *parent = nullptr);
    ~QExtensionManager();

    void registerExtensions(QAbstractExtensionFactory *factory, const QString &iid = QString()) override;
    void unregisterExtensions(QAbstractExtensionFactory *factory, const QString &iid = QString()) override;

    QObject *extension(QObject *object, const QString &iid) const override;

private:
    using FactoryList = QList<QAbstractExtensionFactory *>;
    QHash<QString, FactoryList> m_extensions;
    FactoryList m_globalExtension;
};

QT_END_NAMESPACE

#endif // QEXTENSIONMANAGER_H
