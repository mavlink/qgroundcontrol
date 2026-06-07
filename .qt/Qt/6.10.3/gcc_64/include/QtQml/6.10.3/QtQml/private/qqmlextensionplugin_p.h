// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLEXTENSIONPLUGIN_P_H
#define QQMLEXTENSIONPLUGIN_P_H

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

#include <QtCore/private/qobject_p.h>
#include "qqmlextensionplugin.h"

QT_BEGIN_NAMESPACE

#if QT_DEPRECATED_SINCE(6, 3)
class QQmlExtensionPluginPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlExtensionPlugin)

public:
    static QQmlExtensionPluginPrivate* get(QQmlExtensionPlugin *e) { return e->d_func(); }

    QUrl baseUrl;

};
#endif

QT_END_NAMESPACE

#endif // QQMLEXTENSIONPLUGIN_P_H
