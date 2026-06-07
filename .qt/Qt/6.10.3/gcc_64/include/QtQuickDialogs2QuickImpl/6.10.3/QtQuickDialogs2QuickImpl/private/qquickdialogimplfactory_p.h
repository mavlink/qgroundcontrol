// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKQMLDIALOGFACTORY_P_H
#define QQUICKQMLDIALOGFACTORY_P_H

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

#include <QtCore/qobject.h>
#include <QtGui/qpa/qplatformdialoghelper.h>
#include <QtQuickDialogs2Utils/private/qquickdialogtype_p.h>

#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKDIALOGS2QUICKIMPL_EXPORT QQuickDialogImplFactory
{
public:
    static std::unique_ptr<QPlatformDialogHelper> createPlatformDialogHelper(QQuickDialogType type, QObject *parent);
};

QT_END_NAMESPACE

#endif // QQUICKQMLDIALOGFACTORY_P_H
