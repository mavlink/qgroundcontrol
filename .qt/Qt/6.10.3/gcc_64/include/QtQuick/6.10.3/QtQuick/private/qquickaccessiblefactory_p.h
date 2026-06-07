// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKACCESSIBLEFACTORY_H
#define QQUICKACCESSIBLEFACTORY_H

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

#include <QtGui/qaccessible.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE
#if QT_CONFIG(accessibility)

QAccessibleInterface *qQuickAccessibleFactory(const QString &classname, QObject *object);

#endif
QT_END_NAMESPACE

#endif
