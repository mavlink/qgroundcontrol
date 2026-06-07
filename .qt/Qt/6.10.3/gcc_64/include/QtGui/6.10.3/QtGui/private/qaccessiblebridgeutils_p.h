// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QACCESSIBLEBRIDGEUTILS_H
#define QACCESSIBLEBRIDGEUTILS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtGui/qaccessible.h>

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

namespace QAccessibleBridgeUtils {
    Q_GUI_EXPORT QStringList effectiveActionNames(QAccessibleInterface *iface);
    Q_GUI_EXPORT bool performEffectiveAction(QAccessibleInterface *iface, const QString &actionName);
    Q_GUI_EXPORT QString accessibleId(QAccessibleInterface *accessible);
}

QT_END_NAMESPACE

#endif //QACCESSIBLEBRIDGEUTILS_H
