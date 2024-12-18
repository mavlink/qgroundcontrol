// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>

#include "qgfxsourceproxy_p.h"

QT_BEGIN_NAMESPACE

class QGCGraphicalEffectsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QGCGraphicalEffectsPlugin(QObject *parent = 0) : QQmlExtensionPlugin(parent) { }
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QGroundControl.GraphicalEffects"));

        qmlRegisterType<QGfxSourceProxy>(uri, 1, 0, "SourceProxy");
        qmlRegisterModule(uri, 1, 0);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
