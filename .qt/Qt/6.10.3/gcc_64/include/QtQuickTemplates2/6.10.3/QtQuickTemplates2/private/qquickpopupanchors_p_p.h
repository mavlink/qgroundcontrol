// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPOPUPANCHORS_P_P_H
#define QQUICKPOPUPANCHORS_P_P_H

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

#include <QtCore/private/qobject_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickPopup;

class QQuickPopupAnchorsPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickPopupAnchors)

public:
    static QQuickPopupAnchorsPrivate *get(QQuickPopupAnchors *popupAnchors)
    {
        return popupAnchors->d_func();
    }

    QQuickPopup *popup = nullptr;
    QQuickItem *centerIn = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKPOPUPANCHORS_P_P_H
