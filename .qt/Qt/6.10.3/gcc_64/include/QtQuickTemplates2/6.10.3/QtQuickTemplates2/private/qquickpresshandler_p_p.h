// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPRESSHANDLER_P_P_H
#define QQUICKPRESSHANDLER_P_P_H

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

#include <QtCore/qpoint.h>
#include <QtCore/qbasictimer.h>
#include <QtCore/private/qglobal_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QMouseEvent;
class QTimerEvent;

struct QQuickPressHandler
{
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void timerEvent(QTimerEvent *event);

    void clearDelayedMouseEvent();
    bool isActive();

    static bool isSignalConnected(QQuickItem *item, const char *signalName, int &signalIndex);

    QQuickItem *control = nullptr;
    QBasicTimer timer;
    QPointF pressPos;
    bool longPress = false;
    int pressAndHoldSignalIndex = -1;
    int pressedSignalIndex = -1;
    int releasedSignalIndex = -1;
    std::unique_ptr<QMouseEvent> delayedMousePressEvent;
};

QT_END_NAMESPACE

#endif // QQUICKPRESSHANDLER_P_P_H
