// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEMDELEGATE_P_P_H
#define QQUICKITEMDELEGATE_P_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKTEMPLATES2_EXPORT QQuickItemDelegatePrivate : public QQuickAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickItemDelegate)

public:
    QPalette defaultPalette() const override;

    bool highlighted = false;
};

QT_END_NAMESPACE

#endif // QQUICKITEMDELEGATE_P_P_H
