// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFOCUSSCOPE_P_H
#define QQUICKFOCUSSCOPE_P_H

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

#include <private/qtquickglobal_p.h>

#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickFocusScope : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FocusScope)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickFocusScope(QQuickItem *parent=nullptr);
};

QT_END_NAMESPACE

#endif // QQUICKFOCUSSCOPE_P_H
