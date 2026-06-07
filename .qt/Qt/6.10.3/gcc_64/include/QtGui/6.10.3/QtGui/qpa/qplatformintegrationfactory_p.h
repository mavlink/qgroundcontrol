// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMINTEGRATIONFACTORY_H
#define QPLATFORMINTEGRATIONFACTORY_H

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


class QPlatformIntegration;

class Q_GUI_EXPORT QPlatformIntegrationFactory
{
public:
    static QStringList keys(const QString &platformPluginPath = QString());
    static QPlatformIntegration *create(const QString &name, const QStringList &args, int &argc, char **argv, const QString &platformPluginPath = QString());
};

QT_END_NAMESPACE

#endif // QPLATFORMINTEGRATIONFACTORY_H

