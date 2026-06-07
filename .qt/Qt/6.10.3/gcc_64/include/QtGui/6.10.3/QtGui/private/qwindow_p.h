// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOW_P_H
#define QWINDOW_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <qpa/qplatformwindow.h>

#include <QtCore/private/qobject_p.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qxpfunctional.h>
#include <QtGui/qicon.h>
#include <QtGui/qpalette.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QWindowPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QWindow)

public:
    enum PositionPolicy
    {
        WindowFrameInclusive,
        WindowFrameExclusive
    };

    QWindowPrivate(decltype(QObjectPrivateVersion) version = QObjectPrivateVersion);
    ~QWindowPrivate() override;

    void init(QWindow *parent, QScreen *targetScreen = nullptr);

#ifndef QT_NO_CURSOR
    void setCursor(const QCursor *c = nullptr);
    bool applyCursor();
#endif

    QPoint globalPosition() const;

    QWindow *topLevelWindow(QWindow::AncestorMode mode = QWindow::IncludeTransients) const;

    virtual QWindow *eventReceiver() { Q_Q(QWindow); return q; }
    virtual QPalette windowPalette() const { return QPalette(); }

    virtual void setVisible(bool visible);
    void updateVisibility();
    void _q_clearAlert();

    enum SiblingPosition { PositionTop, PositionBottom };
    void updateSiblingPosition(SiblingPosition);

    bool windowRecreationRequired(QScreen *newScreen) const;
    void create(bool recursive);
    void destroy();
    void setTopLevelScreen(QScreen *newScreen, bool recreate);
    void connectToScreen(QScreen *topLevelScreen);
    void disconnectFromScreen();
    void emitScreenChangedRecursion(QScreen *newScreen);
    QScreen *screenForGeometry(const QRect &rect) const;
    void setTransientParent(QWindow *parent);

    virtual void clearFocusObject();

    enum class FocusTarget {
        First,
        Last,
        Current,
        Next,
        Prev
    };
    virtual void setFocusToTarget(FocusTarget, Qt::FocusReason) {}

    virtual QRectF closestAcceptableGeometry(const QRectF &rect) const;

    void setMinOrMaxSize(QSize *oldSizeMember, const QSize &size,
                         qxp::function_ref<void()> funcWidthChanged,
                         qxp::function_ref<void()> funcHeightChanged);

    virtual bool participatesInLastWindowClosed() const;
    virtual bool treatAsVisible() const;

    virtual void maybeSynthesizeContextMenuEvent(QMouseEvent *event);

    const QWindow *forwardToPopup(QEvent *event, const QWindow *activePopupOnPress);

    bool isPopup() const { return (windowFlags & Qt::WindowType_Mask) == Qt::Popup; }
    void setAutomaticPositionAndResizeEnabled(bool a)
    { positionAutomatic = resizeAutomatic = a; }

    bool updateDevicePixelRatio();

    static QWindowPrivate *get(QWindow *window) { return window->d_func(); }

    static Qt::WindowState effectiveState(Qt::WindowStates);

    QWindow::SurfaceType surfaceType = QWindow::RasterSurface;
    Qt::WindowFlags windowFlags = Qt::Window;
    QWindow *parentWindow = nullptr;
    QPlatformWindow *platformWindow = nullptr;
    bool visible= false;
    bool visibilityOnDestroy = false;
    bool exposed = false;
    bool inClose = false;
    QSurfaceFormat requestedFormat;
    QString windowTitle;
    QString windowFilePath;
    QIcon windowIcon;
    QRect geometry;
    qreal devicePixelRatio = 1.0;
    Qt::WindowStates windowState = Qt::WindowNoState;
    QWindow::Visibility visibility = QWindow::Hidden;
    bool resizeEventPending = true;
    bool receivedExpose = false;
    PositionPolicy positionPolicy = WindowFrameExclusive;
    bool positionAutomatic = true;
    // resizeAutomatic suppresses resizing by QPlatformWindow::initialGeometry().
    // It also indicates that width/height=0 is acceptable (for example, for
    // the QRollEffect widget) and is thus not cleared in setGeometry().
    // An alternative approach might be using -1,-1 as a default size.
    bool resizeAutomatic = true;
    Qt::ScreenOrientation contentOrientation = Qt::PrimaryOrientation;
    qreal opacity= 1;
    QRegion mask;

    QSize minimumSize = {0, 0};
    QSize maximumSize = {QWINDOWSIZE_MAX, QWINDOWSIZE_MAX};
    QSize baseSize;
    QSize sizeIncrement;

    Qt::WindowModality modality = Qt::NonModal;
    bool blockedByModalWindow = false;

    bool updateRequestPending = false;
    bool transientParentPropertySet = false;

    QPointer<QWindow> transientParent;
    QPointer<QScreen> topLevelScreen;

#ifndef QT_NO_CURSOR
    QCursor cursor = {Qt::ArrowCursor};
    bool hasCursor = false;
#endif

    QElapsedTimer lastComposeTime;

#if QT_CONFIG(vulkan)
    QVulkanInstance *vulkanInstance = nullptr;
#endif
};


QT_END_NAMESPACE

#endif // QWINDOW_P_H
