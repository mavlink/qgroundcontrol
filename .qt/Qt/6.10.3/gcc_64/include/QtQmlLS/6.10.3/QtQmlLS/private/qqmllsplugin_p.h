// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QMLLSPLUGIN_P_H
#define QMLLSPLUGIN_P_H

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

#include <memory>

#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qobject.h>
#include <QtQmlLS/private/qqmllscompletionplugin_p.h>

QT_BEGIN_NAMESPACE

class QQmlLSPlugin
{
public:
    QQmlLSPlugin() = default;
    virtual ~QQmlLSPlugin() = default;

    Q_DISABLE_COPY_MOVE(QQmlLSPlugin)

    virtual std::unique_ptr<QQmlLSCompletionPlugin> createCompletionPlugin() const = 0;
};

#define QmlLSPluginInterface_iid "org.qt-project.Qt.QmlLS.Plugin/1.0"
Q_DECLARE_INTERFACE(QQmlLSPlugin, QmlLSPluginInterface_iid)

QT_END_NAMESPACE

#endif // QMLLSPLUGIN_P_H
