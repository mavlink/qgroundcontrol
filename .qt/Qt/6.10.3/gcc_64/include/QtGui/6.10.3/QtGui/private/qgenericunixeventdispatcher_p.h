// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of other Qt classes. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QGENERICUNIXEVENTDISPATCHER_P_H
#define QGENERICUNIXEVENTDISPATCHER_P_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
namespace QtGenericUnixDispatcher {
Q_GUI_EXPORT QAbstractEventDispatcher *createUnixEventDispatcher();
}
using QtGenericUnixDispatcher::createUnixEventDispatcher;

QT_END_NAMESPACE

#endif // QGENERICUNIXEVENTDISPATCHER_P_H
