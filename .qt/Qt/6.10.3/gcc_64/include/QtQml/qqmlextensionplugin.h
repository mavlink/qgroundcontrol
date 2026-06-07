// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLEXTENSIONPLUGIN_H
#define QQMLEXTENSIONPLUGIN_H

#include <QtCore/qplugin.h>
#include <QtCore/QUrl>
#include <QtQml/qqmlextensioninterface.h>

#if defined(Q_CC_GHS)
#  define Q_GHS_KEEP_REFERENCE(S) QT_DO_PRAGMA(ghs reference S ##__Fv)
#else
#  define Q_GHS_KEEP_REFERENCE(S)
#endif

#define Q_IMPORT_QML_PLUGIN(PLUGIN) \
    Q_IMPORT_PLUGIN(PLUGIN)

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQmlExtensionPluginPrivate;

class Q_QML_EXPORT QQmlExtensionPlugin
    : public QObject
    , public QQmlExtensionInterface
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlExtensionPlugin)
    Q_INTERFACES(QQmlExtensionInterface)
    Q_INTERFACES(QQmlTypesExtensionInterface)
public:
    explicit QQmlExtensionPlugin(QObject *parent = nullptr);
    ~QQmlExtensionPlugin() override;

#if QT_DEPRECATED_SINCE(6, 3)
    QT_DEPRECATED_VERSION_X_6_3("Provide a qmldir file to remove the need for calling baseUrl")
    QUrl baseUrl() const;
#endif

    void registerTypes(const char *uri) override = 0;
    virtual void unregisterTypes();
    void initializeEngine(QQmlEngine *engine, const char *uri) override;

private:
    Q_DISABLE_COPY(QQmlExtensionPlugin)
};

class Q_QML_EXPORT QQmlEngineExtensionPlugin
        : public QObject
        , public QQmlEngineExtensionInterface
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QQmlEngineExtensionPlugin)
    Q_INTERFACES(QQmlEngineExtensionInterface)
public:
    explicit QQmlEngineExtensionPlugin(QObject *parent = nullptr);
    ~QQmlEngineExtensionPlugin() override;
    void initializeEngine(QQmlEngine *engine, const char *uri) override;
};

QT_END_NAMESPACE

#endif // QQMLEXTENSIONPLUGIN_H
