// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWIDGETRESIZEHANDLER_P_H
#define QWIDGETRESIZEHANDLER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtCore/qobject.h"
#include "QtCore/qpoint.h"

QT_REQUIRE_CONFIG(resizehandler);

QT_BEGIN_NAMESPACE

class QMouseEvent;
class QKeyEvent;

class QWidgetResizeHandler : public QObject
{
    Q_OBJECT

public:
    explicit QWidgetResizeHandler(QWidget *parent, QWidget *cw = nullptr);
    void setEnabled(bool b);
    bool isEnabled() const;

    bool isButtonDown() const { return buttonDown; }

    void setExtraHeight(int h) { extrahei = h; }

    void setFrameWidth(int w) { fw = w; }

    void doResize();

Q_SIGNALS:
    void activate();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);

private:
    Q_DISABLE_COPY_MOVE(QWidgetResizeHandler)

    enum MousePosition {
        Nowhere,
        TopLeft, BottomRight, BottomLeft, TopRight,
        Top, Bottom, Left, Right,
        Center
    };

    QWidget *widget;
    QWidget *childWidget;
    QPoint moveOffset;
    QPoint invertedMoveOffset;
    MousePosition mode;
    int fw;
    int extrahei;
    int range;
    uint buttonDown     :1;
    uint active         :1;
    uint enabled        :1;

    void setMouseCursor(MousePosition m);
    bool isResizing() const {
        return active && mode != Center;
    }
};

QT_END_NAMESPACE

#endif // QWIDGETRESIZEHANDLER_P_H
