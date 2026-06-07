// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMTHEMEFACTORY_H
#define QPLATFORMTHEMEFACTORY_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE


class QPlatformTheme;

class Q_GUI_EXPORT QPlatformThemeFactory
{
public:
    static QStringList keys(const QString &platformPluginPath = QString());
    static QPlatformTheme *create(const QString &key, const QString &platformPluginPath = QString());
};

QT_END_NAMESPACE

#endif // QPLATFORMTHEMEFACTORY_H
