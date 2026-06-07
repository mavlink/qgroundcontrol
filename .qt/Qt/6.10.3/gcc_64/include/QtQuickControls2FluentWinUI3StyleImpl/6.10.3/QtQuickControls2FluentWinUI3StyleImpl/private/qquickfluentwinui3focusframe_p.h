// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFLUENTWINUI3FOCUSFRAME_H
#define QQUICKFLUENTWINUI3FOCUSFRAME_H
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
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include "qquickfluentwinui3styleimplglobal_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2FLUENTWINUI3STYLEIMPL_EXPORT QQuickFluentWinUI3FocusFrame : public QObject
{
    Q_OBJECT

public:
    QQuickFluentWinUI3FocusFrame();

private:
    static QScopedPointer<QQuickItem> m_focusFrame;

    QQuickItem *createFocusFrame(QQmlContext *context);
    void moveToItem(QQuickControl *item);
    QQuickControl *getFocusTarget(QQuickControl *focusItem) const;
};

QT_END_NAMESPACE

#endif // QQUICKFLUENTWINUI3FOCUSFRAME_H
