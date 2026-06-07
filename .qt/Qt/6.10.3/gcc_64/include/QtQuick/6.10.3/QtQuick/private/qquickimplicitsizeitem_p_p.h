// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKIMPLICITSIZEITEM_P_H
#define QQUICKIMPLICITSIZEITEM_P_H

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

#include "qquickitem_p.h"
#include "qquickpainteditem_p.h"
#include "qquickimplicitsizeitem_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickImplicitSizeItemPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickImplicitSizeItem)

public:
    QQuickImplicitSizeItemPrivate()
    {
    }
};

QT_END_NAMESPACE

#endif // QQUICKIMPLICITSIZEITEM_P_H
