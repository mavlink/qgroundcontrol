// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDSERVERBUFFERINTEGRATIONFACTORY_H
#define QWAYLANDSERVERBUFFERINTEGRATIONFACTORY_H

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
#include <QtCore/QStringList>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

class QWaylandServerBufferIntegration;

class Q_WAYLANDCLIENT_EXPORT QWaylandServerBufferIntegrationFactory
{
public:
    static QStringList keys();
    static QWaylandServerBufferIntegration *create(const QString &name, const QStringList &args);
};

}

QT_END_NAMESPACE

#endif // QWAYLANDSERVERBUFFERINTEGRATIONFACTORY_H
