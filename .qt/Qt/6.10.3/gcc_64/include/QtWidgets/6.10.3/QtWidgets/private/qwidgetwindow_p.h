// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWIDGETWINDOW_P_H
#define QWIDGETWINDOW_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtGui/qwindow.h>

#include <QtCore/private/qobject_p.h>
#include <QtGui/private/qevent_p.h>
#include <QtWidgets/qwidget.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE


class QCloseEvent;
class QMoveEvent;
class QWidgetWindowPrivate;

class QWidgetWindow : public QWindow
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWidgetWindow)
public:
    QWidgetWindow(QWidget *widget);
    ~QWidgetWindow();

    QWidget *widget() const { return m_widget; }
#if QT_CONFIG(accessibility)
    QAccessibleInterface *accessibleRoot() const override;
#endif

    QObject *focusObject() const override;
    void setNativeWindowVisibility(bool visible);
    static void focusNextPrevChild(QWidget *widget, bool next);

protected:
    bool event(QEvent *) override;

    void closeEvent(QCloseEvent *) override;

    void handleEnterLeaveEvent(QEvent *);
    void handleFocusInEvent(QFocusEvent *);
    void handleKeyEvent(QKeyEvent *);
    void handleMouseEvent(QMouseEvent *);
    void handleNonClientAreaMouseEvent(QMouseEvent *);
    void handleTouchEvent(QTouchEvent *);
    void handleMoveEvent(QMoveEvent *);
    void handleResizeEvent(QResizeEvent *);
#if QT_CONFIG(wheelevent)
    void handleWheelEvent(QWheelEvent *);
#endif
#if QT_CONFIG(draganddrop)
    void handleDragEnterEvent(QDragMoveEvent *, QWidget *widget = nullptr);
    void handleDragMoveEvent(QDragMoveEvent *);
    void handleDragLeaveEvent(QDragLeaveEvent *);
    void handleDropEvent(QDropEvent *);
#endif
    void handleExposeEvent(QExposeEvent *);
    void handleWindowStateChangedEvent(QWindowStateChangeEvent *event);
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#if QT_CONFIG(tabletevent)
    void handleTabletEvent(QTabletEvent *);
#endif
#ifndef QT_NO_GESTURES
    void handleGestureEvent(QNativeGestureEvent *);
#endif
#ifndef QT_NO_CONTEXTMENU
    void handleContextMenuEvent(QContextMenuEvent *);
#endif

private slots:
    void updateObjectName();

private:
    void handleScreenChange();
    void handleDevicePixelRatioChange();
    void scheduleRepaint();
    bool updateSize();
    void updateMargins();
    void updateNormalGeometry();

    enum FocusWidgets {
        FirstFocusWidget,
        LastFocusWidget
    };
    QWidget *getFocusWidget(FocusWidgets fw);

    QPointer<QWidget> m_widget;
    QPointer<QWidget> m_implicit_mouse_grabber;
#if QT_CONFIG(draganddrop)
    QPointer<QWidget> m_dragTarget;
#endif
};

QT_END_NAMESPACE

#endif // QWIDGETWINDOW_P_H
