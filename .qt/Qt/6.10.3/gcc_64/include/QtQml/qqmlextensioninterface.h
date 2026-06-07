// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLEXTENSIONINTERFACE_H
#define QQMLEXTENSIONINTERFACE_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class QQmlEngine;

class Q_QML_EXPORT QQmlTypesExtensionInterface
{
public:
    virtual ~QQmlTypesExtensionInterface();
    virtual void registerTypes(const char *uri) = 0;
};

class Q_QML_EXPORT QQmlExtensionInterface : public QQmlTypesExtensionInterface
{
public:
    ~QQmlExtensionInterface() override;
    virtual void initializeEngine(QQmlEngine *engine, const char *uri) = 0;
};

class Q_QML_EXPORT QQmlEngineExtensionInterface
{
public:
    virtual ~QQmlEngineExtensionInterface();
    virtual void initializeEngine(QQmlEngine *engine, const char *uri) = 0;
};

#define QQmlTypesExtensionInterface_iid "org.qt-project.Qt.QQmlTypesExtensionInterface"
Q_DECLARE_INTERFACE(QQmlTypesExtensionInterface, "org.qt-project.Qt.QQmlTypesExtensionInterface/1.0")

// NOTE: When changing this to a new version and deciding to add backup code to
// continue to support the previous version, make sure to support both of these iids.
#define QQmlExtensionInterface_iid "org.qt-project.Qt.QQmlExtensionInterface/1.0"
#define QQmlExtensionInterface_iid_old "org.qt-project.Qt.QQmlExtensionInterface"

Q_DECLARE_INTERFACE(QQmlExtensionInterface, QQmlExtensionInterface_iid)

#define QQmlEngineExtensionInterface_iid "org.qt-project.Qt.QQmlEngineExtensionInterface"
Q_DECLARE_INTERFACE(QQmlEngineExtensionInterface, QQmlEngineExtensionInterface_iid)

QT_END_NAMESPACE

#endif // QQMLEXTENSIONINTERFACE_H
