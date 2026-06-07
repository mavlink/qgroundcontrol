// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTQUICKDIALOGS2FOREIGN_P_H
#define QTQUICKDIALOGS2FOREIGN_P_H

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

#include <QtGui/qpa/qplatformdialoghelper.h>
#include <QtQml/qqml.h>
#include <QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>

QT_BEGIN_NAMESPACE

struct QQuickFileNameFilterQuickDialogs2Foreign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQuickFileNameFilter)
    QML_ADDED_IN_VERSION(6, 2)
};

QT_END_NAMESPACE

#endif // QTQUICKDIALOGS2FOREIGN_P_H
