// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDDECORATIONPLUGIN_H
#define QWAYLANDDECORATIONPLUGIN_H

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

#include <QtWaylandClient/qtwaylandclientglobal.h>

#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>
#include <QtCore/QObject>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandAbstractDecoration;

#define QWaylandDecorationFactoryInterface_iid "org.qt-project.Qt.WaylandClient.QWaylandDecorationFactoryInterface.5.4"

class Q_WAYLANDCLIENT_EXPORT QWaylandDecorationPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QWaylandDecorationPlugin(QObject *parent = nullptr);
    ~QWaylandDecorationPlugin() override;

    virtual QWaylandAbstractDecoration *create(const QString &key, const QStringList &paramList) = 0;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDDECORATIONPLUGIN_H
