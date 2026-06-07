// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPERMISSIONS_P_H
#define QPERMISSIONS_P_H

#include "qpermissions.h"

#include <private/qglobal_p.h>
#include <QtCore/qloggingcategory.h>

#include <QtCore/QObject>

#include <functional>

QT_REQUIRE_CONFIG(permissions);

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

QT_BEGIN_NAMESPACE

QT_DECLARE_EXPORTED_QT_LOGGING_CATEGORY(lcPermissions, Q_CORE_EXPORT)

namespace QPermissions::Private
{
    using PermissionCallback = std::function<void(Qt::PermissionStatus)>;

    Qt::PermissionStatus checkPermission(const QPermission &permission);
    void requestPermission(const QPermission &permission, const PermissionCallback &callback);
}

#define QPermissionPluginInterface_iid "org.qt-project.QPermissionPluginInterface.6.5"

class Q_CORE_EXPORT QPermissionPlugin : public QObject
{
public:
    virtual ~QPermissionPlugin();

    virtual Qt::PermissionStatus checkPermission(const QPermission &permission) = 0;
    virtual void requestPermission(const QPermission &permission,
        const QPermissions::Private::PermissionCallback &callback) = 0;
};

QT_END_NAMESPACE

#endif // QPERMISSIONS_P_H
