// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSHORTCUTCONTEXT_P_P_H
#define QQUICKSHORTCUTCONTEXT_P_P_H

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

#include <QtCore/qnamespace.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

QT_BEGIN_NAMESPACE

class QObject;

struct Q_QUICKTEMPLATES2_EXPORT QQuickShortcutContext
{
    static bool matcher(QObject *object, Qt::ShortcutContext context);
};

QT_END_NAMESPACE

#endif // QQUICKSHORTCUTCONTEXT_P_P_H
