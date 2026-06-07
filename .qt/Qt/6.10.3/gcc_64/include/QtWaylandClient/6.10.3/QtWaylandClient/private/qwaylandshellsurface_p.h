// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDSHELLSURFACE_H
#define QWAYLANDSHELLSURFACE_H

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

#include <QtCore/QSize>
#include <QObject>
#include <QPoint>
#include <QtWaylandClient/qtwaylandclientglobal.h>
#include <QtCore/private/qglobal_p.h>

#include <any>

struct wl_surface;

QT_BEGIN_NAMESPACE

class QVariant;
class QWindow;
class QPlatformWindow;

namespace QtWaylandClient {

class QWaylandWindow;
class QWaylandInputDevice;

class Q_WAYLANDCLIENT_EXPORT QWaylandShellSurface : public QObject
{
    Q_OBJECT
public:
    explicit QWaylandShellSurface(QWaylandWindow *window);
    ~QWaylandShellSurface() override {}
    virtual bool resize(QWaylandInputDevice *, Qt::Edges) { return false; }
    virtual bool move(QWaylandInputDevice *) { return false; }
    virtual bool showWindowMenu(QWaylandInputDevice *seat) { Q_UNUSED(seat); return false; }
    virtual void setTitle(const QString & /*title*/) {}
    virtual void setAppId(const QString & /*appId*/) {}

    virtual void setWindowFlags(Qt::WindowFlags flags);

    virtual bool isExposed() const { return true; }

    virtual void raise() {}
    virtual void lower() {}
    virtual void setContentOrientationMask(Qt::ScreenOrientations orientation) { Q_UNUSED(orientation); }
    virtual void setContentGeometry(const QRect &rect) { Q_UNUSED(rect); }

    virtual void sendProperty(const QString &name, const QVariant &value);

    virtual void applyConfigure() {}
    virtual void requestWindowStates(Qt::WindowStates states) {Q_UNUSED(states);}
    virtual bool wantsDecorations() const { return false; }
    virtual QMargins serverSideFrameMargins() const { return QMargins(); }

    virtual void propagateSizeHints() {}

    virtual void setWindowGeometry(const QRect &rect);
    virtual void setWindowPosition(const QPoint &position) { Q_UNUSED(position); }
    virtual void setWindowSize(const QSize &size) { Q_UNUSED(size); }

    virtual bool requestActivate() { return false; }
    virtual bool requestActivateOnShow() { return false; }
    virtual void setXdgActivationToken(const QString &token);
    virtual void requestXdgActivationToken(quint32 serial);

    virtual void setAlertState(bool enabled) { Q_UNUSED(enabled); }
    virtual bool isAlertState() const { return false; }

    virtual QString externWindowHandle() { return QString(); }

    inline QWaylandWindow *window() { return m_window; }
    QPlatformWindow *platformWindow();
    struct wl_surface *wlSurface();

    virtual std::any surfaceRole() const { return std::any(); };

    virtual void attachPopup(QWaylandShellSurface *popup) { Q_UNUSED(popup); }
    virtual void detachPopup(QWaylandShellSurface *popup) { Q_UNUSED(popup); }

    virtual void setIcon(const QIcon &icon) { Q_UNUSED(icon); }

    virtual bool commitSurfaceRole() const;

protected:
    void resizeFromApplyConfigure(const QSize &sizeWithMargins, const QPoint &offset = {0, 0});
    void repositionFromApplyConfigure(const QPoint &position);
    void setGeometryFromApplyConfigure(const QPoint &globalPosition, const QSize &sizeWithMargins);
    void applyConfigureWhenPossible();
    void handleActivationChanged(bool activated);

    static uint32_t getSerial(QWaylandInputDevice *inputDevice);

private:
    QWaylandWindow *m_window = nullptr;
    friend class QWaylandWindow;
};

}

QT_END_NAMESPACE

#endif // QWAYLANDSHELLSURFACE_H
