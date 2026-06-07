// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMMENUSEPARATOR_P_H
#define QQUICKLABSPLATFORMMENUSEPARATOR_P_H

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

#include "qquicklabsplatformmenuitem_p.h"

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformMenuSeparator : public QQuickLabsPlatformMenuItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuSeparator)
public:
    explicit QQuickLabsPlatformMenuSeparator(QObject *parent = nullptr);
};

QT_END_NAMESPACE

#endif // QQUICKLABSPLATFORMMENUSEPARATOR_P_H
